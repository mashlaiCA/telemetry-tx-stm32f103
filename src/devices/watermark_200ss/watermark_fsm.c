#include "watermark_fsm.h"
#include "devices/lora_sx1276/lora_fsm.h"
#include "devices/ntc/ntc.h"
#include "drivers/timeout_hw/timeout_hw.h"

#define WM_FSM_PERIOD_MS   5000   /* measurement cycle */
#define WM_FSM_NTC_CH         1   /* ADC1 channel of the soil NTC */

static watermark_state_handler_t g_wm_state_handler = 0;

static timeout_t g_wm_timer;

static watermark_data_t g_wm_data;
static char             g_wm_packet[WM_PACKET_LEN];
static uint8_t          g_wm_ready;

static float    g_wm_temp_c;
static float    g_wm_r_sum;
static uint8_t  g_wm_sample_idx;

static void watermark_state_idle(void);
static void watermark_state_temp(void);
static void watermark_state_sample(void);
static void watermark_state_rest(void);
static void watermark_state_evaluate(void);
static void watermark_state_send(void);
static void watermark_state_tx_wait(void);

void watermark_fsm_init(void)
{
    watermark_init();

    g_wm_ready = 0;
    g_wm_temp_c = 0.0f;
    g_wm_r_sum = 0.0f;
    g_wm_sample_idx = 0;
    g_wm_packet[0] = '\0';

    timeout_start(&g_wm_timer, WM_FSM_PERIOD_MS);
    g_wm_state_handler = watermark_state_idle;
}

void watermark_fsm_run(void)
{
    if (g_wm_state_handler)
    {
        g_wm_state_handler();
    }
}

const watermark_data_t *watermark_fsm_data(void)
{
    return &g_wm_data;
}

const char *watermark_fsm_packet(void)
{
    return g_wm_packet;
}

uint8_t watermark_fsm_ready(void)
{
    return g_wm_ready;
}

/* Wait out the measurement period, then start a fresh accumulation. */
static void watermark_state_idle(void)
{
    if (!timeout_has_expired(&g_wm_timer))
    {
        return;
    }

    g_wm_r_sum = 0.0f;
    g_wm_sample_idx = 0;

    g_wm_state_handler = watermark_state_temp;
}

/* Soil temperature is needed for the resistance-to-centibar compensation. */
static void watermark_state_temp(void)
{
    g_wm_temp_c = temp_c_ntc(WM_FSM_NTC_CH);

    g_wm_state_handler = watermark_state_sample;
}

/* One forward/reverse excitation. Short enough to run inside a single step. */
static void watermark_state_sample(void)
{
    uint16_t fwd = 0, rev = 0;

    watermark_sample(&fwd, &rev);

    g_wm_data.raw_fwd = fwd;
    g_wm_data.raw_rev = rev;
    g_wm_r_sum += watermark_resistance(fwd, rev);
    g_wm_sample_idx++;

    timeout_start(&g_wm_timer, WM_REST_MS);
    g_wm_state_handler = watermark_state_rest;
}

/* Let the probe depolarize between excitations. */
static void watermark_state_rest(void)
{
    if (!timeout_has_expired(&g_wm_timer))
    {
        return;
    }

    if (g_wm_sample_idx < WM_SAMPLES)
    {
        g_wm_state_handler = watermark_state_sample;
    }
    else
    {
        g_wm_state_handler = watermark_state_evaluate;
    }
}

static void watermark_state_evaluate(void)
{
    g_wm_data.r_ohm = g_wm_r_sum / WM_SAMPLES;

    watermark_classify(&g_wm_data, g_wm_temp_c);
    watermark_pack_string(&g_wm_data, g_wm_temp_c, g_wm_packet);

    g_wm_ready = 1;

    g_wm_state_handler = watermark_state_send;
}

/* Transmission is done by the LoRa FSM only; here we just queue the packet. */
static void watermark_state_send(void)
{
    if (!lora_tx_request(g_wm_packet))
    {
        /* Another payload is still queued, keep the packet and retry. */
        return;
    }

    g_wm_state_handler = watermark_state_tx_wait;
}

/* Hold the packet buffer stable until the LoRa FSM is done with it. */
static void watermark_state_tx_wait(void)
{
    lora_tx_state_t state = lora_tx_status();

    if (state == lora_tx_pending)
    {
        return;
    }

    lora_tx_clear();

    timeout_start(&g_wm_timer, WM_FSM_PERIOD_MS);
    g_wm_state_handler = watermark_state_idle;
}
