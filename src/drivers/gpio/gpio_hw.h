#ifndef GPIO_H
#define GPIO_H

#include "stdint.h"

/**
 * @file gpioA.h
 * @brief Driver for GPIOA pins on STM32F103xx microcontroller.
 * This module provides functions to initialize GPIOA pins for analog input
 * and LED output, as well as functions to set and reset LED pins.
 */

/** @brief Initialize GPIOA peripheral clock.
 *  This function enables the GPIOA clock in the RCC APB2ENR register.
 */
void gpio_A_init(void);


/** @brief Initialize GPIOB peripheral clock.
 *  This function enables the GPIOB clock in the RCC APB2ENR register.
 */
void gpio_B_init(void);

/** @brief Initialize GPIOB pin for I2C SDA and SCL.
 *   This function performs the following steps:
 *  1. Clear the CNF and MODE bits for the specified GPIOB pin.
 *  2. Configure the specified GPIOB pin as an alternate function open-drain
 *     with a maximum speed of 50 MHz.
 *  @param pin The GPIOB pin number (0-7) to configure for I2C SDA or SCL.
 */
void gpio_B_init_I2C_SDA_SCL(uint8_t pin);


/** @brief Initialize GPIOA pin as analog input.
 *   This function performs the following steps:
 *  1. Enable the GPIOA clock in the RCC APB2ENR register.
 *  2. Configure the specified GPIOA pin as an analog input.
 *  @param pin The GPIOA pin number (0-7) to configure as analog input.
 */
void gpio_a_analog_input_init(uint8_t pin);

/** @brief Initialize GPIOA pin as LED output.
 *   This function performs the following steps:
 *  1. Clear the CNF and MODE bits for the specified GPIOA pin.
 *  2. Configure the specified GPIOA pin as a push-pull output.
 *  3. Set the GPIOA pin high to turn off the LED.
 *  @param pin The GPIOA pin number (0-7) to configure as LED output.
 */
void gpio_a_led_output_init(uint8_t pin);

/** @brief Set GPIOA pin low to turn on LED.
 *  This function sets the specified GPIOA pin low using the BRR register
 *  to turn on the connected LED light bulb.
 *  @param led_gpio The GPIOA pin number (0-7) connected to the LED.
 */
void gpio_a_set(uint8_t led_gpio);

/** @brief Set GPIOA pin high to turn off LED.
 *  This function sets the specified GPIOA pin high using the BSRR register
 *  to turn off the connected LED light bulb.
 *  @param led_gpio The GPIOA pin number (0-7) connected to the LED.
 */
void gpio_a_reset(uint8_t led_gpio);

#endif
