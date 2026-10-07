#include "drivers/sht35/sht35.h"
#include "drivers/I2C/i2c_hw.h"
#include "sensor_sht35.h"

#define SHT35_ADDR 0x44                   // 7-bit I2C address (ADDR pin to GND)
uint8_t cmd[2] = {0x24, 0x00};            // Single-shot measurement: high repeatability, clock stretching disabled
uint8_t cmd_soft_reset[2] = {0x30, 0xA2}; // Soft reset command

/* Addresses on the shared I2C1 bus:
   SHT35 = 0x44 (ADDR pin to GND; 0x45 if ADDR to VDD),
   DS3231 = 0x68, AT24C32 EEPROM on the ZS-042 module = 0x57.
   No conflict - no addresses overlap. */

/* Starts a measurement. The status of this transaction is returned directly,
   because the global I2C status may be overwritten by DS3231 on the same bus. */
I2C_Status_t I2C_Write_Sensor_SHT35(void)
{
    return I2C_Write(SHT35_ADDR, cmd, 2); // Send the 2-byte measurement command
}

/* Reads the measurement result into the driver buffer buf[]. */
I2C_Status_t I2C_Read_Sensor_SHT35(void)
{
    return I2C_Read(SHT35_ADDR, buf, 6); // 6 bytes: T MSB, T LSB, T CRC, RH MSB, RH LSB, RH CRC
}

/* Sends the soft reset command. */
I2C_Status_t I2C_Restart_Sensor_SHT35(void)
{
    return I2C_Write(SHT35_ADDR, cmd_soft_reset, 2); // Send the 2-byte soft reset command
}

/* Getter for the last converted temperature. */
uint16_t temperatureSHT35(void)
{
    return temperature; // Whole degC, negative values clamped to 0 (see SHT35_Calculate())
}
/* Getter for the last converted humidity. */
uint16_t humiditySHT35(void)
{
    return humidity; // Whole %RH
}
