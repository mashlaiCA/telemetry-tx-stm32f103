/**
 * @file  sensor_sht35.h
 * @brief Device-level interface of the SHT35 air temperature/humidity sensor (I2C address 0x44).
 * This file provides:
 * 1. I2C transactions used by the SHT35 FSM: start measurement, read result, soft reset.
 *    Each one returns the status of that exact transaction.
 * 2. Getters for the last converted temperature and humidity.
 * Conversion and CRC checking live in the low-level driver (drivers/sht35/sht35.h);
 * sequencing and error handling live in sht35_fsm.h.
 */

#ifndef SENSOR_SHT35_H
#define SENSOR_SHT35_H

#include "stdint.h"
#include "drivers/I2C/i2c_hw.h" // I2C_Status_t: every transaction returns its own status

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Sends the single-shot measurement command to the SHT35.
     * This function performs the following steps:
     * 1. Writes command 0x2400 (high repeatability, clock stretching disabled) to address 0x44.
     * The conversion then takes up to 15 ms; the FSM waits 20 ms before reading.
     * @return i2c_ok if the command was acknowledged;
     *         i2c_busy if the bus could not be acquired;
     *         i2c_error on NACK or timeout (the exact cause is in i2c_status_error).
     */
    I2C_Status_t I2C_Write_Sensor_SHT35(void); // Returns the status of this transaction, not the shared global one

    /**
     * @brief Reads the 6-byte measurement result into the driver buffer buf[].
     * This function performs the following steps:
     * 1. Reads T MSB, T LSB, T CRC, RH MSB, RH LSB, RH CRC from address 0x44.
     * @return i2c_ok if all 6 bytes were received;
     *         i2c_busy if the bus could not be acquired;
     *         i2c_error on NACK or timeout (the exact cause is in i2c_status_error).
     */
    I2C_Status_t I2C_Read_Sensor_SHT35(void); // Returns the status of this transaction, not the shared global one

    /**
     * @brief Returns the last converted air temperature.
     * The value is in whole degrees Celsius (fraction truncated). Negative
     * temperatures are clamped to 0 because the type is unsigned.
     * @return Temperature in degC, 0..125.
     */
    uint16_t temperatureSHT35(void);

    /**
     * @brief Returns the last converted relative humidity.
     * The value is in whole percent (fraction truncated).
     * @return Relative humidity in %RH, 0..100.
     */
    uint16_t humiditySHT35(void);

    /**
     * @brief Sends the soft reset command to the SHT35.
     * This function performs the following steps:
     * 1. Writes command 0x30A2 to address 0x44. The sensor needs up to 1.5 ms to restart.
     * @return i2c_ok if the command was acknowledged;
     *         i2c_busy if the bus could not be acquired;
     *         i2c_error on NACK or timeout.
     */
    I2C_Status_t I2C_Restart_Sensor_SHT35(void); // Returns the status of this transaction, not the shared global one

#ifdef __cplusplus
}
#endif

#endif
