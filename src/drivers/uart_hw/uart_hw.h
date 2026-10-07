/**
 * @file  uart_hw.h
 * @brief Register-level USART1 driver (PA9 = TX, PA10 = RX), 9600 baud 8N1, transmit only by polling.
 * This file provides:
 * 1. uart_hw_init() - clock, pin and baud-rate setup.
 * 2. uart_hw_send_byte() - blocking single-byte transmit with a bounded wait.
 * 3. uart_hw_flush() - waits until the last byte has left the shift register.
 */

#ifndef UART_HW_H
#define UART_HW_H

#include <stdint.h>

/**
 * @brief Initializes USART1 for 9600 baud, 8 data bits, no parity, 1 stop bit.
 * This function performs the following steps:
 * 1. Enables the USART1 and GPIOA clocks (RCC APB2ENR.USART1EN, IOPAEN).
 * 2. PA9 (TX): alternate function push-pull, 10 MHz; PA10 (RX): floating input.
 * 3. BRR = 52.0625 (mantissa 52, fraction 1/16): 8 MHz / (16 * 52.0625) = 9604 baud (+0.04 %).
 * 4. Enables USART1, the transmitter and the receiver (CR1.UE, TE, RE).
 * Must be called before any other UART function.
 */

void uart_hw_init(void);

/**
 * @brief Sends one byte.
 * This function performs the following steps:
 * 1. Waits for SR.TXE (data register empty), at most 50000 loop iterations.
 * 2. Writes the byte to DR (also after a timeout, so the caller never hangs).
 * @param byte Byte to transmit.
 */
void uart_hw_send_byte(uint8_t byte);

/**
 * @brief Waits until the last byte has physically left the shift register.
 * Waits for SR.TC (not TXE), at most 50000 loop iterations. Required before
 * entering Stop mode, otherwise the USART clock is stopped mid-character and
 * the last byte is truncated.
 */
void uart_hw_flush(void);

#endif
