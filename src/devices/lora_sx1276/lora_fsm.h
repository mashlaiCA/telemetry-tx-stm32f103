/**
 * @file  lora_fsm.h
 * @brief Non-blocking LoRa transmit FSM for the SX1276 (RadioLib), plus Stop-mode helpers.
 * This file provides:
 * 1. Packet size and timing constants of the FSM.
 * 2. The transmit FSM (C++ only): lora_fsm_init() / lora_fsm_run() and the DIO0 callback onTxDone().
 * 3. A C API for producers: lora_tx_request(), lora_tx_status(), lora_tx_clear(), lora_tx_packet().
 * 4. Sleep support: lora_fsm_is_idle(), lora_radio_sleep(), lora_radio_wake(), lora_tx_packet_count.
 * 5. lora_debug_dump() - one-line UART state snapshot.
 * FSM states: init (radio.begin(), retried every LORA_INIT_RETRY_MS) -> tx_wait
 * (collect sources) -> tx (startTransmit) -> tx_done (wait for DIO0 or timeout) -> tx_wait.
 * A packet is built from system_data.data_string and/or one lora_tx_request()
 * payload, joined by LORA_TX_SEPARATOR.
 */

#ifndef LORA_FSM_H
#define LORA_FSM_H

#include <stdint.h>

/* Size of the transmit buffer g_tx_current, including the terminator.
   It must hold system_data.data_string (up to 48 bytes) + LORA_TX_SEPARATOR +
   one lora_tx_request() payload such as a Watermark packet (WM_PACKET_LEN = 56):
   48 + 1 + 56 = 105 bytes. Longer input is truncated at 127 characters.
   The SX1276 accepts up to 255 bytes, so 128 is within limits. */
#define LORA_TX_PACKET_LEN   128
#define LORA_TX_SEPARATOR    ','

/* When only one source is ready, how long the FSM waits for the other one to
   join the same packet before transmitting what it has.
   Both sources start at the wakeup moment, so normally the gap is milliseconds;
   1000 ms leaves margin for I2C bus delays, SHT35 retries with soft reset and
   bus recoveries. The window only shortens the sleep time, it does not
   extend the cycle (the period is fixed by CYCLE_PERIOD_S in main.cpp).
   NOTE: currently no module calls lora_tx_request() (the Watermark value is
   carried inside system_data.data_string), so every system_data packet waits
   the full window before it is transmitted. */
#define LORA_TX_JOIN_MS      1000

/* How long to wait for the DIO0 "TX done" interrupt before treating the
   transmission as failed. Time on air for the longest packet (127 bytes,
   SF7/BW125/CR4/5, 8-symbol preamble, explicit header, CRC on) is ~210 ms,
   so the margin is about 14x. Without this limit a lost interrupt would leave
   system_data.lora_busy = 1 forever: the device would never sleep or transmit again. */
#define LORA_TX_TIMEOUT_MS   3000

/* Pause between radio.begin() retries after a failure. */
#define LORA_INIT_RETRY_MS   2000

/* How long DIO0 may stay high while data is ready before the FSM forces the
   radio to STANDBY and transmits anyway. A healthy radio keeps DIO0 low between
   packets, so 2 s of a constant high level indicates a line fault. */
#define LORA_DIO0_STUCK_MS   2000

/** @brief Result of the last transmission requested with lora_tx_request(). */
typedef enum {
    lora_tx_idle = 0,   // No request pending (initial state, or after lora_tx_clear())
    lora_tx_pending,    // Request accepted, payload not yet on air
    lora_tx_done,       // Payload transmitted (DIO0 TX done received)
    lora_tx_failed      // startTransmit() failed or TX done not received within LORA_TX_TIMEOUT_MS
} lora_tx_state_t;

#ifdef __cplusplus

#include <RadioLib.h>

typedef void (*lora_state_handler_t)(void); // FSM state handler type
extern lora_state_handler_t g_lora_state_handler; // Current state handler; nullptr = FSM stopped

extern int statusTransmit; // Declared only; not defined anywhere in the project
extern int statusTXdone;   // Declared only; not defined anywhere in the project
extern int finish;         // Declared only; not defined anywhere in the project

extern int g_begin_state; // Result of the last radio.begin() call (RADIOLIB_ERR_NONE = 0 on success)

void onTxDone(void);                // DIO0 callback (interrupt context): sets lora_tx_done_flag
void lora_fsm_init(SX1276* radio);  // Stores the radio, resets the FSM to the init state
void lora_fsm_run(void);            // Executes one FSM step; call from the main loop

extern "C" {
#endif

/**
 * @brief Hands a payload to the LoRa FSM for transmission.
 * The FSM is the only place the radio is driven, so every producer goes
 * through this call. The buffer is not copied: it must stay valid and
 * unchanged until lora_tx_status() leaves lora_tx_pending.
 *
 * The payload is not sent on its own: the FSM concatenates it with
 * system_data.data_string, separated by LORA_TX_SEPARATOR, so one
 * transmission carries both. If the other source is not ready, the FSM waits
 * up to LORA_TX_JOIN_MS for it and then sends whatever it has.
 *
 * @param payload Null-terminated string to transmit.
 * @return 1 if the request was accepted (state becomes lora_tx_pending);
 *         0 if payload is NULL or a previous request is still pending.
 */
uint8_t lora_tx_request(const char *payload);

/**
 * @brief Returns the packet last built for the radio (the joined payloads).
 * The buffer may hold a packet whose transmission failed.
 */
const char *lora_tx_packet(void);

/**
 * @brief Returns the state of the last transmission requested with lora_tx_request().
 */
lora_tx_state_t lora_tx_status(void);

/**
 * @brief Resets a finished result (lora_tx_done / lora_tx_failed) to lora_tx_idle.
 * Has no effect while a request is pending.
 */
void lora_tx_clear(void);

/**
 * @brief Reports whether it is safe to enter Stop mode with respect to the radio.
 * Stop mode stops the SPI clock and DIO0 detection, so sleeping mid-transmission
 * would truncate the packet and leave lora_busy stuck at 1.
 * @return 1 if lora_busy == 0, no unhandled TX-done flag is pending and the FSM
 *         is in tx_wait, in init (radio not up yet) or stopped (handler nullptr);
 *         0 otherwise. A pending lora_tx_request() does NOT prevent sleep.
 */
uint8_t lora_fsm_is_idle(void);

/**
 * @brief Number of packets actually transmitted (TX done received from DIO0).
 * Incremented in the tx_done state; not incremented on timeout or failure.
 * The main loop samples it at the start of a cycle and ends the active phase
 * when it changes, i.e. when this cycle's data really went on air.
 */
extern volatile uint32_t lora_tx_packet_count;

/**
 * @brief Puts the SX1276 into SLEEP before the MCU enters Stop mode.
 * STANDBY costs about 1.5 mA, SLEEP about 0.2 uA.
 * @return 1 on success; 0 if the FSM has no radio or RadioLib reported an error.
 */
uint8_t lora_radio_sleep(void);

/**
 * @brief Brings the SX1276 from SLEEP back to STANDBY after a wakeup.
 * The LoRa configuration is kept in SLEEP (a mode change does not touch the
 * LongRangeMode bit of RegOpMode); only the FIFO contents are lost, and the
 * FIFO is reloaded before every transmission anyway.
 * @return 1 on success; 0 if the FSM has no radio or RadioLib reported an error.
 */
uint8_t lora_radio_wake(void);

/**
 * @brief Prints a LoRa state snapshot to UART in one line (diagnostics).
 * Contents: transmitted packet counter, FSM state, lora_tx_request() state,
 * system_data.lora_busy, raw RegOpMode (0x01) of the SX1276 and its bit 7
 * (LongRangeMode: 1 = LoRa, 0 = FSK/OOK).
 *
 * Format:
 *   LORA n=<counter> st=<init|tx_wait|tx|tx_done|null|unknown> tx=<idle|pending|done|failed|?>
 *        busy=<0|1> op=0xNN LRM=<0|1> pkt=<packet|no tx>
 *
 * pkt shows lora_tx_packet() ONLY if the packet counter changed since the
 * previous call, i.e. a transmission really succeeded in between. Otherwise
 * "no tx" is printed so stale buffer contents do not look like a re-sent packet.
 *
 * Changes no radio state: reads FSM variables and performs one SPI register read.
 * Intended to be called before the radio is put to SLEEP (main.cpp does so).
 */
void lora_debug_dump(void);

#ifdef __cplusplus
}
#endif

#endif // LORA_FSM_H
