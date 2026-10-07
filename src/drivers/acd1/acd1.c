#include "stm32f103xb.h"
#include "acd1.h"


/* Limit for the EOC wait in adc1_read(). One conversion takes 63 us
   (252 ADC cycles at 4 MHz); 500 polling iterations at 8 MHz core clock last
   longer than that, so the limit only triggers when the ADC is not converting. */
#define ADC_EOC_MAX_SPINS   500u

/* Limit for the RSTCAL/CAL waits (calibration is short; this only stops a hang). */
#define ADC_CAL_MAX_SPINS   50000u

uint8_t adc1_last_error = 0; // 1 = last conversion/calibration timed out

/* Clock, power-up and calibration of ADC1 (see acd1.h). */
void adc1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable ADC1 clock

    RCC->CFGR &= ~RCC_CFGR_ADCPRE;     // ADCPRE = DIV2 -> 8/2 = 4 MHz
    ADC1->CR2 |= ADC_CR2_ADON;         // Power the ADC up (first ADON write does not start a conversion)


    for(volatile uint16_t i = 0; i<200; i++); // tSTAB: ADC power-up stabilization before calibration

    ADC1->CR2 |= ADC_CR2_RSTCAL; // Reset calibration
    for (uint32_t i = 0; (ADC1->CR2 & ADC_CR2_RSTCAL) && i < ADC_CAL_MAX_SPINS; i++)
        ; // Wait until reset is complete (hardware clears RSTCAL), bounded

    ADC1->CR2 |= ADC_CR2_CAL; // Start calibration
    for (uint32_t i = 0; (ADC1->CR2 & ADC_CR2_CAL) && i < ADC_CAL_MAX_SPINS; i++)
        ; // Wait until calibration is complete (hardware clears CAL), bounded

    adc1_last_error = ((ADC1->CR2 & (ADC_CR2_RSTCAL | ADC_CR2_CAL)) != 0u) ? 1u : 0u; // 1 if either step did not finish
}

/* One software-started conversion with a bounded EOC wait (see acd1.h). */
uint16_t adc1_read(uint8_t channel)
{
    ADC1->SQR1 = 0;            // Single conversion in the regular sequence
    ADC1->SQR3 = channel;      // SQ1 = channel

    // Channels 0-9 use SMPR2, channels 10-17 use SMPR1.
    if (channel < 10) {
        ADC1->SMPR2 &= ~(0x7 << (channel * 3)); // Clear sample time bits for channel x
        ADC1->SMPR2 |= (0x7 << (channel * 3));  // Set sample time to 239.5 cycles
    } else {
        ADC1->SMPR1 &= ~(0x7 << ((channel - 10) * 3)); // Clear sample time bits
        ADC1->SMPR1 |= (0x7 << ((channel - 10) * 3));  // 239.5 cycles
    }

    (void)ADC1->DR; // Discard a stale result (reading DR clears EOC)

    // ADC1 is already ON (from adc1_init); writing ADON again here is the
    // documented STM32F1 idiom to launch a software conversion (RM0008: a
    // conversion starts when ADON is set a second time), since
    // EXTTRIG/EXTSEL are never configured for SWSTART to take effect.
    ADC1->CR2 |= ADC_CR2_ADON;

    uint32_t spins = 0; // Polling iterations so far
    while (!(ADC1->SR & ADC_SR_EOC)) // Wait for end of conversion
    {
        if (++spins >= ADC_EOC_MAX_SPINS)
        {
            adc1_last_error = 1; // Conversion did not complete
            return 0;
        }
    }

    adc1_last_error = 0; // Conversion succeeded
    return ADC1->DR; // Return the converted value
}


/* Powers the ADC down for Stop mode. */
void adc1_sleep(void)
{
    ADC1->CR2 &= ~ADC_CR2_ADON;         // Power down the analog part
    RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN; // Gate the clock as well
}


/* Powers the ADC up after Stop mode and recalibrates it. */
void adc1_wake(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // ADC clock on

    ADC1->CR2 |= ADC_CR2_ADON; // Power up

    for (volatile uint16_t i = 0; i < 200; i++)
        ; // t_STAB after powering the ADC up


    ADC1->CR2 |= ADC_CR2_RSTCAL; // Reset calibration
    for (uint32_t i = 0; (ADC1->CR2 & ADC_CR2_RSTCAL) && i < ADC_CAL_MAX_SPINS; i++)
        ; // Wait for RSTCAL to clear, bounded

    ADC1->CR2 |= ADC_CR2_CAL; // Start calibration
    for (uint32_t i = 0; (ADC1->CR2 & ADC_CR2_CAL) && i < ADC_CAL_MAX_SPINS; i++)
        ; // Wait for CAL to clear, bounded

    adc1_last_error = ((ADC1->CR2 & (ADC_CR2_RSTCAL | ADC_CR2_CAL)) != 0u) ? 1u : 0u; // 1 if calibration did not finish
}
