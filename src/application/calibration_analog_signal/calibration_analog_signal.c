#include "calibration_analog_signal.h"
#include "drivers/acd1/acd1.h"
#include "devices/analog_sensor_soil_moisture/analog_leaf_sensor.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"


uint16_t analog_signal_leaf_sensor(void)
{
   return adc1_read(4);
}