#include "stm32f103xb.h"
#include "acd1.h"

void adc1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable ADC1 clock

    RCC->CFGR &= ~RCC_CFGR_ADCPRE;     // ADCPRE = DIV2 -> 8/2 = 4 MHz
    ADC1->CR2 |= ADC_CR2_ADON;         // Enable ADC1


    for(volatile uint16_t i = 0; i<200; i++);//test

    ADC1->CR2 |= ADC_CR2_RSTCAL; // Reset calibration
    while (ADC1->CR2 & ADC_CR2_RSTCAL)
        ; // Wait until reset is complete

    ADC1->CR2 |= ADC_CR2_CAL; // Start calibration
    while (ADC1->CR2 & ADC_CR2_CAL)
        ; // Wait until calibration is complete
}

uint16_t adc1_read(uint8_t channel)
{
    ADC1->SQR1 = 0;            // Single conversion in the regular sequence
    ADC1->SQR3 = channel;

    // Channels 0-9 use SMPR2, channels 10-17 use SMPR1.
    if (channel < 10) {
        ADC1->SMPR2 &= ~(0x7 << (channel * 3)); // Clear sample time bits for channel x
        ADC1->SMPR2 |= (0x7 << (channel * 3));  // Set sample time to 239.5 cycles
    } else {
        ADC1->SMPR1 &= ~(0x7 << ((channel - 10) * 3));
        ADC1->SMPR1 |= (0x7 << ((channel - 10) * 3));
    }

    // ADC1 is already ON (from adc1_init); writing ADON again here is the
    // documented STM32F1 idiom to launch a software conversion, since
    // EXTTRIG/EXTSEL are never configured for SWSTART to take effect.
    ADC1->CR2 |= ADC_CR2_ADON;
    while (!(ADC1->SR & ADC_SR_EOC))
        ;            // Wait until conversion is complete
    return ADC1->DR; // Return the converted value
}