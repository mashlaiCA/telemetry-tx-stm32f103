#ifndef SHT31_H
#define SHT31_H

#include <stdint.h>
#include <stdbool.h>

uint8_t SHT31_CalcCRC(uint8_t *data, uint8_t len);
bool SHT31_CheckCRC(uint8_t *rx);
float SHT31_ConvertTemperature(uint16_t raw);
float SHT31_ConvertHumidity(uint16_t raw);
bool SHT31_ReadData(float *temperature, float *humidity);
void SHT31_StartMeasurement(void);


#endif