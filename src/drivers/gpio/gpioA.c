#include "gpioA.h"
#include "stm32f103xb.h"


void gpio_a_init(void){ //!
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable GPIOA clock
}

void gpio_a_analog_input_init(uint8_t pin){//!
    GPIOA->CRL &= ~(0xF << pin * 4); // CNFx[0:0] and MODEx[0:0] bits
    GPIOA->CRL |= (0x0 << pin * 4);  // Set PAx as analog input
}

void gpio_a_led_output_init(uint8_t pin){//!
    GPIOA->CRL &= ~(0xF << (pin * 4)); // Clear CNF[0:0] and MODE[0:0] bits
    GPIOA->CRL |= (0x1 << (pin * 4));  // Set PAx as output (push-pull)
    GPIOA->BSRR = (1 << pin); // Set PAx high (turn off LED)
}

void gpio_a_set(uint8_t led_gpio)
{
    GPIOA->BRR = (1 << led_gpio); // Set PAx low to turn on LED light bulb
}

void gpio_a_reset(uint8_t led_gpio)
{
    GPIOA->BSRR = (1 << led_gpio); // Set PAx high to turn off LED light bulb
}   