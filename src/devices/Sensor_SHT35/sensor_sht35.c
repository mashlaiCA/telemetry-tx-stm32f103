#include "drivers/sht35/sht35.h"
#include "drivers/I2C/i2c_hw.h"


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

void I2C_Restart_Sensor_SHT35()
{
    I2C_Write(SHT35_ADDR, 0x30, 0xA2); // Send soft reset command   
}


uint16_t temperatureSHT35(void){
    return temperature;
}
uint16_t humiditySHT35(void){
    return humidity;
}
