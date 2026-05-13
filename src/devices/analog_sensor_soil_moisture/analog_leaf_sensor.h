#ifndef ANALOG_LEAF_SENSOR_H
#define ANALOG_LEAF_SENSOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void leaf_sensor_init(void);

int16_t leaf_sensor_read_average(void);

#ifdef __cplusplus
}
#endif

#endif