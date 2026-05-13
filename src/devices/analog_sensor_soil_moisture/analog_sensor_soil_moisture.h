
#ifndef ANALOG_SENSORS_SOIL_MOISTURE_H
#define ANALOG_SENSORS_SOIL_MOISTURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void soil_sensor_init(void);

int16_t soil_sensor_read_average(void);
extern int16_t test_value;//test

#ifdef __cplusplus
}
#endif

#endif

