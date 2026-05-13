#include "analog_leaf_sensor.h"
#include "analog_sensors_manager.h"

static int8_t leaf_sensor_id = -1;

void leaf_sensor_init(void)
{
   // analog_sensors_init();

    leaf_sensor_id =
        analog_sensor_add(4, 4090, 1);
}

int16_t leaf_sensor_read_average(void)
{
    return
        analog_sensor_read_average(
            leaf_sensor_id
        );
}