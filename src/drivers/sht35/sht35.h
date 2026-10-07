/**
 * @file  sht35.h
 * @brief Low-level data processing for the SHT35 sensor: CRC check and conversion.
 * This file provides:
 * 1. The shared receive buffer and the last converted temperature/humidity.
 * 2. SHT35_CRC8() - the Sensirion CRC-8 used by the sensor.
 * 3. SHT35_CRC_Check() - verifies both CRC bytes and extracts the raw words.
 * 4. SHT35_Calculate() - converts raw words to degC / %RH with range checks.
 * Both check/convert functions also store their result in sht35_status
 * (read with get_last_sht35_error()).
 */

#ifndef SHT35_H
#define SHT35_H

#include "stdint.h"
#include "sht35_status.h"


extern uint16_t temperature; // Last valid temperature, whole degC (negative values clamped to 0)
extern uint16_t humidity; // Last valid relative humidity, whole %RH
extern uint8_t buf[6]; // Raw I2C data: T MSB, T LSB, T CRC, RH MSB, RH LSB, RH CRC

/**
 * @brief Computes the SHT3x CRC-8 over a data block.
 * This function performs the following steps:
 * 1. Starts from the initial value 0xFF.
 * 2. Processes each byte MSB first with polynomial 0x31 (x^8 + x^5 + x^4 + 1), no final XOR.
 * @param data Pointer to the data bytes.
 * @param len  Number of bytes (2 for one SHT35 word).
 * @return The computed CRC-8 value.
 */
uint8_t SHT35_CRC8(uint8_t *data, uint8_t len);


/**
 * @brief Verifies the CRC of both received words and extracts the raw values.
 * This function performs the following steps:
 * 1. Compares CRC8(buf[0..1]) with buf[2] (temperature word).
 * 2. Compares CRC8(buf[3..4]) with buf[5] (humidity word).
 * 3. On success stores the raw temperature and humidity words for SHT35_Calculate().
 * 4. Stores the result in sht35_status.
 * @return sht35_ok if both CRCs match;
 *         sht35_error_crc if either CRC is wrong (raw values are not updated).
 */
SHT35_Status_t SHT35_CRC_Check(void);

/**
 * @brief Converts the raw words to temperature and humidity.
 * This function performs the following steps:
 * 1. T[degC] = -45 + 175 * rawT / 65535,  RH[%] = 100 * rawRH / 65535 (SHT3x datasheet).
 * 2. Range-checks the float results before storing them.
 * 3. Stores whole degC (negative clamped to 0) and whole %RH in temperature/humidity.
 * 4. Stores the result in sht35_status.
 * @return sht35_ok if both values are in range;
 *         sht35_calc_out_of_range if T is outside -50..125 degC or RH outside 0..100 %
 *         (temperature/humidity are not updated).
 */
SHT35_Status_t SHT35_Calculate(void);

#endif