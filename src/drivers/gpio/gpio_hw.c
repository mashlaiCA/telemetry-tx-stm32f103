#include "gpio_hw.h"
#include "../timeout_hw/timeout_hw.h"
#include "stm32f103xb.h"


timeout_t lora_timeout; 
uint8_t lora_timeout_ms;

// Initialize GPIOA clock
void gpio_A_init(void){ 
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable GPIOA clock RCC->APB2ENR |= (1<<2);
}

// Initialize GPIOB clock
void gpio_B_init(void){
RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;// Enable GPIOB and AFIO clock
}

// Initialize GPIOB pins for I2C SDA and SCL
void gpio_B_init_I2C_SDA_SCL(uint8_t pin){
    GPIOB->CRL &= ~(0xF << (pin * 4)); // Clear CNF[1:0] and MODE[1:0] bits
    GPIOB->CRL |= (0xF << (pin * 4));  // set PBx as Alternate function open-drain with max speed 50 MHz
}

// Initialize GPIOA pin as analog input
void gpio_a_analog_input_init(uint8_t pin){
    GPIOA->CRL &= ~(0xF << (pin * 4)); // CNFx[1:1] and MODEx[1:1] bits
    GPIOA->CRL |= (0x0 << (pin * 4));  // Set PAx as analog input
}

void gpio_SPI_init()
{
    GPIOA->CRL &= ~(0xB<< (5 * 4)); // (SCK)+
    GPIOA->CRL |= (0xB << (5 * 4));  

    GPIOA->CRL &= ~(0x3<< (0 * 4)); // (NSS)+
    GPIOA->CRL |= (0x3 << (0 * 4)); 

    GPIOA->CRL &= ~(0x4 << (6 * 4)); // (MISO)+
    GPIOA->CRL |= (0x4 << (6 * 4)); 

    GPIOA->CRL &= ~ (0x3 << (7 * 4)); // (MOSI)+
    GPIOA->CRL |= (0x3 << (7 * 4)); 
}

void lora_ctrl_gpio_init()
{
    GPIOB->CRL &= ~ (0x3 << (1 * 4)); // (RST)+
    GPIOB->CRL |= (0x3 << (1 * 4)); 

    GPIOA->CRL &= ~ (0x4 << (1 * 4)); // (DIO0)+
    GPIOA->CRL |= (0x4 << (1 * 4)); 
}

// Initialize GPIOA pin as output for LED
void gpio_a_led_output_init(uint8_t pin){
    GPIOA->CRL &= ~(0xF << (pin * 4)); // Clear CNF[0:0] and MODE[0:0] bits
    GPIOA->CRL |= (0x1 << (pin * 4));  // Set PAx as output (push-pull)
    GPIOA->BSRR = (1 << (pin)); // Set PAx high (turn off LED)
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

void reset_pin_1(void){
    GPIOB->BRR = (1 << 1); // Set PA1 low
    timeout_start(&lora_timeout, 100);
    while(!timeout_has_expired(&lora_timeout));

    GPIOB->BSRR = (1 << 1); // Set PA1 high
    timeout_start(&lora_timeout, 10);
    while (!timeout_has_expired(&lora_timeout));
}