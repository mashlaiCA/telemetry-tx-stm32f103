#ifndef ANALOG_SENSOR_SOIL_MOISTURE_FSM_H
#define ANALOG_SENSOR_SOIL_MOISTURE_FSM_H
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint16_t sensor_soil_10;
    uint16_t sensor_soil_20;

} system_analog_sensors_data_t;

typedef void (*analog_sensor_state_handler_t)(void);

extern system_analog_sensors_data_t system_analog_sensors_data;


void analog_sensor_fsm_init(void);
void analog_sensor_FSM_Run(void);
void analog_sensor_state_wait_read(void);
void analog_sensor_state_read(void);


#ifdef __cplusplus
}
#endif

#endif

