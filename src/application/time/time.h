#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include "drivers/timeout_hw/timeout_hw.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @file time.h
     * @brief Time management module for STM32F103xx microcontroller.
     * This module provides functions to manage system time and delays.
     */

    /**
     * @brief Start the system timer.
     */
    void timer_start(void);

    /**
     * @brief Set the timer timeout value.
     * @param t Pointer to the timeout structure.
     * @param timeout_ms Timeout value in milliseconds.
     */
    void timer_set(timeout_t *t, uint32_t timeout_ms);

    /**
     * @brief Wait for the timer to expire.
     * @param t Pointer to the timeout structure.
     * @return 1 if the timer has expired, 0 otherwise.
     */
    uint8_t timer_wait(timeout_t *t);

    void delay_ms(uint32_t ms);
    void delay_us(uint32_t us);
    uint32_t millis_time();
    uint32_t micros_time();

#ifdef __cplusplus
}
#endif

#endif // TIME_H