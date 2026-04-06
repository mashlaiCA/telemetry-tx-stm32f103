#ifndef UART_H
#define UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @file uart.h
     * @brief UART communication interface for STM32F103xx microcontroller.
     * This module provides functions to initialize UART and send data over UART.
     */

    /**
     * @brief Initialize UART communication.
     */
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

    /**
     * @brief Send multiple uint16_t values over UART as strings.
     * This function sends four uint16_t values over UART, separated by spaces.
     * @param value1 The first uint16_t value to send.
     * @param value2 The second uint16_t value to send.
     * @param value3 The third uint16_t value to send.
     * @param value4 The fourth uint16_t value to send.
     */
    void uart_send_uint16_t2(uint16_t value1,
                             uint16_t value2,
                             uint16_t value3,
                             uint16_t value4);

#ifdef __cplusplus
}
#endif

#endif // UART_H