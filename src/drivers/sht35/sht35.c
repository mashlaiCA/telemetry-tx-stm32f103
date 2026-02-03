#include "sht35.h"
#include "drivers/I2C/i2c_hw.h"
#include "drivers/tim/tim2_hw.h"

SHT35_Status_t sht35_status = sht35_ok; // Global variable to hold the last SHT35 error status

uint8_t buf[6] = {0}; // data buffer

uint16_t rawT = 0;  // raw temperature
uint16_t rawRH = 0; // raw humidity

uint16_t temperature = 0; // temperature in hundredths of degrees Celsius
uint16_t humidity = 0;    // humidity in hundredths of percent

uint8_t SHT35_CRC8(uint8_t *data, uint8_t len) // CRC8 checksum calculation
{
    uint8_t crc = 0xFF; // Initialization

    for (uint8_t i = 0; i < len; i++) // Data bytes
    {
        crc ^= data[i];                 // XOR byte into least sig. byte of crc
        for (uint8_t b = 0; b < 8; b++) // 8 bits
        {
            if (crc & 0x80)              // MSB is set3
                crc = (crc << 1) ^ 0x31; // Polynomial x^8 + x^5 + x^4 +1
            else
                crc <<= 1; // Shift left
        }
    }
    return crc; // Final CRC value
}

SHT35_Status_t SHT35_CRC_Check(void) // CRC check
{
    if (SHT35_CRC8(&buf[0], 2) != buf[2])
        return sht35_error_crc; // Check temperature CRC
    if (SHT35_CRC8(&buf[3], 2) != buf[5])
        return sht35_error_crc; // Check humidity CRC

    rawT = (buf[0] << 8) | buf[1];  // Combine bytes to raw temperature
    rawRH = (buf[3] << 8) | buf[4]; // Combine bytes to raw humidity
    return sht35_ok;
}

SHT35_Status_t SHT35_Calculate(void)
{
    temperature = -45.0f + 175.0f * ((float)rawT / 65535.0f); // Calculate temperature in Celsius
    humidity = 100.0f * ((float)rawRH / 65535.0f);            // Calculate humidity in percent

    if (temperature < -50.0f || temperature > 125.0f)
        return sht35_calc_out_of_range;

    if (humidity < 0.0f || humidity > 100.0f)
        return sht35_calc_out_of_range;

    return sht35_ok;
}
