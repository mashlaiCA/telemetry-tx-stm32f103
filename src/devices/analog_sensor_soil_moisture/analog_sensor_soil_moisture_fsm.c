#include "analog_sensor_soil_moisture_fsm.h"
#include "analog_sensor_soil_moisture.h"
#include "application/system_data/system_data.h"

typedef void (*analog_sensor_state_handler_t)(void);

static analog_sensor_state_handler_t
g_analog_sensor_state_handler = 0; // Current state; 0 = FSM not initialized

static void analog_sensor_state_wait(void);

static void analog_sensor_state_read(void);

/* Starts the FSM in the wait state. */
void analog_sensor_fsm_init(void)
{
    g_analog_sensor_state_handler =
        analog_sensor_state_wait;
}

/* Runs the current state once (no-op before init). */
void analog_sensor_FSM_Run(void)
{
    if(g_analog_sensor_state_handler)
    {
        g_analog_sensor_state_handler();
    }
}

/* Waits until system_data has consumed the flag (DATA_ANALOG_READY cleared). */
static void analog_sensor_state_wait(void)
{
    if(!(system_data.ready_sensors_flag &
        DATA_ANALOG_READY)) // Flag consumed: a new packet cycle has started
    {
        g_analog_sensor_state_handler =
            analog_sensor_state_read;
    }
}

/* Publishes DATA_ANALOG_READY; the leaf sensor itself is read by
   system_data_run() when it sees the flag. No measurement is done here. */
static void analog_sensor_state_read(void)
{
    system_data.ready_sensors_flag |= DATA_ANALOG_READY; // Leaf sensor may be read

    g_analog_sensor_state_handler =
        analog_sensor_state_wait;
}

#if 0
static void analog_sensor_state_read(void)
{
    int16_t value =
        soil_sensor_read_average();
    if (value >= 0)
    {
        system_data.soil_moisture_10 = (uint16_t)value;

        system_data.ready_sensors_flag |=
            DATA_ANALOG_READY;
    }

    g_analog_sensor_state_handler =
        analog_sensor_state_wait;
}
#endif