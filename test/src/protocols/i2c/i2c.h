#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @file i2c.h
     * @brief I2C communication protocol for STM32F103xx microcontroller.
     * This module provides functions to manage I2C communication.
     */

    /**
     * @brief Start the I2C communication.
     */
    void i2c_start(void);

    /**
     * @brief Configure the GPIO pins for I2C SDA and SCL.
     * @param pin The GPIO pin number to configure for I2C (e.g., 6 for SCL, 7 for SDA).
     */
    void i2c_SDA_SCL(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif // I2C_H
