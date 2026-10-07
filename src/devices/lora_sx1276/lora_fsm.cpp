#include <RadioLib.h>
#include "lora_fsm.h"
#include "lora_sx1276.h"
#include "application/time/time.h"
#include "application/system_data/system_data.h"
#include "radiolib_stm32_hal/radiolib_stm32_hal.h"
#include "protocols/uart/uart.h" // pack_str()/pack_int()/uart_send_string() for lora_debug_dump()

/* The Module object is created in main.cpp. It is needed here only to read a
   raw SX1276 register: SX127x::getMod() in RadioLib is protected (public only
   with RADIOLIB_GODMODE/RADIOLIB_LOW_LEVEL), so SPIreadRegister() cannot be
   reached through g_radio. */
extern Module module;

static SX1276 *g_radio = nullptr; // Radio driven by the FSM, set in lora_fsm_init()

typedef void (*lora_state_handler_t)(void);
lora_state_handler_t g_lora_state_handler = nullptr; // Current state handler; nullptr = FSM stopped

/* Explicit transmit request from a producer (lora_tx_request()).
   The payload is borrowed, never copied - see lora_fsm.h. */
static const char *g_tx_payload = nullptr;                  // Borrowed payload of the pending request
static volatile lora_tx_state_t g_tx_state = lora_tx_idle; // State of that request

/* Payload sources merged into one packet. */
#define LORA_SRC_SYSTEM   (1 << 0)   /* system_data.data_string */
#define LORA_SRC_REQUEST  (1 << 1)   /* lora_tx_request() payload */

/* Packet assembled from every ready source, and the sources it carries. */
static char g_tx_current[LORA_TX_PACKET_LEN]; // Packet handed to startTransmit()
static uint8_t g_tx_sources = 0;              // LORA_SRC_* bits of the packet on air

/* When only one source is ready, the other gets a short window to join
   the same packet instead of costing a second transmission. */
static timeout_t g_tx_join_timer; // LORA_TX_JOIN_MS window
static uint8_t g_tx_join_armed = 0; // 1 while the join window is running

/* Limit for waiting on the DIO0 "TX done" interrupt. */
static timeout_t g_tx_timeout; // LORA_TX_TIMEOUT_MS

/* Guard for the case when the DIO0 line is constantly held at "1". */
static timeout_t g_dio0_stuck_timer; // LORA_DIO0_STUCK_MS
static uint8_t g_dio0_stuck_armed = 0; // 1 while DIO0 has been seen high with data ready

/* Retry radio initialization after a failed begin(). */
static timeout_t g_init_retry_timer; // LORA_INIT_RETRY_MS
static uint8_t g_init_retry_armed = 0; // 1 after a failed begin(): wait before retrying

volatile uint8_t lora_tx_done_flag = 0; // Set in onTxDone() (EXTI1 interrupt), cleared by the FSM; volatile for ISR access

/* Counter of actually transmitted packets. The main loop uses it to
   know that this cycle's data has gone on air and it may go to sleep. */
volatile uint32_t lora_tx_packet_count = 0;


// static timeout_t g_timer; // unused - removed to avoid warning
int g_begin_state; // Last radio.begin() result

static void lora_state_init(void);
// static void lora_state_rx(void);
// static void lora_state_rx_wait(void);
// static void lora_state_check(void);
static void lora_state_tx_done(void);
static void lora_state_tx_wait(void);
static void lora_state_tx(void);

/* DIO0 callback, runs in EXTI1 interrupt context: only sets a flag. */
void onTxDone(void)
{
    lora_tx_done_flag = 1; // Processed by lora_state_tx_done() in the main loop
}

/* Resets the FSM; the radio is initialized by the first lora_fsm_run() call. */
void lora_fsm_init(SX1276 *radio)
{
    g_radio = radio;          // Radio to drive
    g_tx_payload = nullptr;   // No pending request
    g_tx_current[0] = '\0';   // Empty packet buffer
    g_tx_sources = 0;         // No packet on air
    g_tx_join_armed = 0;      // Join window not running
    g_tx_state = lora_tx_idle; // Request state: idle
    g_init_retry_armed = 0; // First begin() is attempted immediately
    g_dio0_stuck_armed = 0; // DIO0 stuck guard not running
    g_lora_state_handler = lora_state_init; // Start with radio initialization
}

/* Accepts a payload unless a previous request is still pending. */
uint8_t lora_tx_request(const char *payload)
{
    if (payload == nullptr || g_tx_state == lora_tx_pending)
    {
        return 0; // Invalid payload or a request is already queued
    }

    g_tx_payload = payload;       // Borrow the caller's buffer
    g_tx_state = lora_tx_pending; // Picked up by lora_state_tx_wait()

    return 1;
}

/* Last packet built for the radio. */
const char *lora_tx_packet(void)
{
    return g_tx_current;
}

/* State of the last lora_tx_request(). */
lora_tx_state_t lora_tx_status(void)
{
    return g_tx_state;
}

/* Clears a finished result; a pending request is left untouched. */
void lora_tx_clear(void)
{
    if (g_tx_state == lora_tx_done || g_tx_state == lora_tx_failed)
    {
        g_tx_state = lora_tx_idle;
    }
}

/* Runs the current state handler once. */
void lora_fsm_run(void)
{
    if (g_lora_state_handler != nullptr) // FSM stopped if no handler
    {
        g_lora_state_handler();
    }
}

/* ==========================================================================
   LoRa state snapshot as one UART line (diagnostics).
   Changes no radio state: only reads the FSM's own flags and one SX1276
   register (SPIreadRegister is read-only and does not touch the radio mode).
   ========================================================================== */

/* Packet counter at the PREVIOUS call. A difference from the current value
   means "there was a transmission since the last dump". This way "no tx" is not
   confused with a real repeated packet, and g_tx_current is not shown if it is
   left over from a failed attempt or a previous cycle. */
static uint32_t g_dbg_prev_count = 0;

/* Writes v as "0xNN" at p; returns the position after it. */
static char *dbg_hex8(char *p, uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF"; // Nibble -> hex digit

    *p++ = '0';
    *p++ = 'x';
    *p++ = hex[(v >> 4) & 0x0Fu]; // High nibble
    *p++ = hex[v & 0x0Fu];        // Low nibble

    return p;
}

/* Builds and prints the one-line state snapshot (format in lora_fsm.h). */
void lora_debug_dump(void)
{
    /* Longest line: 7 + 10 (counter) + 11 + 11 + 9 + 8 + 6 + 5 +
       127 (packet) + 1 = ~195 bytes. 224 gives margin. */
    char line[224];
    char *p = line;

    uint32_t count = lora_tx_packet_count; // Single read of the volatile counter

    p = pack_str(p, "LORA n=");
    p = pack_int(p, (int32_t)count);

    p = pack_str(p, " st="); // FSM state, identified by the handler pointer
    if (g_lora_state_handler == nullptr)
    {
        p = pack_str(p, "null");
    }
    else if (g_lora_state_handler == lora_state_init)
    {
        p = pack_str(p, "init");
    }
    else if (g_lora_state_handler == lora_state_tx_wait)
    {
        p = pack_str(p, "tx_wait");
    }
    else if (g_lora_state_handler == lora_state_tx)
    {
        p = pack_str(p, "tx");
    }
    else if (g_lora_state_handler == lora_state_tx_done)
    {
        p = pack_str(p, "tx_done");
    }
    else
    {
        p = pack_str(p, "unknown");
    }

    p = pack_str(p, " tx="); // lora_tx_request() state
    switch (g_tx_state)
    {
    case lora_tx_idle:    p = pack_str(p, "idle");    break;
    case lora_tx_pending: p = pack_str(p, "pending"); break;
    case lora_tx_done:    p = pack_str(p, "done");    break;
    case lora_tx_failed:  p = pack_str(p, "failed");  break;
    default:              p = pack_str(p, "?");       break;
    }

    p = pack_str(p, " busy=");
    p = pack_int(p, (int32_t)system_data.lora_busy);

    /* RegOpMode (0x01). Bit 7 - LongRangeMode: 1 = LoRa, 0 = FSK/OOK. */
    uint8_t op_mode = 0;
    if (g_radio != nullptr)
    {
        op_mode = module.SPIreadRegister(RADIOLIB_SX127X_REG_OP_MODE); // Raw register read over SPI
    }

    p = pack_str(p, " op=");
    p = dbg_hex8(p, op_mode);

    p = pack_str(p, " LRM=");
    *p++ = (op_mode & 0x80u) ? '1' : '0'; // RegOpMode bit 7

    p = pack_str(p, " pkt=");
    if (count != g_dbg_prev_count)
    {
        p = pack_str(p, lora_tx_packet()); // What actually went on air since the last dump
    }
    else
    {
        p = pack_str(p, "no tx");
    }

    g_dbg_prev_count = count; // Reference for the next call

    *p = '\0';

    uart_send_string(line);
}

/* Whether it is safe to enter Stop mode now.
   Sleeping during a transmission would stop SPI and DIO0 detection mid-packet:
   the packet would be cut off and lora_busy would stay at 1 forever. */
uint8_t lora_fsm_is_idle(void)
{
    if (system_data.lora_busy != 0)
    {
        return 0; // A packet is being prepared or is on air
    }

    /* A pending lora_tx_request() deliberately does NOT block sleep.
       lora_tx_pending is a software state (a producer queued a payload), not
       an indication that the radio is on air. If the FSM cannot transmit (e.g.
       DIO0 broken high, see lora_state_tx_wait()), the request could stay
       pending indefinitely; blocking sleep on it would keep the MCU awake at
       full current while the main loop still feeds the watchdog - a battery
       sized for months would be drained in days. Sleeping with a pending
       request is safe: the payload buffer is borrowed and remains valid, and
       the request is picked up again after the wakeup. */

    if (lora_tx_done_flag != 0)
    {
        return 0; // TX done interrupt not processed yet
    }

    /* If the handler is nullptr the FSM is stopped. This is treated as idle:
       otherwise the main loop would never enter Stop mode and a board with a
       faulty radio would drain the battery. */
    if (g_lora_state_handler == nullptr)
    {
        return 1;
    }

    /* The (re)initialization state is also idle: the radio is not on air, so
       the device may sleep and retry after the next wakeup. */
    if (g_lora_state_handler == lora_state_init)
    {
        return 1;
    }

    return (g_lora_state_handler == lora_state_tx_wait) ? 1 : 0; // tx / tx_done: transmission in progress
}

/* SX1276 in STANDBY (where lora_state_tx_done() leaves it) draws ~1.5 mA,
   hundreds of times more than the MCU in Stop mode, so it is put into SLEEP
   (~0.2 uA) before the MCU sleeps. */
uint8_t lora_radio_sleep(void)
{
    if (g_radio == nullptr)
    {
        return 0; // FSM not initialized
    }

    return (g_radio->sleep() == RADIOLIB_ERR_NONE) ? 1 : 0; // RegOpMode Mode = SLEEP
}

/* Returns the radio to STANDBY after a wakeup.
   startTransmit() in RadioLib would switch to STANDBY by itself, and the
   SX1276 configuration (frequency, SF, LoRa mode) survives SLEEP -> STANDBY,
   because setMode() changes only RegOpMode Mode[2:0] and not LongRangeMode.
   The explicit wake makes the radio state predictable and gives the oscillator
   time to start before the first SPI exchange. The FIFO contents lost in SLEEP
   do not matter: the packet is reloaded before every transmission. */
uint8_t lora_radio_wake(void)
{
    if (g_radio == nullptr)
    {
        return 0; // FSM not initialized
    }

    return (g_radio->standby() == RADIOLIB_ERR_NONE) ? 1 : 0; // RegOpMode Mode = STANDBY
}

/* Initializes the radio. A failed begin() is retried every LORA_INIT_RETRY_MS
   instead of stopping the FSM for good: begin() can fail for transient reasons
   (e.g. the chip not answering on SPI right after power-up), and a single
   failure at startup must not leave the node silent until a reset. The pause
   does not block the main loop: the FSM simply returns here on the next call. */
static void lora_state_init(void)
{
    if (g_init_retry_armed && !timer_wait(&g_init_retry_timer))
    {
        return; // Retry pause not over yet
    }

    int state = g_radio->begin(915.0, 125.0, 7, 5, 0x34, 17, 8, true); // 915 MHz, BW 125 kHz, SF7, CR 4/5, sync word 0x34, 17 dBm, preamble 8, LNA gain 1 (true -> 1 = max gain, AGC off)
    g_begin_state = state; // Kept for diagnostics

    if (state == RADIOLIB_ERR_NONE)
    {
        g_init_retry_armed = 0; // Success: no retry pending

        g_radio->setDio0Action(onTxDone, 0); // Register onTxDone as the DIO0 callback (stored by hal.attachInterrupt)
        // g_lora_state_handler = lora_state_rx;
        g_lora_state_handler = lora_state_tx_wait; // Ready to transmit
    }
    else
    {
        timer_set(&g_init_retry_timer, LORA_INIT_RETRY_MS); // Try again later
        g_init_retry_armed = 1;
        // handler is NOT nulled - stay in this state and try again
    }
}
/*
static void lora_state_rx(void)
{
    int state = g_radio->startReceive();
    if (state == RADIOLIB_ERR_NONE)
    {
        g_lora_state_handler = lora_state_check;

    }
}

static void lora_state_check(void)
{
    int state = g_radio->readData(g_rx, sizeof(g_rx)-1);
    if (state == RADIOLIB_ERR_NONE)
    {
        if (g_rx[0] == '5')
        {
            timer_set(&g_timer, 50);
            g_lora_state_handler = lora_state_tx_wait;
        }
        else
        {
            g_lora_state_handler = lora_state_rx;
        }
    }
}
*/
/* Bounded append, keeps room for the terminator. */
static char *lora_tx_append(char *p, const char *end, const char *s)
{
    while (*s != '\0' && p < end) // Stop at the end of s or of the buffer
    {
        *p++ = *s++;
    }

    return p;
}

/* Concatenate every ready source into g_tx_current. */
static void lora_tx_build(uint8_t sources)
{
    char *p = g_tx_current;
    char *end = g_tx_current + LORA_TX_PACKET_LEN - 1; // Last byte reserved for '\0'

    if (sources & LORA_SRC_SYSTEM)
    {
        p = lora_tx_append(p, end, system_data.data_string); // Sensor CSV string
    }

    if ((sources & LORA_SRC_SYSTEM) && (sources & LORA_SRC_REQUEST) && p < end)
    {
        *p++ = LORA_TX_SEPARATOR; // Separator only between two parts
    }

    if (sources & LORA_SRC_REQUEST)
    {
        p = lora_tx_append(p, end, g_tx_payload); // Requested payload
    }

    *p = '\0';
}

/* Waits for data to send, handles a stuck DIO0 line and the join window,
   then builds the packet and moves to lora_state_tx(). */
static void lora_state_tx_wait(void)
{
    uint8_t sources = 0; // LORA_SRC_* bits ready in this call

    if (system_data.lora_busy != 0)
    {
        return; // Radio still marked busy
    }

    /* DIO0 is expected low before a new transmission. A broken wire or bad
       contact on PB1 can make it read "1" permanently; returning early forever
       would stop all transmissions (and leave a request pending) although the
       radio itself works. Therefore a high DIO0 is tolerated only for
       LORA_DIO0_STUCK_MS: then the radio is forced to STANDBY and the FSM
       continues. If the line is really broken, the packet is still sent and
       its end is detected by the LORA_TX_TIMEOUT_MS limit in lora_state_tx_done()
       instead of the interrupt. */
    if (hal.digitalRead(1) != 0) // DIO0 (PB1) high
    {
        if (!g_dio0_stuck_armed)
        {
            timer_set(&g_dio0_stuck_timer, LORA_DIO0_STUCK_MS); // Start the tolerance window
            g_dio0_stuck_armed = 1;
            return;
        }

        if (!timer_wait(&g_dio0_stuck_timer))
        {
            return; // Still within the tolerance window
        }

        (void)g_radio->standby(); // Force STANDBY before transmitting despite the high line
        g_dio0_stuck_armed = 0;
        // Don't return: transmit even if the line stays at "1"
    }
    else
    {
        g_dio0_stuck_armed = 0; // DIO0 low: reset the guard
    }

    if (system_data.ready_data_creation_flag == 1)
    {
        sources |= LORA_SRC_SYSTEM; // data_string has been built by system_data_run()
    }

    if (g_tx_state == lora_tx_pending && g_tx_payload != nullptr)
    {
        sources |= LORA_SRC_REQUEST; // A producer queued a payload
    }

    if (sources == 0)
    {
        g_tx_join_armed = 0; // Nothing to send: reset the join window
        return;
    }

    /* Only one source so far: wait out the join window before giving up on
       merging, so both payloads normally travel in a single packet. */
    if (sources != (LORA_SRC_SYSTEM | LORA_SRC_REQUEST))
    {
        if (!g_tx_join_armed)
        {
            timer_set(&g_tx_join_timer, LORA_TX_JOIN_MS); // Start the join window
            g_tx_join_armed = 1;
            return;
        }

        if (!timer_wait(&g_tx_join_timer))
        {
            return; // Still waiting for the other source
        }
    }

    lora_tx_build(sources); // Assemble g_tx_current

    g_tx_join_armed = 0;
    g_tx_sources = sources;              // Remember what this packet carries
    g_lora_state_handler = lora_state_tx; // Transmit on the next call
    system_data.lora_busy = 1;           // Block new data creation and sleep until done
}
/* Starts the transmission; DIO0 signals completion. */
static void lora_state_tx(void)
{

    lora_tx_done_flag = 0; // Drop any stale TX done before starting

    int state = g_radio->startTransmit(g_tx_current); // Load FIFO and enter TX (non-blocking)
    if (state == RADIOLIB_ERR_NONE)
    {
        timer_set(&g_tx_timeout, LORA_TX_TIMEOUT_MS); // Limit for waiting on DIO0
        g_lora_state_handler = lora_state_tx_done;
    }
    else
    {

        if (g_tx_sources & LORA_SRC_REQUEST)
        {
            g_tx_state = lora_tx_failed; // Report the failure to the producer
        }

        g_tx_sources = 0;
        system_data.lora_busy = 0;                 // Radio free again; data_string stays ready and is retried
        g_lora_state_handler = lora_state_tx_wait;
    }
}

/* Waits for TX done (DIO0) or the LORA_TX_TIMEOUT_MS limit, then releases the radio. */
static void lora_state_tx_done(void)
{
    /* Timeout waiting for the TX done interrupt.
       lora_tx_done_flag is set only from the EXTI1 handler (DIO0 on PB1). If
       that interrupt is lost (edge while the line was masked for sleep, bounce,
       SPI glitch), waiting without a limit would keep the FSM here and
       system_data.lora_busy at 1 forever: lora_fsm_is_idle() would never return
       1, the device would never sleep or transmit again, and the battery would
       be drained within days. With ~210 ms time on air for the longest packet,
       LORA_TX_TIMEOUT_MS = 3000 ms makes a false timeout practically impossible. */
    if (!lora_tx_done_flag && timer_wait(&g_tx_timeout))
    {
        (void)g_radio->standby(); // Force the transmitter off the air

        if (g_tx_sources & LORA_SRC_REQUEST)
        {
            /* Mark the request as failed and release the payload, otherwise the
               producer would wait forever for lora_tx_pending to end. */
            g_tx_payload = nullptr;
            g_tx_state = lora_tx_failed;
        }

        g_tx_sources = 0;
        lora_tx_done_flag = 0;
        system_data.lora_busy = 0;                 // data_string stays ready and is retried
        g_lora_state_handler = lora_state_tx_wait;
        return;
    }

    if (lora_tx_done_flag) // TX done received from DIO0
    {

        (void)g_radio->finishTransmit(); // RadioLib post-transmit cleanup; result ignored

        lora_tx_done_flag = 0;

        g_radio->standby(); // Make sure the radio is in STANDBY

        if (g_tx_sources & LORA_SRC_REQUEST)
        {
            g_tx_payload = nullptr;    // Release the borrowed buffer
            g_tx_state = lora_tx_done; // Report success to the producer
        }

        if (g_tx_sources & LORA_SRC_SYSTEM)
        {
            system_data.ready_data_creation_flag = 0; // data_string consumed
            system_data.ready_sensors_flag = 0;       // All sensors measure again for the next packet
        }

        g_tx_sources = 0;
        system_data.lora_busy = 0;
        lora_tx_packet_count++; // packet actually went on air
        g_lora_state_handler = lora_state_tx_wait;
    }
}
/*
static void lora_state_rx_wait(void)
{
    if (timer_wait(&g_timer))
    {
        g_lora_state_handler = lora_state_rx;
    }
}
*/
