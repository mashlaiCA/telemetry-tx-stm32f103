#ifndef SHT35_H
#define SHT35_H

#include "stdint.h"
#include "sht35_status.h"


/**
 * @file sht35.h
 * @brief SHT35 sensor driver header file.
 * This module provides functions to interface with the SHT35 temperature and humidity sensor.
 */


extern uint16_t temperature; // Temperature in hundredths of degrees Celsius
extern uint16_t humidity; // Humidity in hundredths of percent
extern uint8_t buf[6]; // Data buffer

/**
 * @brief Calculate CRC8 checksum for SHT35 data.
 * This function computes the CRC8 checksum for the given data array using the polynomial
 * x^8 + x^5 + x^4 + 1, which is used by the SHT35 sensor for data integrity verification.
 *
 * @param data Pointer to the data array.
 * @param len Length of the data array.
 * @return Calculated CRC8 checksum.
 */
uint8_t SHT35_CRC8(uint8_t *data, uint8_t len);


/**
 * @brief Check the CRC of the received SHT35 data.
 * This function verifies the integrity of the temperature and humidity data received from
 * the SHT35 sensor by checking their CRC values. If the CRC check passes, it extracts
 * the raw temperature and humidity values from the data buffer.
 *
 * @return SHT35_Status_t Status of the CRC check operation.
 */
SHT35_Status_t SHT35_CRC_Check(void);

/**
 * @brief Calculate temperature and humidity from raw SHT35 data.
 * This function converts the raw temperature and humidity values obtained from the SHT35
 * sensor into human-readable formats (Celsius and percent). It also checks if the
 * calculated values are within valid ranges.
 *
 * @return SHT35_Status_t Status of the calculation operation.
 */
SHT35_Status_t SHT35_Calculate(void);

#endif 