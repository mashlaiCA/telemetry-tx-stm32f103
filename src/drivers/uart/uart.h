#ifndef UART_H
#define UART_H

#include <stdint.h>
/**
 * @file uart.h
 * @brief Driver for UART communication on STM32F103xx microcontroller.
 * This module provides functions to initialize UART and send data over UART.
 */

/**
 * @brief Initialize UART peripheral.
 * This function performs the following steps:
 * 1. Enable APB2 clock for USART1 and GPIOA peripherals (RCC_APB2ENR_USART1EN and RCC_APB2ENR_IOPAEN)
 * 2. Configure PA9 (TX) and PA10 (RX) pins for UART functionality.
 * 3. Set the baud rate to 9600 bps assuming an 8 MHz PCLK2 clock.
 * 4. Enable USART1, transmitter, and receiver.
 * @note This function must be called before any UART communication.
 */

void uart_init(void);

/**
 * @brief Send a single byte over UART.
 * This function waits until the transmit data register is empty and then sends the byte.
 * This function performs the following steps:
 * 1. Wait until the TXE (Transmit Data Register Empty) flag is set.
 * 2. Write the byte to the USART data register (DR).
 */
void uart_send_byte(uint8_t byte);

/**
 * @brief Send string to zero-terminated string over UART.
 * This function performs the following steps:
 * 1. Iterate through each character in the string until the null terminator is reached.
 * 2. For each character, call uart_send_byte to send it over UART.
 * @note The input string must be null-terminated.
 */
void uart_send_string(const char *str);

/**
 * @brief Send a uint16_t value over UART as a string.
 * This function performs the following steps:
 * 1. Convert the uint16_t value to its string representation.
 * 2. Send each character of the string representation over UART using uart_send_byte.
 * @note This function handles values from 0 to 65535.
 */
void uart_send_uint16_t(uint16_t value);

#endif
