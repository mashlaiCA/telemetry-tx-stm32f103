/**
 * @file  i2c.h
 * @brief Protocol-level wrapper over the I2C1 driver used by the application.
 * This file provides:
 * 1. i2c_start() - I2C1 peripheral initialization.
 * 2. i2c_SDA_SCL() - GPIO setup of one I2C pin.
 * 3. i2c_recover() - bus recovery for the application (e.g. after RTC errors).
 */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initializes the I2C1 peripheral (100 kHz standard mode).
     * This function performs the following steps:
     * 1. Calls I2C1_Init().
     * The SCL/SDA pins must be configured with i2c_SDA_SCL() first.
     */
    void i2c_start(void);

    /**
     * @brief Configures one GPIOB pin as I2C alternate function open-drain, 50 MHz.
     * @param pin GPIOB pin number, 0..7 (6 = SCL, 7 = SDA for I2C1).
     */
    void i2c_SDA_SCL(uint8_t pin);

    /**
     * @brief Releases a bus that a slave is holding low and re-initializes I2C1.
     * Exposed at the protocol level so the application can unstick the shared
     * SHT35/DS3231 bus after a failed RTC access without calling the driver directly.
     * This function performs the following steps:
     * 1. Calls i2c_bus_recover() (9 SCL clocks, START + STOP, SWRST, re-init).
     * @return 1 if the bus is usable afterwards, 0 if SDA or SCL is still stuck low.
     */
    uint8_t i2c_recover(void);

#ifdef __cplusplus
}
#endif

#endif // I2C_H
