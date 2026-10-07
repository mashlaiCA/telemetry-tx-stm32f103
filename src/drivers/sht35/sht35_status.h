/**
 * @file  sht35_status.h
 * @brief Status codes of the SHT35 data processing (CRC check and conversion).
 * This file provides:
 * 1. The SHT35_Status_t enumeration.
 * 2. The global sht35_status holding the result of the last check/conversion.
 * 3. get_last_sht35_error() to read it.
 * I2C transfer errors are reported separately through I2C_Status_t.
 */

#ifndef SHT35_STATUS_H
#define SHT35_STATUS_H


/** @brief Result of SHT35_CRC_Check() / SHT35_Calculate(). */
typedef enum {
    sht35_ok = 0, // Last check/conversion succeeded
    sht35_error_crc, // CRC of the temperature or humidity word did not match
    sht35_calc_out_of_range, // Converted temperature or humidity outside the accepted range
    sht35_error_calculation, // General conversion error (currently not produced by the code)

    sht35_count // Number of status codes
} SHT35_Status_t; // SHT35 processing status

extern SHT35_Status_t sht35_status;  // Result of the last SHT35_CRC_Check() / SHT35_Calculate()

/**
 * @brief Returns the result of the last SHT35 CRC check or conversion.
 * @return The current value of sht35_status (see SHT35_Status_t).
 */
SHT35_Status_t get_last_sht35_error(void);

#endif