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

void gpio_PBx_analog_input_init(uint8_t pin)   // pin: 0..7
{
    GPIOB->CRL &= ~(0xF << (pin * 4));       // analog mode = 0000
}
/*
//============test timing========

void probe_pin_init(void)
{
    GPIOB->CRL &= ~(0xFu << 0);  // PB0
    GPIOB->CRL |=  (0x1u << 0);  // output 10 MHz push-pull
    GPIOB->BSRR = (1u << (0 + 16)); // LOW
}
//=========================
*/


//===============================polarity==============//


//=====================WM==================

void gpio_PBx_polarity_init(uint8_t pinB)      
{
    uint8_t s = (pinB - 8) * 4;
    GPIOB->CRH &= ~(0xF << s);
    GPIOB->CRH |=  (0x2 << s);
}

void polarity_PBx_fwd(uint8_t pin1, uint8_t pin2){
    gpio_PBx_polarity_init(pin1);
    gpio_PBx_polarity_init(pin2);
    GPIOB -> BSRR = (1 << pin1);
    GPIOB -> BRR = (1 << pin2);
}

void polarity_PBx_rev(uint8_t pin1, uint8_t pin2){
    gpio_PBx_polarity_init(pin1);
    gpio_PBx_polarity_init(pin2);
    GPIOB -> BSRR = (1 << pin2);
    GPIOB -> BRR = (1 << pin1);
}


void polarity_PBx_off(uint8_t pin1, uint8_t pin2){
    uint8_t s1 = (pin1 - 8) * 4;
    uint8_t s2 = (pin2 - 8) * 4;
    GPIOB -> CRH &= ~((0xF << s1) | (0xF << s2));
    GPIOB -> CRH |= (0x4 << s1) | (0x4 << s2);
}

//=====================WM==================
void gpio_A_polarity_init(uint8_t pin1){

    GPIOA -> CRL &= ~(0xF << pin1 * 4);
    GPIOA -> CRL |= (0x2 << pin1 * 4);
}

void polarity_fwd(uint8_t pin1, uint8_t pin2){
    gpio_A_polarity_init(pin1);
    gpio_A_polarity_init(pin2);
    GPIOA -> BSRR = (1 << pin1);
    GPIOA -> BRR = (1 << pin2);
}

void polarity_rev(uint8_t pin1, uint8_t pin2){
    gpio_A_polarity_init(pin1);
    gpio_A_polarity_init(pin2);
    GPIOA -> BSRR = (1 << pin2);
    GPIOA -> BRR = (1 << pin1);
}

void polarity_off(uint8_t pin1, uint8_t pin2){
    // Float both excitation pins (input, no pull) instead of driving them
    // low, so no leakage/bias current path exists through the sensor
    // between measurements.
    GPIOA -> CRL &= ~((0xF << pin1 * 4) | (0xF << pin2 * 4));
    GPIOA -> CRL |= (0x4 << pin1 * 4) | (0x4 << pin2 * 4);
}

//===============================+====================//

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

void ntc_gpio_init(void){
   GPIOA->CRL &= ~(0xF << (1 * 4)); //PA1 Analog
}

void gpio_SPI_init(void)
{
    GPIOA->CRL &= ~(0xF << (5 * 4));// Clear mode bits for PA5 (SCK)
    GPIOA->CRL |= (0xB << (5 * 4));// Set PA5 to Alternate Function Push-Pull output

    GPIOB->CRH &= ~(0xF << (4 * 4));// Clear mode bits for PB12 (NSS)
    GPIOB->CRH |= (0x3 << (4 * 4));// Set P12 to General Purpose Output Push-Pull

    GPIOA->CRL &= ~(0xF << (6 * 4));// Clear mode bits for PA6 (MISO)+
    GPIOA->CRL |= (0x8 << (6 * 4));// Set PA6 to Input Floating

    GPIOA->CRL &= ~(0xF << (7 * 4)); // Clear mode bits for PA7 (MOSI)
    GPIOA->CRL |= (0xB << (7 * 4));// Set PA7 to Alternate Function Push-Pull output
}

void lora_ctrl_gpio_init(void)
{
    GPIOB->CRL &= ~(0xF << (5 * 4));  // Clear mode bits for PB5 (RST)
    GPIOB->CRL |= (0x3 << (5 * 4)); // Set PB5 to General Purpose Output Push-Pull

    GPIOB->CRL &= ~(0xF << (1 * 4)); // Clear mode bits for PB1 (DIO0)+
    GPIOB->CRL |= (0x4 << (1 * 4)); // Set PB1 to Input Floating
    
}

uint8_t gpio_read_pin(GPIO_TypeDef* GPIOx, uint8_t pin)
{
    return (GPIOx->IDR & (1 << pin)) ? 1 : 0;
}

void rst_low(void)
{
    GPIOB->BRR = (1 << 5);
    delay_hw_ms(10);
}

void rst_high(void)
{
    GPIOB->BSRR = (1 << 5);
    delay_hw_ms(10);
}

void nss_low(void)
{
    GPIOB->BRR = (1 << 12);
}

void nss_high(void)
{
    GPIOB->BSRR = (1 << 12);
}

uint8_t dio0_read(void)
{
    return (GPIOB->IDR & (1 << 1)) ? 1 : 0;
}