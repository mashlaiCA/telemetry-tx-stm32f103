#include "i2c.h"
#include "drivers/I2C/i2c_hw.h"
#include "drivers/sht35/sht35.h"


#define SHT35_ADDR   0x44  
uint8_t cmd[2] = {0x24, 0x00};


void I2C_Write_Sensor_SHT35() 
{  
    I2C_Write(SHT35_ADDR, cmd, 2);
}

void I2C_Read_Sensor_SHT35()
{  
    I2C_Read(SHT35_ADDR, buf, 6);
}