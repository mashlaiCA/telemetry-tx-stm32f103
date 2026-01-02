#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);

/**
 * @brief Send a string over UART.
 * @param str Pointer to the null-terminated string to send.
 */
void uart_send_string(const char *str);

/**
 * @brief Send a uint16_t value over UART as a string.
 * @param value The uint16_t value to send.
 */
void uart_send_uint16_t(uint16_t value);

#endif // UART_H    