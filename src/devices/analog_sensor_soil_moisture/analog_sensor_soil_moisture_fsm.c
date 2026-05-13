#include "analog_sensor_soil_moisture_fsm.h"
#include "analog_sensor_soil_moisture.h"
#include "application/system_data/system_data.h"

typedef void (*analog_sensor_state_handler_t)(void);

static analog_sensor_state_handler_t
g_analog_sensor_state_handler = 0;

static void analog_sensor_state_wait(void);

static void analog_sensor_state_read(void);

void analog_sensor_fsm_init(void)
{
    g_analog_sensor_state_handler =
        analog_sensor_state_wait;
}

void analog_sensor_FSM_Run(void)
{
    if(g_analog_sensor_state_handler)
    {
        g_analog_sensor_state_handler();
    }
}

static void analog_sensor_state_wait(void)
{
    if(!(system_data.ready_sensors_flag &
        DATA_ANALOG_READY))
    {
        g_analog_sensor_state_handler =
            analog_sensor_state_read;
    }
}

static void analog_sensor_state_read(void)
{
    int16_t value =
        soil_sensor_read_average();

    if(value > 0)
    {
        system_data.soil_moisture_10 = (uint16_t)value;

        system_data.ready_sensors_flag |=
            DATA_ANALOG_READY;
    }

    g_analog_sensor_state_handler =
        analog_sensor_state_wait;
}