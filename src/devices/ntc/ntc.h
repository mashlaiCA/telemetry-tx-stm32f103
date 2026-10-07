/**
 * @file  ntc.h
 * @brief Soil temperature from a 10 kOhm NTC thermistor (B = 3950) on PA1 / ADC1 channel 1.
 * This file provides:
 * 1. ntc_init() - PA1 as analog input.
 * 2. temp_c_ntc() - one conversion turned into degrees Celsius (Beta equation).
 * On an ADC failure or an open/shorted thermistor the function returns 24 degC,
 * the reference temperature of the Watermark conversion, so the temperature
 * compensation becomes neutral instead of wrong.
 */

#ifndef NTC_H
#define NTC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ntc_init(void); // Configures PA1 as analog input
float temp_c_ntc(uint8_t chanel_adc); // Temperature in degC from ADC channel chanel_adc; 24.0 on ADC error or raw code outside 10..4085

#ifdef __cplusplus
}
#endif

#endif