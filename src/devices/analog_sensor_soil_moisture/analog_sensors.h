#ifndef ANALOG_SENSORS_H
#define ANALOG_SENSORS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void analog_sensor_init(uint8_t sensor_gpio);

void analog_sensors_init(void);


#ifdef __cplusplus
}
#endif

#endif // ANALOG_SENSORS_H