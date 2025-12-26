#include "stm32f103xb.h"
#include "soil_moisture_sensors.h"

/**
 * @brief Initialize the ADC for soil moisture sensor on PA4.
 */

void adc_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable ADC1 clock

    GPIOA->CRL &= ~(0xF << 16); // CNF4[0:0] and MODE4[0:0] bits
    GPIOA->CRL |= (0x0 << 16);  // Set PA4 as analog input

    RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6; // Set ADC prescaler to 6
    ADC1->CR2 |= ADC_CR2_ADON;         // Enable ADC1

    ADC1->CR2 |= ADC_CR2_RSTCAL; // Reset calibration
    while (ADC1->CR2 & ADC_CR2_RSTCAL)
        ; // Wait until reset is complete

    ADC1->CR2 |= ADC_CR2_CAL; // Start calibration
    while (ADC1->CR2 & ADC_CR2_CAL)
        ; // Wait until calibration is complete

    ADC1->CR2 |= ADC_CR2_ADON; // Turn on ADC after calibration

    ADC1->SQR1 = 0; // One conversion in regular sequence
    ADC1->SQR3 = 4; // 1st conversion in regular sequence is channel 4 (PA4)

    ADC1->SMPR2 |= (0x7 << (3 * 4)); // Set sample time for channel 4 to 239.5 cycles
}
/**
 * @brief Read the ADC value from the soil moisture sensor on PA4.
 * @return uint16_t The digital value read from the ADC.
 */

uint16_t adc_read(void)
{
    ADC1->CR2 |= ADC_CR2_ADON; // Ensure ADC is enabled
                               // timer_sec(1); // Wait for ADC to stabilize

    ADC1->CR2 |= ADC_CR2_SWSTART; // Start conversion
    while (!(ADC1->SR & ADC_SR_EOC))
        ;            // Wait until conversion is complete
    return ADC1->DR; // Return the converted value
}