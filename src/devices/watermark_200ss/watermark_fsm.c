#include "watermark_fsm.h"
#include "devices/lora_sx1276/lora_fsm.h"
#include "devices/ntc/ntc.h"
#include "drivers/timeout_hw/timeout_hw.h"
#include "drivers/acd1/acd1.h" // adc1_last_error - indicates a non-working ADC
#include "application/system_data/system_data.h" // DATA_WATERMARK_READY

/* Minimum interval between two probe excitations, counted from the START of a
   measurement. It never starts a measurement (that is done only by
   watermark_fsm_start_now()); it only delays one.
   In normal operation it has always expired by the next wakeup: the cycle is
   CYCLE_PERIOD_S = 30 s, and sleep time is added to sys_ms by sys_time_add_ms()
   in sleep_hw.c. It matters when cycles run back to back without sleep (e.g.
   the RTC alarm cannot be set and system_sleep_cycle() returns without
   sleeping): then it keeps the electrochemical probe from being excited far
   more often than designed, which would accelerate electrode wear. */
#define WM_MIN_INTERVAL_MS 5000   /* minimum between probe excitations (safety) */
#define WM_FSM_NTC_CH         1   /* ADC1 channel of the soil NTC */

static watermark_state_handler_t g_wm_state_handler = 0; // Current state; 0 = not initialized

/* Two independent timers. The minimum interval must be counted from the start
   of a measurement, while the rest timer is restarted after every sample; a
   single shared timer would lose the interval countdown on the first sample. */
static timeout_t g_wm_rest_timer;   /* depolarization pause between samples */
static timeout_t g_wm_floor_timer;  /* lower bound between probe excitations */

/* Latched measurement request. watermark_fsm_start_now() only sets it and
   watermark_state_idle() consumes it as soon as starting is allowed. The
   trigger arrives once per cycle; if it were dropped while starting is blocked
   (e.g. a lora_tx_request() payload still pending when the cycle ended on
   MEASURE_WINDOW_MS), this cycle would get no Watermark measurement. */
static uint8_t g_wm_start_req;

static watermark_data_t g_wm_data;              // Last completed measurement
static char             g_wm_packet[WM_PACKET_LEN]; // Debug text of g_wm_data
static uint8_t          g_wm_ready;             // 1 after the first completed measurement

static float    g_wm_temp_c;     // Soil temperature for this measurement, degC
static float    g_wm_r_sum;      // Sum of resistances of successful samples, Ohm
static uint8_t  g_wm_sample_idx; // Samples attempted in this measurement
static uint8_t  g_wm_taken;      // number of samples actually taken

static void watermark_state_idle(void);
static void watermark_state_temp(void);
static void watermark_state_sample(void);
static void watermark_state_rest(void);
static void watermark_state_evaluate(void);
static void watermark_state_send(void);

/* Initializes the sensor and parks the FSM in idle (see watermark_fsm.h). */
void watermark_fsm_init(void)
{
    watermark_init(); // PB0 analog, PB13/PB14 released

    g_wm_ready = 0;
    g_wm_temp_c = 0.0f;
    g_wm_r_sum = 0.0f;
    g_wm_sample_idx = 0;
    g_wm_taken = 0;
    g_wm_packet[0] = '\0';

    /* The minimum interval is armed with 0 ms, i.e. already expired, so the
       startup trigger (watermark_fsm_start_now() in main.cpp) measures
       immediately and the first packet after power-on carries Watermark data. */
    timeout_start(&g_wm_floor_timer, 0);
    g_wm_start_req = 0; // No request until the first trigger

    g_wm_state_handler = watermark_state_idle;
}

/* Runs the current state once (no-op before init). */
void watermark_fsm_run(void)
{
    if (g_wm_state_handler)
    {
        g_wm_state_handler();
    }
}

/* Measurement trigger, driven by the wakeup event instead of an own timer.
   The DS3231 alarm is the only time source of the cycle; this function is
   called from system_sleep_cycle() in main.cpp right after the peripherals are
   restored after Stop mode, so the measurement starts from the wakeup moment
   together with the other sensors and fits into the LORA_TX_JOIN_MS window.
   A latch is used instead of a direct transition because starting may be
   blocked at this moment (see watermark_state_idle()); the request then stays
   set until it can be served. Safe to call repeatedly. */
void watermark_fsm_start_now(void)
{
    g_wm_start_req = 1; // Served by watermark_state_idle()
}

/* Last completed measurement. */
const watermark_data_t *watermark_fsm_data(void)
{
    return &g_wm_data;
}

/* Debug text of the last completed measurement. */
const char *watermark_fsm_packet(void)
{
    return g_wm_packet;
}

/* 1 after the first completed measurement. */
uint8_t watermark_fsm_ready(void)
{
    return g_wm_ready;
}

/* cb getter for system_data (soil_moisture_10).
   Only reads g_wm_data.cb: does not start a measurement, does not touch GPIO/ADC,
   does not enter the watermark_sample() critical section. The value is updated
   only in watermark_state_evaluate() in the same main loop, so there is no race. */
float watermark_get_cb(void)
{
    return g_wm_data.cb;
}

/* Parked state: counts no time itself and waits for the trigger.
   Three conditions must hold to start a measurement:
   1) g_wm_start_req - the per-wakeup trigger; without it the FSM stays here
      however much time passes (the period is set only by the RTC).
   2) lora_tx_status() != lora_tx_pending - a payload borrowed by the LoRa FSM
      must not be rebuilt while queued. The trigger is NOT consumed while
      waiting, so the measurement starts as soon as the radio releases it.
   3) timeout_has_expired(&g_wm_floor_timer) - excitation rate guard
      (WM_MIN_INTERVAL_MS); already met at wakeup in normal operation. */
static void watermark_state_idle(void)
{
    if (!g_wm_start_req)
    {
        return; // No trigger yet
    }

    if (lora_tx_status() == lora_tx_pending)
    {
        return; // Packet still needed by LoRa FSM - buffer must not be touched
    }

    if (!timeout_has_expired(&g_wm_floor_timer))
    {
        return; // Too soon after the previous excitation
    }

    g_wm_start_req = 0; // Trigger consumed

    /* Clear the readiness of the previous measurement at the start of a new
       one. If the last cycle ended on MEASURE_WINDOW_MS (e.g. SHT35 did not
       finish), the bit could still be set, and after wakeup data_creation()
       would take the previous cb before this measurement completes. */
    system_data.ready_sensors_flag &= (uint8_t)~DATA_WATERMARK_READY;

    lora_tx_clear(); // Clear the result of the previous transmission (done/failed)

    /* The minimum interval is counted from the START of the measurement. */
    timeout_start(&g_wm_floor_timer, WM_MIN_INTERVAL_MS);

    g_wm_r_sum = 0.0f;   // New measurement: reset accumulators
    g_wm_sample_idx = 0;
    g_wm_taken = 0; // new cycle - successful sample counter from zero

    g_wm_state_handler = watermark_state_temp;
}

/* Soil temperature is needed for the resistance-to-centibar compensation. */
static void watermark_state_temp(void)
{
    g_wm_temp_c = temp_c_ntc(WM_FSM_NTC_CH); // NTC on ADC channel 1 (24 degC fallback on failure)

    /* ADC health check before the probe is excited.
       The NTC read above uses the same ADC1 with the probe unpowered, so it is
       a free test of the ADC. If it failed (e.g. adc1_wake() after Stop mode
       did not finish the calibration), the Watermark conversion would fail
       too: watermark_sample() would apply the forward excitation, hit the ADC
       timeout and exit before the reverse phase - a one-directional DC pulse
       on every sample of every cycle, slowly destroying the electrodes.
       With a dead ADC the probe is therefore not excited at all. r_ohm stays 0,
       which watermark_classify() reports as WM_OPEN - an explicit fault
       indication instead of a made-up value. The check repeats every cycle,
       so measurements resume automatically once the ADC works again. */
    if (adc1_last_error)
    {
        g_wm_taken = 0;
        g_wm_r_sum = 0.0f;
        g_wm_state_handler = watermark_state_evaluate; // Skip sampling, report OPEN
        return;
    }

    g_wm_state_handler = watermark_state_sample;
}

/* One forward/reverse excitation. Short enough to run inside a single step. */
static void watermark_state_sample(void)
{
    uint16_t fwd = 0, rev = 0; // Raw codes of this sample

    /* Only successful samples are accumulated: a failed sample returns no
       codes, and watermark_resistance(0, 0) would give 999999 Ohm, pulling
       the average towards "open circuit" with a healthy sensor.
       g_wm_sample_idx is incremented in any case, so a persistent ADC failure
       cannot keep the FSM in this state forever. */
    uint8_t ok = watermark_sample(&fwd, &rev);

    if (ok)
    {
        g_wm_data.raw_fwd = fwd;
        g_wm_data.raw_rev = rev;
        g_wm_r_sum += watermark_resistance(fwd, rev); // Accumulate resistance, Ohm
        g_wm_taken++;
    }

    g_wm_sample_idx++; // Attempt counted even if it failed

    timeout_start(&g_wm_rest_timer, WM_REST_MS); // Depolarization pause (own timer, see declaration)
    g_wm_state_handler = watermark_state_rest;
}

/* Let the probe depolarize between excitations. */
static void watermark_state_rest(void)
{
    if (!timeout_has_expired(&g_wm_rest_timer)) // WM_REST_MS not elapsed yet
    {
        return;
    }

    if (g_wm_sample_idx < WM_SAMPLES)
    {
        g_wm_state_handler = watermark_state_sample; // Next sample
    }
    else
    {
        g_wm_state_handler = watermark_state_evaluate; // All samples done
    }
}

/* Averages, classifies and formats the measurement. */
static void watermark_state_evaluate(void)
{
    /* Divide by the number of SUCCESSFUL samples, not by WM_SAMPLES. With none,
       r_ohm = 0, which watermark_classify() reports as WM_OPEN - an explicit
       fault indication rather than a silently understated resistance. */
    g_wm_data.r_ohm = g_wm_taken ? (g_wm_r_sum / (float)g_wm_taken) : 0.0f;

    watermark_classify(&g_wm_data, g_wm_temp_c);                   // Status + cb
    watermark_pack_string(&g_wm_data, g_wm_temp_c, g_wm_packet);   // Debug text

    g_wm_ready = 1; // At least one measurement available

    g_wm_state_handler = watermark_state_send;
}

/* Publishes the result to system_data. Nothing is sent over LoRa from here:
   only system_data.data_string is transmitted, and it already carries this cb
   as its first field (soil_moisture_10, see data_creation()). g_wm_packet stays
   available through watermark_fsm_packet() for debugging. */
static void watermark_state_send(void)
{
    system_data.ready_sensors_flag |= DATA_WATERMARK_READY; // This cycle's cb is ready

    g_wm_state_handler = watermark_state_idle; // Park until the next trigger
}
