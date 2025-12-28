#include "stm32f103xb.h"
#include "soil_moisture_sensors.h"

/**
 * @brief Initialize the ADC for soil moisture sensor on PA4.
 */

void adc_init_pin(uint8_t pin)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable ADC1 clock

    GPIOA->CRL &= ~(0xF << pin * 4); // CNFx[0:0] and MODEx[0:0] bits
    GPIOA->CRL |= (0x0 << pin * 4);  // Set PAx as analog input

    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6; // Set ADC prescaler to 6
    ADC1->CR2 |= ADC_CR2_ADON;         // Enable ADC1

    ADC1->CR2 |= ADC_CR2_RSTCAL; // Reset calibration
    while (ADC1->CR2 & ADC_CR2_RSTCAL)
        ; // Wait until reset is complete

    ADC1->CR2 |= ADC_CR2_CAL; // Start calibration
    while (ADC1->CR2 & ADC_CR2_CAL)
        ; // Wait until calibration is complete
}
/**
 * @brief Read the ADC value from the soil moisture sensor on PAx.
 * @return uint16_t The digital value read from the ADC.
 */

uint16_t adc_read(uint8_t channel)
{
    ADC1->CR2 |= ADC_CR2_ADON; // Ensure ADC is enabled
    ADC1->SQR1 = 0; // 1st conversion in regular sequence is channel 4 (PA4)
    ADC1->SQR3 = channel;

    ADC1->CR2 |= ADC_CR2_SWSTART; // Start conversion
    while (!(ADC1->SR & ADC_SR_EOC)); // Wait until conversion is complete
    return ADC1->DR; // Return the converted value
}