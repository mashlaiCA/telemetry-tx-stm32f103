
#ifndef ANALOG_SENSORS_MANAGER_H
#define ANALOG_SENSORS_MANAGER_H

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif

#define SENSOR_COUNT 4
//#define AVERAGE_COUNT 5

typedef struct
{
    uint8_t pin;

    uint16_t cal_dry;

    uint16_t cal_wet;

    uint16_t current_value;

    float previous_value;

    //uint16_t measurements[AVERAGE_COUNT];

   // uint8_t measurement_index;

    uint8_t enabled;

} analog_sensor_t;


void analog_sensors_init(void);

int8_t analog_sensor_add(uint8_t pin,
                         uint16_t cal_dry,
                         uint16_t cal_wet);

int16_t analog_sensor_read_average(uint8_t sensor_id);


#ifdef __cplusplus
}
#endif

#endif