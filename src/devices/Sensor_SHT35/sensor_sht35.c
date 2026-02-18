#include "drivers/sht35/sht35.h"
#include "drivers/I2C/i2c_hw.h"


#define SHT35_ADDR   0x44  // I2C address of the SHT35 sensor
uint8_t cmd[2] = {0x24, 0x00}; // Command to start measurement (high repeatability, clock stretching disabled)



void I2C_Write_Sensor_SHT35() // Send command to start measurement
{  
    I2C_Write(SHT35_ADDR, cmd, 2); // Send the command to the SHT35 sensor to start a measurement using I2C communication
}

void I2C_Read_Sensor_SHT35() // Read raw data from sensor
{  
    I2C_Read(SHT35_ADDR, buf, 6); // Read 6 bytes of data from the SHT35 sensor, which includes raw temperature and humidity values along with their CRC checksums
}

void I2C_Restart_Sensor_SHT35() // Restart sensor
{
    I2C_Write(SHT35_ADDR, 0x30, 0xA2); // Send soft reset command   
}


uint16_t temperatureSHT35(void)// Get calculated temperature
{
    return temperature; // Return the calculated temperature value in hundredths of degrees Celsius
}
uint16_t humiditySHT35(void)// Get calculated humidity
{
    return humidity; // Return the calculated humidity value in hundredths of percent
}
