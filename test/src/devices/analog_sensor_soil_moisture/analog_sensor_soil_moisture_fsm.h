#ifndef ANALOG_SENSOR_SOIL_MOISTURE_FSM_H
#define ANALOG_SENSOR_SOIL_MOISTURE_FSM_H
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*analog_sensor_state_handler_t)(void);



void analog_sensor_fsm_init(void);
void analog_sensor_FSM_Run(void);

#ifdef __cplusplus
}
#endif

#endif

