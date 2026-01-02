#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stdint.h"

/**
 * @file board_pins.c
 * @brief Board pins initialization for STM32F103xx microcontroller.
 * This module provides a function to initialize the GPIOA peripheral
 */

/**
 * @brief Initialize board pins.
 * This function initializes the GPIOA peripheral for use with the soil moisture sensor.
 */

void pins_init(void);

#endif
