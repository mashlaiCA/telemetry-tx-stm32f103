#ifndef ANALOG_SENSOR_SOIL_MOISTURE_H
#define ANALOG_SENSOR_SOIL_MOISTURE_H

#include <stdint.h>

/**
 * @file analog_sensor_soil_moisture.h
 * @brief Driver for Analog Soil Moisture Sensor using ADC1 on STM32F103xx microcontroller.
 * This module provides functions to initialize the soil moisture sensor
 * and read soil moisture values from specified GPIO pins configured as ADC1 channels.
 */

/** @brief Initialize soil moisture sensor on specified GPIO pin.
 *   This function performs the following steps:
 *  1. Initialize the ADC1 peripheral.
 *  2. Configure the specified GPIOA pin as an analog input.
 *  3. Perform an initial read to stabilize the ADC.
 *  @param sensor_gpio The GPIOA pin number (0-7) connected to the soil moisture sensor.
 */
void analog_sensor_soil_moisture_init(uint8_t sensor_gpio);

/** @brief Read soil moisture value from specified GPIO pin.
 *   This function performs the following steps:
 *  1. Read the analog value from the specified GPIOA pin using ADC1.
 *  @param sensor_gpio The GPIOA pin number (0-7) connected to the soil moisture sensor.
 *  @return The 12-bit soil moisture value read from the sensor.
 */
uint16_t analog_sensor_soil_moisture_read(uint8_t sensor_gpio);

#endif
