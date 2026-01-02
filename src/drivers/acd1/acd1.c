#include "stm32f103xb.h"
#include "acd1.h"


void adc1_init(void){
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable ADC1 clock

    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6; // Set ADC prescaler to 6
    ADC1->CR2 |= ADC_CR2_ADON;         // Enable ADC1

    ADC1->CR2 |= ADC_CR2_RSTCAL; // Reset calibration
    while (ADC1->CR2 & ADC_CR2_RSTCAL)
        ; // Wait until reset is complete

    ADC1->CR2 |= ADC_CR2_CAL; // Start calibration
    while (ADC1->CR2 & ADC_CR2_CAL)
        ; // Wait until calibration is complete
}


uint16_t adc1_read(uint8_t channel)
{
    ADC1->CR2 |= ADC_CR2_ADON; // Ensure ADC is enabled
    ADC1->SQR1 = 0; // 1st conversion in regular sequence is channel 4 (PA4)
    ADC1->SQR3 = channel;
    
    ADC1->SMPR2 &= ~(0x7 << (channel * 3)); // Clear sample time bits for channel x
    ADC1->SMPR2 |= (0x7 << (channel * 3)); // Set sample time to 239.5 cycles

    ADC1->CR2 |= ADC_CR2_SWSTART; // Start conversion
    while (!(ADC1->SR & ADC_SR_EOC)); // Wait until conversion is complete
    return ADC1->DR; // Return the converted value
}