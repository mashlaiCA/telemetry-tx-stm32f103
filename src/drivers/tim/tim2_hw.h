/**
 * @file  tim2_hw.h
 * @brief TIM2-based system time base: 1 ms tick, millisecond/microsecond clocks and busy-wait delays.
 * This file provides:
 * 1. The software clocks sys_ms / sys_us, advanced by the TIM2 update interrupt every 1 ms.
 * 2. timer_init() - TIM2 setup (1 MHz counter, 1000 counts per update).
 * 3. millis_hw() / micros_hw() - current time.
 * 4. delay_hw_ms() / delay_hw_us() / wm_delay_hw_us() - blocking delays with loop guards.
 * 5. sys_time_add_ms() - adds time spent in Stop mode, when TIM2 is not clocked.
 * Assumes TIM2 is clocked at 8 MHz (HSI 8 MHz, no PLL, APB1 prescaler 1).
 */

#ifndef TIM2_H
#define TIM2_H

#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern volatile uint32_t sys_ms; // Milliseconds since timer_init() (+ Stop time added by sys_time_add_ms()); written in TIM2_IRQHandler
extern volatile uint64_t sys_us; // Microseconds at the last 1 ms tick (multiple of 1000); written in TIM2_IRQHandler

/**
 * @brief Initializes TIM2 as the 1 ms system tick.
 * This function performs the following steps:
 * 1. Enables the TIM2 clock (RCC APB1ENR.TIM2EN) and pulses the TIM2 reset (APB1RSTR.TIM2RST).
 * 2. PSC = 7: 8 MHz / 8 = 1 MHz counter clock (1 count = 1 us).
 * 3. ARR = 999: update event every 1000 counts = 1 ms.
 * 4. Generates an update event (EGR.UG) so PSC is loaded immediately, then clears UIF.
 * 5. Enables the update interrupt (DIER.UIE) and TIM2_IRQn in the NVIC.
 * 6. Starts the counter (CR1.CEN).
 */
void timer_init(void);

/**
 * @brief Blocking delay in milliseconds, based on sys_ms.
 * Requires the TIM2 interrupt to be running. A loop counter of
 * (ms + 1) * 2000 iterations ends the wait early if sys_ms stops advancing
 * (e.g. interrupts disabled), so the function cannot hang.
 * @param ms Delay in milliseconds.
 */
void delay_hw_ms(uint32_t ms);
/**
 * @brief Blocking delay in microseconds, based on micros_hw().
 * Works with interrupts disabled for delays below 1 ms (micros_hw() accounts for
 * one pending update). A loop counter of (us + 1) * 20 iterations bounds the wait.
 * @param us Delay in microseconds.
 */
void delay_hw_us(uint32_t us);
/**
 * @brief Short blocking delay that reads TIM2->CNT directly.
 * Does not depend on interrupts, so it is usable inside critical sections
 * (Watermark excitation, I2C bus recovery). A loop counter of 4 * (us + 4)
 * iterations bounds the wait.
 * @param us Delay in microseconds, must be below 1000 (one TIM2 period).
 */
void wm_delay_hw_us(uint16_t us);
/**
 * @brief Current time in microseconds (wraps every ~71.6 minutes).
 * Reads sys_us and TIM2->CNT atomically and accounts for an update event that
 * is pending but not yet handled.
 * @return Microseconds since timer_init(), truncated to 32 bits.
 */
uint32_t micros_hw(void);
/**
 * @brief Current time in milliseconds.
 * @return sys_ms (wraps every ~49.7 days).
 */
uint32_t millis_hw(void);

/**
 * @brief Advances the software clocks by ms milliseconds.
 * TIM2 is clocked from APB1 and therefore stops in Stop mode, so the sleep
 * interval never reaches sys_ms/sys_us. Calling this after a wakeup keeps
 * millis_hw()/micros_hw() monotonic and close to wall-clock time.
 * @param ms Milliseconds spent with the timer stopped.
 */
void sys_time_add_ms(uint32_t ms);

/**
 * @brief Test helper: 100 us pulse on PB0 to check wm_delay_hw_us() with a scope.
 * PB0 must already be configured as an output.
 */
void test_wm_100us(void);

#ifdef __cplusplus
}
#endif

#endif
