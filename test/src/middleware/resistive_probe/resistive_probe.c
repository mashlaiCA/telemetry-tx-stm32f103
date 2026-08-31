#include "resistive_probe.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpio_hw.h"
#include "drivers/tim/tim2_hw.h"


void resistive_probe_init(const resistive_probe_t *p){
    gpio_A_polarity_init(p->pin_a);
    gpio_A_polarity_init(p->pin_b);
    gpio_a_analog_input_init(p->adc_channel);
    polarity_off(p->pin_a, p->pin_b);
}

uint16_t resistive_probe_read(const resistive_probe_t *p){

    uint16_t fwd, rev;
 
    polarity_fwd(p->pin_a, p->pin_b);
    delay_hw_us(p->set_us);
    fwd = adc1_read(p->adc_channel);

    polarity_rev(p->pin_a, p->pin_b);
    delay_hw_us(p->set_us);
    rev = adc1_read(p->adc_channel);

    polarity_off(p->pin_a, p->pin_b);

    return (fwd + (4095 - rev)) / 2;
    
}