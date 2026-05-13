#include "analog_sensor_soil_moisture.h"
#include "analog_sensors_manager.h"

static int8_t soil_sensor_id = -1;

void soil_sensor_init(void)
{
    //analog_sensors_init();

    soil_sensor_id =
        analog_sensor_add(3, 4047, 1350);
}

int16_t soil_sensor_read_average(void)
{
    return
        analog_sensor_read_average(
            soil_sensor_id
        );
}