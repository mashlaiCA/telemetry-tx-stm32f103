#include "gpio_hw.h"
#include "../tim/tim2_hw.h"
#include "stm32f103xb.h"

// Initialize GPIOA clock
void gpio_A_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable GPIOA clock RCC->APB2ENR |= (1<<2);
}

// Initialize GPIOB clock
void gpio_B_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // Enable GPIOB and AFIO clock
}

// Initialize GPIOB pins for I2C SDA and SCL
void gpio_B_init_I2C_SDA_SCL(uint8_t pin)
{
    GPIOB->CRL &= ~(0xF << (pin * 4)); // Clear CNF[1:0] and MODE[1:0] bits
    GPIOB->CRL |= (0xF << (pin * 4));  // set PBx as Alternate function open-drain with max speed 50 MHz
}

// Initialize GPIOA pin as analog input
void gpio_a_analog_input_init(uint8_t pin)
{
    GPIOA->CRL &= ~(0xF << (pin * 4)); // CNFx[1:1] and MODEx[1:1] bits
    GPIOA->CRL |= (0x0 << (pin * 4));  // Set PAx as analog input
}

// Initialize GPIOA pin as output for LED
void gpio_a_led_output_init(uint8_t pin)
{
    GPIOA->CRL &= ~(0xF << (pin * 4)); // Clear CNF[0:0] and MODE[0:0] bits
    GPIOA->CRL |= (0x1 << (pin * 4));  // Set PAx as output (push-pull)
    GPIOA->BSRR = (1 << (pin));        // Set PAx high (turn off LED)
}

// Set GPIOA pin low to turn on LED
void gpio_a_set(uint8_t led_gpio)
{
    GPIOA->BRR = (1 << led_gpio); // Set PAx low to turn on LED light bulb
}

// Set GPIOA pin high to turn off LED
void gpio_a_reset(uint8_t led_gpio)
{
    GPIOA->BSRR = (1 << led_gpio); // Set PAx high to turn off LED light bulb
}

void gpio_SPI_init(void)
{
    GPIOA->CRL &= ~(0xF << (5 * 4));
    GPIOA->CRL |= (0xB << (5 * 4));

    GPIOA->CRL &= ~(0xF << (0 * 4));
    GPIOA->CRL |= (0x3 << (0 * 4));

    GPIOA->CRL &= ~(0xF << (6 * 4));
    GPIOA->CRL |= (0x8 << (6 * 4));

    GPIOA->CRL &= ~(0xF << (7 * 4));
    GPIOA->CRL |= (0xB << (7 * 4));
}

void lora_ctrl_gpio_init(void)
{
    GPIOB->CRL &= ~(0xF << (1 * 4));
    GPIOB->CRL |= (0x3 << (1 * 4));

    GPIOA->CRL &= ~(0xF << (1 * 4));
    GPIOA->CRL |= (0x4 << (1 * 4));
}

void rst_low(void)
{
    GPIOB->BRR = (1 << 1);
    delay_hw_ms(10);
}

void rst_high(void)
{
    GPIOB->BSRR = (1 << 1);
    delay_hw_ms(10);
}

void nss_low(void)
{
    GPIOA->BRR = (1 << 0);
}

void nss_high(void)
{
    GPIOA->BSRR = (1 << 0);
}

uint8_t dio0_read(void)
{
    return (GPIOA->IDR & (1 << 1)) ? 1 : 0;
}