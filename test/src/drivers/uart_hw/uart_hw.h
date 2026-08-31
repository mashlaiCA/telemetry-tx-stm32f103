#ifndef UART_HW_H
#define UART_HW_H

#include <stdint.h>
/**
 * @file uart_hw.h
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

void uart_hw_init(void);

/**
 * @brief Send a single byte over UART.
 * This function waits until the transmit data register is empty and then sends the byte.
 * This function performs the following steps:
 * 1. Wait until the TXE (Transmit Data Register Empty) flag is set.
 * 2. Write the byte to the USART data register (DR).
 */
void uart_hw_send_byte(uint8_t byte);

#endif
