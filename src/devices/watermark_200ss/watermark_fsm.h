#ifndef WATERMARK_FSM_H
#define WATERMARK_FSM_H

#include <stdint.h>
#include "watermark_200ss.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file watermark_fsm.h
 * @brief Non-blocking finite state machine for the Watermark 200SS soil
 * tension sensor.
 *
 * watermark_read() samples the probe with blocking delays between the
 * excitation bursts. This FSM performs the same measurement one step per
 * call, so the main loop keeps servicing the other FSMs while the probe
 * rests between samples.
 *
 * The measured packet is handed to the LoRa FSM through lora_tx_request();
 * transmission itself is done only there.
 */

typedef void (*watermark_state_handler_t)(void);

/**
 * @brief Initialize the sensor and arm the first measurement cycle.
 * Calls watermark_init(), so the caller does not have to.
 */
void watermark_fsm_init(void);

/**
 * @brief Advance the FSM by one step. Call from the main loop.
 */
void watermark_fsm_run(void);

/**
 * @brief Last completed measurement.
 * Contents are only meaningful once watermark_fsm_ready() has returned 1.
 */
const watermark_data_t *watermark_fsm_data(void);

/**
 * @brief Packet built from the last completed measurement, as handed to LoRa.
 */
const char *watermark_fsm_packet(void);

/**
 * @brief 1 once a first measurement has been completed and packed.
 */
uint8_t watermark_fsm_ready(void);

#ifdef __cplusplus
}
#endif

#endif // WATERMARK_FSM_H
