#include "sht35.h"
#include "sht35_status.h"
#include "drivers/I2C/i2c_hw.h"
#include "drivers/tim/tim2_hw.h"

SHT35_Status_t sht35_status = sht35_ok; // Result of the last CRC check / conversion

uint8_t buf[6] = {0}; // Raw I2C data, filled by I2C_Read_Sensor_SHT35()

uint16_t rawT = 0;  // Raw temperature word (valid after a successful CRC check)
uint16_t rawRH = 0; // Raw humidity word (valid after a successful CRC check)

uint16_t temperature = 0; // Last valid temperature, whole degC (negative clamped to 0)
uint16_t humidity = 0;    // Last valid relative humidity, whole %RH

/* Sensirion CRC-8: init 0xFF, polynomial 0x31, MSB first, no final XOR. */
uint8_t SHT35_CRC8(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF; // Initial value defined by the SHT3x datasheet

    for (uint8_t i = 0; i < len; i++) // For each data byte
    {
        crc ^= data[i];                 // Feed the byte into the top of the CRC register
        for (uint8_t b = 0; b < 8; b++) // Process 8 bits, MSB first
        {
            if (crc & 0x80)              // Top bit set: shift and apply the polynomial
                crc = (crc << 1) ^ 0x31; // Polynomial 0x31 = x^8 + x^5 + x^4 + 1
            else
                crc <<= 1; // Top bit clear: shift only
        }
    }
    return crc; // Final CRC value
}

/* Verifies both CRC bytes. The result is written to the global sht35_status as
   well as returned, because the FSM reads it via get_last_sht35_error(); a status
   that is only returned would let corrupted data pass unnoticed. */
SHT35_Status_t SHT35_CRC_Check(void)
{
    if (SHT35_CRC8(&buf[0], 2) != buf[2]) // Temperature word CRC mismatch
    {
        sht35_status = sht35_error_crc; // Publish the error for the FSM
        return sht35_error_crc;         // Raw values are left unchanged
    }
    if (SHT35_CRC8(&buf[3], 2) != buf[5]) // Humidity word CRC mismatch
    {
        sht35_status = sht35_error_crc; // Publish the error for the FSM
        return sht35_error_crc;         // Raw values are left unchanged
    }

    rawT = (buf[0] << 8) | buf[1];  // Raw temperature word, MSB first
    rawRH = (buf[3] << 8) | buf[4]; // Raw humidity word, MSB first

    sht35_status = sht35_ok; // Clear the status on success so an old error does not stick
    return sht35_ok;
}

/* Converts raw words to physical values.
   The conversion is done in float and range-checked BEFORE storing into the
   uint16_t globals: a negative temperature cast to uint16_t would wrap to a
   huge value, and a check like "temperature < -50" on an unsigned variable is
   always false. */
SHT35_Status_t SHT35_Calculate(void)
{
    float t_c = -45.0f + 175.0f * ((float)rawT / 65535.0f); // T[degC] = -45 + 175 * raw / (2^16 - 1)
    float rh = 100.0f * ((float)rawRH / 65535.0f);          // RH[%] = 100 * raw / (2^16 - 1)

    if (t_c < -50.0f || t_c > 125.0f) // Outside the accepted temperature range
    {
        sht35_status = sht35_calc_out_of_range; // Publish the error for the FSM
        return sht35_calc_out_of_range;         // temperature/humidity are not updated
    }

    if (rh < 0.0f || rh > 100.0f) // Outside the physical humidity range
    {
        sht35_status = sht35_calc_out_of_range; // Publish the error for the FSM
        return sht35_calc_out_of_range;         // temperature/humidity are not updated
    }

    /* temperature/humidity stay uint16_t (the type is used in system_data and
       in the payload), so negative temperatures are clamped to 0 and the
       fraction is truncated. Sub-zero air temperatures would require int16_t
       here, in sht35.h, in system_data_t and in int_to_str() callers. */
    temperature = (t_c < 0.0f) ? 0u : (uint16_t)t_c; // Whole degC, clamped at 0
    humidity = (uint16_t)rh;                         // Whole %RH

    sht35_status = sht35_ok; // Clear the status on success
    return sht35_ok;         // Successful conversion
}
