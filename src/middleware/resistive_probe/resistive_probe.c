#include "resistive_probe.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpio_hw.h"
#include "drivers/tim/tim2_hw.h"
#include "stm32f103xb.h" // __get_PRIMASK/__disable_irq


/* Drive pins as outputs, sense pin as analog input, probe released. */
void resistive_probe_init(const resistive_probe_t *p){
    gpio_A_polarity_init(p->pin_a);          // Push-pull output, 2 MHz
    gpio_A_polarity_init(p->pin_b);          // Push-pull output, 2 MHz
    gpio_a_analog_input_init(p->adc_channel); // ADC channel n -> PAn analog input
    polarity_off(p->pin_a, p->pin_b);        // Both pins floating: no current through the probe
}

/* Forward + reverse measurement with equal duration (see resistive_probe.h). */
uint16_t resistive_probe_read(const resistive_probe_t *p){

    uint16_t fwd, rev; // Raw ADC codes of both directions

    /* Interrupts are disabled for the whole excitation. TIM2 fires every
       millisecond and EXTI1 (LoRa DIO0) at any moment; an interrupt inside one
       direction would make that direction longer than the other, i.e. a net
       DC current through the electrodes and their gradual electrochemical
       degradation (the same mechanism as for Watermark, only slower).
       PRIMASK is saved and restored so a caller's critical section is not
       ended here. The section lasts about 2 * (set_us + 63 us); for the leaf
       sensor (set_us = 300) that is under 1 ms, so a TIM2 tick is only
       delayed (UIF stays pending), not lost. */
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    polarity_fwd(p->pin_a, p->pin_b);  // pin_a = 1, pin_b = 0
    delay_hw_us(p->set_us);            // Let the divider settle
    fwd = adc1_read(p->adc_channel);   // Forward reading

    /* On an ADC failure the excitation is removed immediately, as in
       watermark_sample(); otherwise the electrodes would stay under voltage. */
    if (adc1_last_error)
    {
        polarity_off(p->pin_a, p->pin_b); // Remove excitation
        __set_PRIMASK(primask);           // Restore interrupt state
        return 0;
    }

    polarity_rev(p->pin_a, p->pin_b);  // pin_a = 0, pin_b = 1
    delay_hw_us(p->set_us);            // Same settling time as forward
    rev = adc1_read(p->adc_channel);   // Reverse reading

    uint8_t failed = adc1_last_error; // Capture before anything else touches the ADC

    polarity_off(p->pin_a, p->pin_b); // Remove excitation

    __set_PRIMASK(primask); // Restore interrupt state

    if (failed)
    {
        return 0; // Reverse conversion failed
    }

    return (fwd + (4095 - rev)) / 2; // Reverse reading is mirrored, then both directions are averaged

}