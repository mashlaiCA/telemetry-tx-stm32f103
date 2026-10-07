/**
 * @file  board_pins.h
 * @brief Board-level pin initialization.
 * This file provides:
 * 1. pins_init() - GPIO clocks plus the SPI1 and LoRa control pins.
 * 2. polarity_init() - configures one GPIOA pin as a probe drive output.
 * Sensor-specific pins (ADC inputs, probe drives, I2C) are configured by their own modules.
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initializes the board pins needed before the peripherals.
     * This function performs the following steps:
     * 1. Enables the GPIOA and GPIOB clocks.
     * 2. Configures the SPI1 pins (PA5 SCK, PA6 MISO, PA7 MOSI) and PB12 (SX1276 NSS).
     * 3. Configures PB5 (SX1276 RST) and PB1 (SX1276 DIO0).
     */

    void pins_init(void);

    void polarity_init(uint8_t pin_1); // Configures PA<pin_1> (0..7) as push-pull output 2 MHz (gpio_A_polarity_init())

#ifdef __cplusplus
}
#endif

#endif
