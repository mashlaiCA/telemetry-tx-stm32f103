/**
 * @file  analog_sensor_soil_moisture_fsm.h
 * @brief Minimal FSM that publishes DATA_ANALOG_READY for system_data.
 * This file provides:
 * 1. analog_sensor_fsm_init() - starts the FSM in the wait state.
 * 2. analog_sensor_FSM_Run() - one FSM step, called from the main loop.
 * The FSM performs no measurement: whenever DATA_ANALOG_READY is cleared, it
 * sets it again on the next step, which lets system_data_run() read the leaf
 * sensor once per packet. (The former soil measurement is kept under #if 0 in the .c file.)
 */

#ifndef ANALOG_SENSOR_SOIL_MOISTURE_FSM_H
#define ANALOG_SENSOR_SOIL_MOISTURE_FSM_H
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*analog_sensor_state_handler_t)(void); // FSM state handler type



void analog_sensor_fsm_init(void); // Sets the initial state (wait)
void analog_sensor_FSM_Run(void);  // Executes the current state handler once

#ifdef __cplusplus
}
#endif

#endif

