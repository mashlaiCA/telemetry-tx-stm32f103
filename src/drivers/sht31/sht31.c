#include <stdbool.h>
#include "stm32f103xb.h"
#include "sht31.h"
#include "../I2C/i2c.h"

uint8_t SHT31_CalcCRC(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;          

    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];         

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)      
                crc = (crc << 1) ^ 0x31; 
            else
                crc <<= 1;
        }
    }
    return crc;
}

bool SHT31_CheckCRC(uint8_t *rx){

    if(SHT31_CalcCRC(&rx[0], 2) != rx[2]){
        return false;
    }
    if(SHT31_CalcCRC(&rx[3], 2) != rx[5]){
        return false;
    }
    return true;
}

float SHT31_ConvertTemperature(uint16_t raw){
   
    return -45.0f + 175.0f * ((float)raw / 65535.0f);
}

float SHT31_ConvertHumidity(uint16_t raw){
   
    return 100.0f * ((float)raw / 65535.0f);
}

bool SHT31_ReadData(float *temperature, float *humidity){
static uint8_t rx[6];
   
I2C_StartRead(&i2c_ctx, 0x44, rx, 6);

if(i2c_ctx.state != I2C_IDLE){
    return false;
}

uint16_t raw_temp = (rx[0] << 8) | rx[1];
uint16_t raw_hum = (rx[3] << 8) | rx[4];

*temperature = SHT31_ConvertTemperature(raw_temp);
*humidity = SHT31_ConvertHumidity(raw_hum);

return true;
}

void SHT31_StartMeasurement(void){
    uint8_t cmd[2] = {0x24, 0x00};
    I2C_StartWrite(&i2c_ctx, 0x44, cmd, 2);
}   