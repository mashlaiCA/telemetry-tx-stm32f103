#ifndef ACD1_H
#define ACD1_H

#include <stdint.h>

/**
 * @file acd1.h
 * @brief Driver for ADC1 on STM32F103xx microcontroller.
 * This module provides functions to initialize ADC1 and read analog values
 * from specified channels.
 */

/** @brief Initialize ADC1 peripheral.
 *   This function performs the following steps:
 *  1. Enable the ADC1 clock in the RCC APB2ENR register.
 *  2. Set the ADC prescaler to divide by 6 in the RCC CFGR register.
 *  3. Enable ADC1 by setting the ADON bit in the ADC1 CR2 register.
 *  4. Reset the ADC calibration by setting the RSTCAL bit in the ADC1 CR2 register
 *     and wait until the reset is complete.
 *  5. Start the ADC calibration by setting the CAL bit in the ADC1 CR2 register
 *     and wait until the calibration is complete.
 */
void adc1_init(void);


/** @brief Read analog value from specified ADC1 channel.
 *   This function performs the following steps:
 *  1. Ensure ADC1 is enabled by setting the ADON bit in the ADC1 CR2 register.
 *  2. Configure the regular sequence registers (SQR1 and SQR3) to select the
 *     specified channel for conversion.
 *  3. Set the sample time for the specified channel to 239.5 cycles in the
 *     ADC1 SMPR2 register.
 *  4. Start the conversion by setting the SWSTART bit in the ADC1 CR2 register.
 *  5. Wait until the conversion is complete by polling the EOC bit in the
 *     ADC1 SR register.
 *  6. Return the converted value from the ADC1 DR register.
 *  @param channel The ADC1 channel number (0-15) to read from.
 *  @return The 12-bit converted analog value from the specified channel.
 */
uint16_t adc1_read(uint8_t channel);

#endif
