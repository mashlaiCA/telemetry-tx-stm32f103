#ifndef SOIL_MOISTURE_SENSORS_H
#define SOIL_MOISTURE_SENSORS_H

#include <stdint.h>
/**
 * @file soil_moisture_sensors.h
 * @brief Driver for Soil Moisture Sensors using ADC1 on STM32F103xx microcontroller.
 * This module initializes the ADC and provides functionality
 * to read analog values from soil moisture sensors connected to pin PA4.
 */

/**
 * @brief Initialize the ADC for soil moisture sensor.
 *  This function cperforms the following steps:
 * 1. Enable APB2 clock for GPIOA and ADC1 peripherals (RCC_APB2ENR_IOPAEN and RCC_APB2ENR_ADC1EN)
 * 2. Configure PA4 as an analog input:
 *   - Clear CNF4[0:0] and MODE4[0:0] bits in GPIOA_CRL register (0b00 for both)
 *   - Set CNF4[0:0] analog mode and MODE4[0:0] input mode bits in GPIOA_CRL register (0b00 for both)
 * 3. Configure ADC1:
 *   - Set the ADC1 clock prescaler to 6 (RCC_CFGR_ADCPRE_DIV6)
 *   - Enable the ADC1 peripheral (ADC_CR2_ADON)
 * 4. Perform ADC calibration:
 *  - Reset calibration (ADC_CR2_RSTCAL) and wait for completion
 *  - Start calibration (ADC_CR2_CAL) and wait for completion
 *  - Turn on ADC1 after calibration (ADC_CR2_ADON)
 * 5. Configure ADC1 for single conversion mode:
 *  - Set regular sequence length to 1 (ADC_SQR1 = 0)
 *  - Set the first conversion in regular sequence to channel 4 (ADC_SQR3 = 4)
 * 6. Set sample time for channel 4 to 239.5 cycles (ADC_SMPR2)
 */
void adc_init(void);

/**
 * @brief Read the ADC value from the soil moisture sensor.
 * This function starts an ADC conversion on channel 4 (PA4) and returns the digital value.
 * This function performs the following steps:
 * 1. Ensure ADC1 is enabled (ADC_CR2_ADON)
 * 2. Wait for ADC to stabilize
 * 3. Start ADC conversion (ADC_CR2_SWSTART)
 * 4. Wait until conversion is complete (check ADC_SR_EOC)
 * 5. Return the converted digital value from ADC1 data register (ADC_DR)
 * @return uint16_t The digital value read from the ADC.
 */
uint16_t adc_read(void);

#endif
