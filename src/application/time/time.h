/**
 * @file  time.h
 * @brief Application-level time API: thin wrappers over the TIM2 time base and software timeouts.
 * This file provides:
 * 1. timer_start() - starts the 1 ms system tick.
 * 2. timer_set() / timer_wait() - non-blocking timeouts.
 * 3. delay_ms() / delay_us() / wm_delay_us() - blocking delays.
 * 4. millis_time() / micros_time() - current time (also used by the RadioLib HAL).
 */

#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include "drivers/timeout_hw/timeout_hw.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Starts the system time base (TIM2, 1 ms tick).
     */
    void timer_start(void);

    /**
     * @brief Arms a non-blocking timeout.
     * @param t          Timeout to arm.
     * @param timeout_ms Duration in milliseconds.
     */
    void timer_set(timeout_t *t, uint32_t timeout_ms);

    /**
     * @brief Polls a timeout armed with timer_set(). Does not block, despite the name.
     * @param t Timeout to check.
     * @return 1 if the timeout has expired, 0 otherwise.
     */
    uint8_t timer_wait(timeout_t *t);

    void delay_ms(uint32_t ms);       // Blocking delay in ms (needs the TIM2 interrupt; see delay_hw_ms())
    void wm_delay_us(uint16_t us);    // Blocking delay < 1000 us, works with interrupts disabled (see wm_delay_hw_us())
    void delay_us(uint32_t us);       // Blocking delay in us (see delay_hw_us())
    uint32_t millis_time();           // Milliseconds since start (+ compensated Stop time)
    uint32_t micros_time();           // Microseconds since start, 32-bit (wraps every ~71.6 min)

#ifdef __cplusplus
}
#endif

#endif // TIME_H