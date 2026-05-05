#include "analog_sensor_soil_moisture_fsm.h"
#include "analog_sensor_soil_moisture.h"
#include "application/system_data/system_data.h"
#include <string.h>


#define ANALOG_SENSOR_COUNT 1

typedef struct
{
    uint8_t sensor_gpio; // GPIO pin number for the soil moisture sensor
    uint16_t value_sensor;      // Latest soil moisture value read from the sensor
} analog_sensor_t;

typedef void (*analog_sensor_state_handler_t)(void);

analog_sensor_state_handler_t g_analog_sensor_state_handler = NULL;

system_analog_sensors_data_t system_analog_sensors_data = {0}; 

void analog_sensor_fsm_init(void);
void analog_sensor_FSM_Run(void);
void analog_sensor_state_wait_read(void);
void analog_sensor_state_read(void);

analog_sensor_t sensors[ANALOG_SENSOR_COUNT] = {
    {.sensor_gpio = 3,
     .value_sensor = 0}
};

uint8_t current_sensor_index = 0;


void analog_sensor_fsm_init(void)
{
    g_analog_sensor_state_handler = analog_sensor_state_wait_read; 
}


void analog_sensor_FSM_Run(void)
{
    if (g_analog_sensor_state_handler != NULL)
    {
        g_analog_sensor_state_handler(); 
    }
}

void analog_sensor_state_wait_read(void)
{
    if (!(system_data.ready_sensors_flag & DATA_ANALOG_READY))
    {
        g_analog_sensor_state_handler = analog_sensor_state_read; 
    }
}

void analog_sensor_state_read(void){
    sensors[current_sensor_index].value_sensor = analog_sensor_soil_moisture_read(sensors[current_sensor_index].sensor_gpio);
    current_sensor_index++;

    if(current_sensor_index >= ANALOG_SENSOR_COUNT){
        current_sensor_index = 0;

      system_analog_sensors_data.sensor_soil_10 = sensors[0].value_sensor;

        system_data.ready_sensors_flag |= DATA_ANALOG_READY;

        g_analog_sensor_state_handler = analog_sensor_state_wait_read;
    }
}


