/**
 * @file  uart.h
 * @brief Text output over USART1 for debugging, plus helpers that format numbers into buffers.
 * This file provides:
 * 1. uart_init() / uart_flush() - initialization and wait-for-idle before Stop mode.
 * 2. Direct output: strings, integers, uint16_t values, floats with one decimal.
 * 3. pack_*() helpers that write text into a caller's buffer and return the
 *    position after the last character (no terminating '\0' is written).
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initializes USART1 (9600 baud 8N1, PA9 TX / PA10 RX).
     */
    void uart_init(void);

    /**
     * @brief Sends a null-terminated string followed by '\n' (LF only).
     * @param str String to send.
     */
    void uart_send_string(const char *str);

    /**
     * @brief Blocks until the UART has finished shifting out the last byte.
     * Must be called before entering Stop mode, which stops the USART clock.
     */
    void uart_flush(void);

    /**
     * @brief Sends a uint16_t value as decimal text, without separator or line ending.
     * @param value Value to send, 0..65535.
     */
    void uart_send_uint16_t(uint16_t value);

    /**
     * @brief Sends four uint16_t values as decimal text on one line.
     * Each value is preceded by a space, the line ends with '\n'.
     * A value of 0 produces no digits (only the space).
     * @param value1 First value.
     * @param value2 Second value.
     * @param value3 Third value.
     * @param value4 Fourth value.
     */
    void uart_send_uint16_t2(uint16_t value1,
                             uint16_t value2,
                             uint16_t value3,
                             uint16_t value4);

    void uart_send_line(const char *str); // Sends str + '\n' (from uart_send_string()) + "\r\n"

    void uart_print_float1(float v); // Sends v with one decimal (rounded), no line ending

    void uart_print_int_raw(int32_t value); // Sends value as decimal text, no line ending

    void uart_print_int(int value); // Sends value as decimal text followed by "\r\n"

    char *pack_int(char *p, int32_t value); // Writes value as decimal text at p; returns the position after it

    char *pack_float1(char *p, float v); // Writes v with one decimal at p; returns the position after it

    char *pack_str(char *p, const char *s); // Copies s (without '\0') to p; returns the position after it


#ifdef __cplusplus
}
#endif

#endif // UART_H