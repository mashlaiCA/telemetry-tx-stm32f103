/**
 * @file  analog_sensors_manager.h
 * @brief Registry of simple analog sensors read directly by ADC1 with two-point calibration.
 * This file provides:
 * 1. analog_sensor_t - configuration and filter state of one sensor.
 * 2. analog_sensors_init() - ADC1 initialization.
 * 3. analog_sensor_add() - registers a sensor in a free slot (up to SENSOR_COUNT).
 * 4. analog_sensor_read_average() - one reading scaled to 0..100 % and smoothed.
 */

#ifndef ANALOG_SENSORS_MANAGER_H
#define ANALOG_SENSORS_MANAGER_H

#include <stdint.h>


#ifdef __cplusplus
extern "C"
{
#endif

#define SENSOR_COUNT 4 // Maximum number of registered sensors
//#define AVERAGE_COUNT 5

/** @brief One registered analog sensor. */
typedef struct
{
    uint8_t pin; // ADC1 channel; also passed to gpio_a_analog_input_init() (configured only for 0..7)

    uint16_t cal_dry; // Raw ADC code that maps to 0 %

    uint16_t cal_wet; // Raw ADC code that maps to 100 %

    uint16_t current_value; // Last smoothed value, % (0..100)

    float previous_value; // Filter state, %; -1 = no reading yet

    //uint16_t measurements[AVERAGE_COUNT];

   // uint8_t measurement_index;

    uint8_t enabled; // 1 = slot in use

} analog_sensor_t;


void analog_sensors_init(void); // Initializes ADC1 (adc1_init())

/**
 * @brief Registers an analog sensor in the first free slot.
 * This function performs the following steps:
 * 1. Stores the channel and calibration points, resets the filter.
 * 2. Configures PA<pin> as analog input (only if pin is 0..7).
 * 3. Performs 5 dummy conversions on the channel.
 * @param pin     ADC1 channel number.
 * @param cal_dry Raw code for 0 %.
 * @param cal_wet Raw code for 100 %.
 * @return Slot index 0..SENSOR_COUNT-1, or -1 if all slots are used.
 */
int8_t analog_sensor_add(uint8_t pin,
                         uint16_t cal_dry,
                         uint16_t cal_wet);

/**
 * @brief Reads a sensor once and returns the smoothed percentage.
 * This function performs the following steps:
 * 1. One ADC conversion.
 * 2. Linear scaling: (raw - cal_dry) / (cal_wet - cal_dry) * 100, clamped to 0..100.
 * 3. Exponential smoothing: 0.2 * previous + 0.8 * new (the first reading is taken as is).
 * Despite the name, no averaging over several samples is done.
 * @param sensor_id Index returned by analog_sensor_add().
 * @return Smoothed value in %, 0..100;
 *         -1 if sensor_id is out of range, the slot is unused, or cal_dry == cal_wet.
 */
int16_t analog_sensor_read_average(uint8_t sensor_id);


#ifdef __cplusplus
}
#endif

#endif