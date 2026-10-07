/**
 * @file  analog_sensor_soil_moisture.h
 * @brief Analog soil moisture sensor on ADC1 channel 8 (PB0).
 * This file provides:
 * 1. soil_sensor_init() - registers the sensor in the analog sensor manager.
 * 2. soil_sensor_read_average() - smoothed moisture in percent.
 * The soil moisture field of the packet now comes from the Watermark sensor;
 * this sensor is registered at startup but its reading is not used in the
 * current data path (sensor_update_soil_moisture() is not called).
 */

#ifndef ANALOG_SENSORS_SOIL_MOISTURE_H
#define ANALOG_SENSORS_SOIL_MOISTURE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void soil_sensor_init(void); // Registers the sensor: ADC channel 8, dry = 4047, wet = 1350 (raw codes)

int16_t soil_sensor_read_average(void); // Smoothed moisture 0..100 %, or -1 if the sensor is not registered


#ifdef __cplusplus
}
#endif

#endif

