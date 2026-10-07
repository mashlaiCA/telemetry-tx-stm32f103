/**
 * @file  gpio_hw.h
 * @brief Register-level GPIO helpers for GPIOA/GPIOB (STM32F103 CRL/CRH configuration).
 * This file provides:
 * 1. Port clock enables (GPIOA, GPIOB).
 * 2. Pin mode setup: I2C open-drain AF, analog inputs, SPI1 pins, LoRa control pins.
 * 3. Polarity (H-bridge style) control of resistive probes: a pin pair is driven
 *    forward (pin1 = 1, pin2 = 0), reverse (pin1 = 0, pin2 = 1) or released (Hi-Z)
 *    - on GPIOA pins 0..7 (leaf sensor) and on GPIOB pins 8..15 (Watermark).
 * 4. LoRa SX1276 control: RST (PB5), NSS (PB12), DIO0 (PB1).
 * CNF/MODE nibbles used in this module (RM0008 9.2.1/9.2.2):
 * 0x0 analog input, 0x2 push-pull output 2 MHz, 0x3 push-pull output 50 MHz,
 * 0x4 floating input, 0x8 input with pull-up/pull-down (selected by ODR),
 * 0xB AF push-pull 50 MHz, 0xF AF open-drain 50 MHz.
 */

#ifndef GPIO_H
#define GPIO_H

#include "stdint.h"
#include "stm32f103xb.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Enables the GPIOA clock (RCC APB2ENR.IOPAEN). */
void gpio_A_init(void);

/** @brief Enables the GPIOB clock (RCC APB2ENR.IOPBEN). The AFIO clock is NOT enabled here. */
void gpio_B_init(void);

/** @brief Configures one GPIOB pin for I2C (alternate function open-drain, 50 MHz).
 *  This function performs the following steps:
 *  1. Clears CNF/MODE of the pin in GPIOB->CRL.
 *  2. Sets them to 0xF (CNF = 11 AF open-drain, MODE = 11 50 MHz).
 *  @param pin GPIOB pin number, 0..7 (only CRL is written; no range check).
 */
void gpio_B_init_I2C_SDA_SCL(uint8_t pin);

/** @brief Configures one GPIOA pin as analog input.
 *  This function performs the following steps:
 *  1. Returns without changes if pin > 7.
 *  2. Clears CNF/MODE of the pin in GPIOA->CRL (0x0 = analog input).
 *  The GPIOA clock must already be enabled.
 *  @param pin GPIOA pin number, 0..7.
 */
void gpio_a_analog_input_init(uint8_t pin);

/** @brief Configures one GPIOB pin (0..7) as analog input; ignored if pin > 7. */
void gpio_PBx_analog_input_init(uint8_t pin);

/** @brief Configures PA1 (NTC divider) as analog input. */
void ntc_gpio_init(void);

/** @brief Configures one GPIOB pin (8..15) as push-pull output, 2 MHz; ignored if pinB < 8. */
void gpio_PBx_polarity_init(uint8_t pinB);

/** @brief Drives a GPIOB pair forward: pin1 = 1, pin2 = 0, both push-pull outputs.
 *  @param pin1 GPIOB pin 8..15.
 *  @param pin2 GPIOB pin 8..15 (no range check: values below 8 give an invalid shift).
 */
void polarity_PBx_fwd(uint8_t pin1, uint8_t pin2);

/** @brief Drives a GPIOB pair in reverse: pin1 = 0, pin2 = 1, both push-pull outputs.
 *  @param pin1 GPIOB pin 8..15.
 *  @param pin2 GPIOB pin 8..15.
 */
void polarity_PBx_rev(uint8_t pin1, uint8_t pin2);

/** @brief Releases a GPIOB pair (8..15): both pins become floating inputs (Hi-Z), no current through the probe. */
void polarity_PBx_off(uint8_t pin1, uint8_t pin2);

/** @brief Configures one GPIOA pin (0..7) as push-pull output, 2 MHz; ignored if pin1 > 7. */
void gpio_A_polarity_init(uint8_t pin1);

/** @brief Drives a GPIOA pair (0..7) forward: pin1 = 1, pin2 = 0, both push-pull outputs. */
void polarity_fwd(uint8_t pin1, uint8_t pin2);

/** @brief Drives a GPIOA pair (0..7) in reverse: pin1 = 0, pin2 = 1, both push-pull outputs. */
void polarity_rev(uint8_t pin1, uint8_t pin2);

/** @brief Releases a GPIOA pair (0..7): both pins become floating inputs (Hi-Z). */
void polarity_off(uint8_t pin1, uint8_t pin2);


/** @brief Reads a GPIO input.
 *  @param GPIOx Port (GPIOA, GPIOB, ...).
 *  @param pin   Pin number 0..15.
 *  @return 1 if the pin reads high, 0 if low.
 */
uint8_t gpio_read_pin(GPIO_TypeDef* GPIOx, uint8_t pin);

void gpio_SPI_init(void);// SPI1 pins: PA5 SCK, PA7 MOSI (AF push-pull), PA6 MISO (input), PB12 NSS (GP output)
void lora_ctrl_gpio_init(void); // PB5 RST (push-pull output), PB1 DIO0 (input with pull-down)
void rst_low(void);   // PB5 = 0 (SX1276 in reset), then 10 ms delay
void rst_high(void);  // PB5 = 1 (SX1276 out of reset), then 10 ms delay
void nss_low(void);   // PB12 = 0: select SX1276 on SPI
void nss_high(void);  // PB12 = 1: deselect SX1276
uint8_t dio0_read(void); // Level of PB1 (SX1276 DIO0): 1 = high, 0 = low


#ifdef __cplusplus
}
#endif

#endif //
