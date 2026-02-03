#ifndef LED_LIGHT_BULB_H
#define LED_LIGHT_BULB_H

#include <stdint.h>
/*
    * @file led_light_bulb.h
    * @brief Driver for LED Light Bulb using GPIOA on STM32F103xx microcontroller.
    * This module provides functions to initialize the LED light bulb
    * and control its state (ON/OFF) using specified GPIO pins.
    * One LED is assigned to a single sensor, and during measurement it illuminates
    to indicate the sensor is active.
*/

/**
 * @brief Initialize LED light bulb on specified GPIO pin.
 *   This function performs the following steps:
 *  1. Configure the specified GPIOA pin as an output for the LED light bulb.
 *  @param led_gpio The GPIOA pin number (0-15) connected to the LED light bulb.
 */ 
void led_light_init(uint8_t led_gpio);

/**
 * @brief Turn on the LED light bulb.
 * @param led_gpio The GPIOA pin number (0-15) connected to the LED light bulb.
 */
void led_ON(uint8_t led_gpio);

/**
 * @brief Turn off the LED light bulb.
 * @param led_gpio The GPIOA pin number (0-15) connected to the LED light bulb.
 */
void led_OFF(uint8_t led_gpio);

#endif
