#include <RadioLib.h>
#include "lora_fsm.h"
#include "lora_sx1276.h"
#include "application/time/time.h"
#include "application/system_data/system_data.h"
#include "radiolib_stm32_hal/radiolib_stm32_hal.h"

static SX1276 *g_radio = nullptr;

typedef void (*lora_state_handler_t)(void);
lora_state_handler_t g_lora_state_handler = nullptr;

/* Explicit transmit request (watermark FSM and any other producer).
   The payload is borrowed, never copied - see lora_fsm.h. */
static const char *g_tx_payload = nullptr;
static volatile lora_tx_state_t g_tx_state = lora_tx_idle;

/* Payload sources merged into one packet. */
#define LORA_SRC_SYSTEM   (1 << 0)   /* system_data.data_string */
#define LORA_SRC_REQUEST  (1 << 1)   /* lora_tx_request() payload */

/* Packet assembled from every ready source, and the sources it carries. */
static char g_tx_current[LORA_TX_PACKET_LEN];
static uint8_t g_tx_sources = 0;

/* When only one source is ready, the other gets a short window to join
   the same packet instead of costing a second transmission. */
static timeout_t g_tx_join_timer;
static uint8_t g_tx_join_armed = 0;

volatile uint8_t lora_tx_done_flag = 0; // Flag to indicate that transmission is done, set in onTxDone() ISR


// static timeout_t g_timer; // unused - removed to avoid warning
int g_begin_state;

static void lora_state_init(void);
// static void lora_state_rx(void);
// static void lora_state_rx_wait(void);
// static void lora_state_check(void);
static void lora_state_tx_done(void);
static void lora_state_tx_wait(void);
static void lora_state_tx(void);

void onTxDone(void)
{
    lora_tx_done_flag = 1;
}

void lora_fsm_init(SX1276 *radio)
{
    g_radio = radio;
    g_tx_payload = nullptr;
    g_tx_current[0] = '\0';
    g_tx_sources = 0;
    g_tx_join_armed = 0;
    g_tx_state = lora_tx_idle;
    g_lora_state_handler = lora_state_init;
}

uint8_t lora_tx_request(const char *payload)
{
    if (payload == nullptr || g_tx_state == lora_tx_pending)
    {
        return 0;
    }

    g_tx_payload = payload;
    g_tx_state = lora_tx_pending;

    return 1;
}

const char *lora_tx_packet(void)
{
    return g_tx_current;
}

lora_tx_state_t lora_tx_status(void)
{
    return g_tx_state;
}

void lora_tx_clear(void)
{
    if (g_tx_state == lora_tx_done || g_tx_state == lora_tx_failed)
    {
        g_tx_state = lora_tx_idle;
    }
}

void lora_fsm_run(void)
{
    if (g_lora_state_handler != nullptr)
    {
        g_lora_state_handler();
    }
}

static void lora_state_init(void)
{
    int state = g_radio->begin(915.0, 125.0, 7, 5, 0x34, 17, 8, true);

    if (state == RADIOLIB_ERR_NONE)
    {

        g_radio->setDio0Action(onTxDone, 0);
        // g_lora_state_handler = lora_state_rx;
        g_lora_state_handler = lora_state_tx_wait; // new
    }
    else
    {
        g_lora_state_handler = nullptr;
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
    while (*s != '\0' && p < end)
    {
        *p++ = *s++;
    }

    return p;
}

/* Concatenate every ready source into g_tx_current. */
static void lora_tx_build(uint8_t sources)
{
    char *p = g_tx_current;
    char *end = g_tx_current + LORA_TX_PACKET_LEN - 1;

    if (sources & LORA_SRC_SYSTEM)
    {
        p = lora_tx_append(p, end, system_data.data_string);
    }

    if ((sources & LORA_SRC_SYSTEM) && (sources & LORA_SRC_REQUEST) && p < end)
    {
        *p++ = LORA_TX_SEPARATOR;
    }

    if (sources & LORA_SRC_REQUEST)
    {
        p = lora_tx_append(p, end, g_tx_payload);
    }

    *p = '\0';
}

static void lora_state_tx_wait(void)
{
    uint8_t sources = 0;

    if (system_data.lora_busy != 0 || hal.digitalRead(1) != 0)
    {
        return;
    }

    if (system_data.ready_data_creation_flag == 1)
    {
        sources |= LORA_SRC_SYSTEM;
    }

    if (g_tx_state == lora_tx_pending && g_tx_payload != nullptr)
    {
        sources |= LORA_SRC_REQUEST;
    }

    if (sources == 0)
    {
        g_tx_join_armed = 0;
        return;
    }

    /* Only one source so far: wait out the join window before giving up on
       merging, so both payloads normally travel in a single packet. */
    if (sources != (LORA_SRC_SYSTEM | LORA_SRC_REQUEST))
    {
        if (!g_tx_join_armed)
        {
            timer_set(&g_tx_join_timer, LORA_TX_JOIN_MS);
            g_tx_join_armed = 1;
            return;
        }

        if (!timer_wait(&g_tx_join_timer))
        {
            return;
        }
    }

    lora_tx_build(sources);

    g_tx_join_armed = 0;
    g_tx_sources = sources;
    g_lora_state_handler = lora_state_tx;
    system_data.lora_busy = 1;
}
static void lora_state_tx(void)
{

    lora_tx_done_flag = 0;

    int state = g_radio->startTransmit(g_tx_current);
    if (state == RADIOLIB_ERR_NONE)
    {

        g_lora_state_handler = lora_state_tx_done;
    }
    else
    {

        if (g_tx_sources & LORA_SRC_REQUEST)
        {
            g_tx_state = lora_tx_failed;
        }

        g_tx_sources = 0;
        system_data.lora_busy = 0;
        g_lora_state_handler = lora_state_tx_wait;
    }
}

static void lora_state_tx_done(void)
{
    if (lora_tx_done_flag)
    {

        (void)g_radio->finishTransmit();

        lora_tx_done_flag = 0;

        g_radio->standby();

        if (g_tx_sources & LORA_SRC_REQUEST)
        {
            g_tx_payload = nullptr;
            g_tx_state = lora_tx_done;
        }

        if (g_tx_sources & LORA_SRC_SYSTEM)
        {
            system_data.ready_data_creation_flag = 0;
            system_data.ready_sensors_flag = 0;
        }

        g_tx_sources = 0;
        system_data.lora_busy = 0;
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
