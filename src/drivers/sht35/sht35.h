#ifndef SHT35_H
#define SHT35_H

#include <stdint.h>

/**
 * @file sht35.h
 * @brief SHT35 sensor driver header file.
 * This module provides functions to interface with the SHT35 temperature and humidity sensor.
 */

typedef enum {
    sht35_ok = 0,
    sht35_error_crc,
    sht35_calc_out_of_range,
    sht35_error_calculation,
} SHT35_Status_t;

extern SHT35_Status_t sht35_status; // Global variable to hold the last SHT35 error status
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


SHT35_Status_t SHT35_CRC_Check(void);


SHT35_Status_t SHT35_Calculate(void);

#endif 