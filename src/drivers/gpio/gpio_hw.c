#include "gpio_hw.h"
#include "../tim/tim2_hw.h"
#include "stm32f103xb.h"


/* Enables the GPIOA clock. */
void gpio_A_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // APB2ENR.IOPAEN: GPIOA clock on
}

/* Enables the GPIOB clock. */
void gpio_B_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // APB2ENR.IOPBEN: GPIOB clock on (AFIO clock is not touched)
}

/* Configures one PB0..PB7 pin as I2C alternate function open-drain. */
void gpio_B_init_I2C_SDA_SCL(uint8_t pin)
{
    GPIOB->CRL &= ~(0xFu << (pin * 4)); // Clear CNF[1:0] and MODE[1:0] of the pin
    GPIOB->CRL |= (0xFu << (pin * 4));  // 0xF: CNF = 11 AF open-drain, MODE = 11 output 50 MHz
}

/* Configures one PA0..PA7 pin as analog input; other pin numbers are ignored. */
void gpio_a_analog_input_init(uint8_t pin)
{
    if (pin > 7u)
    {
        return; // Only CRL (pins 0..7) is handled
    }
    GPIOA->CRL &= ~(0xFu << (pin * 4)); // CNF = 00, MODE = 00: analog input
    GPIOA->CRL |= (0x0u << (pin * 4));  // No-op (analog mode is all zeros), kept for symmetry
}

/* Configures one PB0..PB7 pin as analog input; other pin numbers are ignored. */
void gpio_PBx_analog_input_init(uint8_t pin)
{
     if (pin > 7u)
    {
        return; // Only CRL (pins 0..7) is handled
    }
    GPIOB->CRL &= ~(0xFu << (pin * 4)); // CNF = 00, MODE = 00: analog input
}

/* Configures one PB8..PB15 pin as push-pull output, 2 MHz; pins 0..7 are ignored. */
void gpio_PBx_polarity_init(uint8_t pinB)
{

    if (pinB < 8u)
    {
        return; // Only CRH (pins 8..15) is handled
    }

    uint8_t s = (pinB - 8) * 4; // Bit offset of the pin's nibble in CRH
    GPIOB->CRH &= ~(0xFu << s); // Clear CNF/MODE
    GPIOB->CRH |=  (0x2u << s); // 0x2: CNF = 00 push-pull, MODE = 10 output 2 MHz
}

/* Switches a PB8..PB15 pair to push-pull outputs in a single CRH write, so both
   pins start driving at the same moment (the levels are set in ODR beforehand). */
static void gpio_PBx_polarity_pair_output(uint8_t pin1, uint8_t pin2)
{
    uint8_t s1 = (uint8_t)((pin1 - 8u) * 4u); // CRH nibble offset of pin1
    uint8_t s2 = (uint8_t)((pin2 - 8u) * 4u); // CRH nibble offset of pin2

    uint32_t crh = GPIOB->CRH; // Read-modify-write on a local copy

    crh &= ~((0xFu << s1) | (0xFu << s2)); // Clear CNF/MODE of both pins
    crh |= ((0x2u << s1) | (0x2u << s2)); // push-pull output, 2 MHz

    GPIOB->CRH = crh; // Apply both pins at once
}

/* Forward excitation on GPIOB: output levels are written to ODR first, then the
   pins are switched from Hi-Z to outputs, so no wrong level appears on the probe. */
void polarity_PBx_fwd(uint8_t pin1, uint8_t pin2){
    GPIOB->BSRR = (1u << pin1) | (1u << (pin2 + 16u)); // pin1 = 1, pin2 = 0
    gpio_PBx_polarity_pair_output(pin1, pin2);           // Enable both outputs
}

/* Reverse excitation on GPIOB (levels first, then outputs). */
void polarity_PBx_rev(uint8_t pin1, uint8_t pin2){
    GPIOB->BSRR = (1u << pin2) | (1u << (pin1 + 16u)); // pin2 = 1, pin1 = 0
    gpio_PBx_polarity_pair_output(pin1, pin2);           // Enable both outputs
}


/* Releases a PB8..PB15 pair: both pins become floating inputs (Hi-Z). */
void polarity_PBx_off(uint8_t pin1, uint8_t pin2)
{
    uint8_t s1 = (uint8_t)((pin1 - 8u) * 4u); // CRH nibble offset of pin1
    uint8_t s2 = (uint8_t)((pin2 - 8u) * 4u); // CRH nibble offset of pin2

    uint32_t crh = GPIOB->CRH; // Read-modify-write on a local copy

    crh &= ~((0xFu << s1) | (0xFu << s2)); // Clear CNF/MODE of both pins
    crh |= ((0x4u << s1) | (0x4u << s2));  // 0x4: CNF = 01 floating input, MODE = 00

    GPIOB->CRH = crh; // Release both pins at once
}

/* Configures one PA0..PA7 pin as push-pull output, 2 MHz; other pins are ignored. */
void gpio_A_polarity_init(uint8_t pin1){

     if (pin1 > 7u)
    {
        return; // Only CRL (pins 0..7) is handled
    }
    GPIOA -> CRL &= ~(0xFu << pin1 * 4); // Clear CNF/MODE
    GPIOA -> CRL |= (0x2u << pin1 * 4);  // 0x2: push-pull output, 2 MHz
}

/* Switches a PA0..PA7 pair to push-pull outputs in a single CRL write. */
static void gpio_A_polarity_pair_output(uint8_t pin1, uint8_t pin2)
{
    uint8_t s1 = (uint8_t)(pin1 * 4u); // CRL nibble offset of pin1
    uint8_t s2 = (uint8_t)(pin2 * 4u); // CRL nibble offset of pin2

    uint32_t crl = GPIOA->CRL; // Read-modify-write on a local copy

    crl &= ~((0xFu << s1) | (0xFu << s2)); // Clear CNF/MODE of both pins
    crl |= ((0x2u << s1) | (0x2u << s2)); // push-pull output, 2 MHz

    GPIOA->CRL = crl; // Apply both pins at once
}

/* Forward excitation on GPIOA (levels first, then outputs). */
void polarity_fwd(uint8_t pin1, uint8_t pin2){
    GPIOA->BSRR = (1u << pin1) | (1u << (pin2 + 16u)); // pin1 = 1, pin2 = 0
    gpio_A_polarity_pair_output(pin1, pin2);             // Enable both outputs
}

/* Reverse excitation on GPIOA (levels first, then outputs). */
void polarity_rev(uint8_t pin1, uint8_t pin2){
    GPIOA->BSRR = (1u << pin2) | (1u << (pin1 + 16u)); // pin2 = 1, pin1 = 0
    gpio_A_polarity_pair_output(pin1, pin2);             // Enable both outputs
}

/* Releases a PA0..PA7 pair: both pins become floating inputs (Hi-Z). */
void polarity_off(uint8_t pin1, uint8_t pin2)
{
    uint8_t s1 = (uint8_t)(pin1 * 4u); // CRL nibble offset of pin1
    uint8_t s2 = (uint8_t)(pin2 * 4u); // CRL nibble offset of pin2

    uint32_t crl = GPIOA->CRL; // Read-modify-write on a local copy

    crl &= ~((0xFu << s1) | (0xFu << s2)); // Clear CNF/MODE of both pins
    crl |= ((0x4u << s1) | (0x4u << s2));  // 0x4: floating input

    GPIOA->CRL = crl; // Release both pins at once
}

/* PA1 = NTC voltage divider, ADC channel 1. */
void ntc_gpio_init(void){
   GPIOA->CRL &= ~(0xF << (1 * 4)); //PA1 Analog (CNF = 00, MODE = 00)
}

/* SPI1 pins for the SX1276 (NSS is driven manually as a GPIO). */
void gpio_SPI_init(void)
{
    GPIOA->CRL &= ~(0xF << (5 * 4));// Clear mode bits for PA5 (SCK)
    GPIOA->CRL |= (0xB << (5 * 4));// 0xB: PA5 alternate function push-pull, 50 MHz

    GPIOB->CRH &= ~(0xF << (4 * 4));// Clear mode bits for PB12 (NSS) - CRH nibble 4 = pin 12
    GPIOB->CRH |= (0x3 << (4 * 4));// 0x3: PB12 general purpose push-pull output, 50 MHz

    GPIOA->CRL &= ~(0xF << (6 * 4));// Clear mode bits for PA6 (MISO)
    GPIOA->CRL |= (0x8 << (6 * 4));// 0x8: PA6 input with pull-up/pull-down (direction set by ODR6), not floating

    GPIOA->CRL &= ~(0xF << (7 * 4)); // Clear mode bits for PA7 (MOSI)
    GPIOA->CRL |= (0xB << (7 * 4));// 0xB: PA7 alternate function push-pull, 50 MHz
}

/* SX1276 control pins: RST output and DIO0 input. */
void lora_ctrl_gpio_init(void)
{
    GPIOB->CRL &= ~(0xF << (5 * 4));  // Clear mode bits for PB5 (RST)
    GPIOB->CRL |= (0x3 << (5 * 4)); // 0x3: PB5 general purpose push-pull output, 50 MHz

    GPIOB->CRL &= ~(0xF << (1 * 4)); // Clear mode bits for PB1 (DIO0)
    GPIOB->CRL |= (0x8 << (1 * 4));  // CNF=10, MODE=00 -> input with pull resistor
    GPIOB->BRR = (1 << 1);           // ODR=0 selects pull-DOWN: DIO0 reads 0 if the line is open
}

/* Reads one input pin of any port. */
uint8_t gpio_read_pin(GPIO_TypeDef* GPIOx, uint8_t pin)
{
    return (GPIOx->IDR & (1 << pin)) ? 1 : 0; // IDR bit -> 0/1
}

/* Asserts SX1276 reset (active low) and holds it for 10 ms. */
void rst_low(void)
{
    GPIOB->BRR = (1 << 5); // PB5 = 0
    delay_hw_ms(10);       // Reset pulse width (SX1276 datasheet requires > 100 us)
}

/* Releases SX1276 reset and waits 10 ms for the chip to become ready. */
void rst_high(void)
{
    GPIOB->BSRR = (1 << 5); // PB5 = 1
    delay_hw_ms(10);        // Startup time after reset (SX1276 datasheet: 5 ms)
}

/* Selects the SX1276 on SPI (NSS active low). */
void nss_low(void)
{
    GPIOB->BRR = (1 << 12); // PB12 = 0
}

/* Deselects the SX1276. */
void nss_high(void)
{
    GPIOB->BSRR = (1 << 12); // PB12 = 1
}

/* Level of SX1276 DIO0 (TX done / RX done interrupt line). */
uint8_t dio0_read(void)
{
    return (GPIOB->IDR & (1 << 1)) ? 1 : 0; // PB1 IDR bit
}