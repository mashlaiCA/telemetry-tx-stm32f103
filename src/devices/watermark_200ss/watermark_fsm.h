/**
 * @file  watermark_fsm.h
 * @brief Non-blocking finite state machine for the Watermark 200SS soil
 * tension sensor.
 * This file provides:
 * 1. watermark_fsm_init() / watermark_fsm_run() - FSM setup and one step per main-loop call.
 * 2. watermark_fsm_start_now() - per-wakeup measurement trigger.
 * 3. Accessors for the last result: watermark_fsm_data(), watermark_fsm_packet(),
 *    watermark_fsm_ready(), watermark_get_cb().
 *
 * watermark_read() samples the probe with blocking delays between the
 * excitation bursts. This FSM performs the same measurement one step per
 * call, so the main loop keeps servicing the other FSMs while the probe
 * rests between samples.
 * States: idle -> temp (NTC soil temperature, ADC health check) -> sample/rest
 * x WM_SAMPLES -> evaluate -> send -> idle.
 *
 * The FSM transmits nothing itself: after a measurement it sets
 * DATA_WATERMARK_READY in system_data, and cb goes into
 * system_data.data_string (field soil_moisture_10) via watermark_get_cb().
 */

#ifndef WATERMARK_FSM_H
#define WATERMARK_FSM_H

#include <stdint.h>
#include "watermark_200ss.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*watermark_state_handler_t)(void); // FSM state handler type

/**
 * @brief Initializes the sensor and the FSM.
 * Calls watermark_init(), so the caller does not have to. The FSM starts in
 * idle and waits for watermark_fsm_start_now(); the minimum-interval guard is
 * already expired, so the first trigger starts a measurement immediately.
 */
void watermark_fsm_init(void);

/**
 * @brief Advance the FSM by one step. Call from the main loop.
 */
void watermark_fsm_run(void);

/**
 * @brief Requests one measurement (the FSM has no periodic timer of its own).
 *
 * Called from system_sleep_cycle() in main.cpp right after
 * system_restore_after_stop() (i.e. after every wakeup) and once at startup.
 * The cycle period is therefore set only by the DS3231 alarm; a second,
 * independent time base in the FSM would drift against it and make the
 * active phase alternate between short and long cycles.
 *
 * The request is LATCHED, not executed immediately: while a lora_tx_request()
 * payload is pending (lora_tx_status() == lora_tx_pending) or the minimum
 * interval between excitations has not elapsed, the measurement waits, but
 * the request is not lost - it is served on the first step when allowed.
 * Safe to call repeatedly.
 */
void watermark_fsm_start_now(void);

/**
 * @brief Last completed measurement.
 * Contents are only meaningful once watermark_fsm_ready() has returned 1.
 */
const watermark_data_t *watermark_fsm_data(void);

/**
 * @brief Debug text of the last completed measurement ("cb=..,R=..,T=..,st=..,f=..,r=..").
 * Printed to UART by main.cpp; not transmitted over LoRa.
 */
const char *watermark_fsm_packet(void);

/**
 * @brief 1 once a first measurement has been completed and packed (never cleared afterwards).
 */
uint8_t watermark_fsm_ready(void);

/**
 * @brief cb of the last completed measurement (g_wm_data.cb).
 * Read-only: no measurement, no GPIO/ADC, no critical section.
 * Source of soil_moisture_10 in system_data. Freshness of the value for the current
 * cycle is guaranteed by the DATA_WATERMARK_READY bit, not by the getter itself.
 * @return 0..239 cb, WM_CB_SHORT (240) or WM_CB_OPEN (255); 0 before the first measurement.
 */
float watermark_get_cb(void);

#ifdef __cplusplus
}
#endif

#endif // WATERMARK_FSM_H
