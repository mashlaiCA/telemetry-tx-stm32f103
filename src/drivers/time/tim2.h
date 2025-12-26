#ifndef TIM2_H
#define TIM2_H

#include "stdint.h"
/**
 * @file tim2.h
 * @brief Driver for TIM2 timer on STM32F103xx microcontroller.
 * This module provides functions to initialize TIM2 for system timekeeping,
 * update the system time, and check for elapsed time intervals.
 */
extern volatile uint32_t sys_ms; // System milliseconds counter

/**
 * @brief Initialize TIM2 for system timekeeping.
 * This function performs the following steps:
 * 1. Enable the TIM2 clock in the RCC_APB1ENR register (RCC_APB1ENR_TIM2EN).
 * 2. Set the prescaler (PSC) to divide the 8MHz clock to 2000 Hz (0.5 ms per tick).
 * 3. Set the auto-reload register (ARR) to 2 for 0.5 millisecond intervals.
 * 4. Reset the TIM2 counter (CNT) to 0.
 * 5. Clear the update interrupt flag (UIF) in the status register (SR).
 * 6. Enable TIM2 by setting the CEN bit in the control register (CR1).
 * @note This function should be called once during system initialization.
 */
void timer_init(void);

/**
 * @brief Update system time. Called in main loop.
 * This function performs the following steps:
 * 1. Check if the update interrupt flag (UIF) is set in the TIM2 status register (SR).
 * 2. If UIF is set, clear the UIF flag.
 * 3. Increment the system milliseconds counter (sys_ms) by 1.
 * @note This function should be called frequently in the main loop to ensure accurate timekeeping.
 */
void timer_tick(void);

/**
 * @brief Check if specified delay in milliseconds has elapsed.
 * This function performs the following steps:
 * 1. Compare the difference between the current system milliseconds (sys_ms) and the last recorded time.
 * 2. If the difference is greater than or equal to the specified delay (delay_ms):
 *    - Update the last recorded time to the current sys_ms.
 *    - Return 1 to indicate that the delay has elapsed.
 * 3. If the delay has not yet elapsed, return 0.
 * @param delay_ms The delay in milliseconds to check.
 * @return uint8_t Returns 1 if the specified delay has elapsed, otherwise returns 0.
 */
uint8_t timer_sec(uint32_t delay_ms);

#endif
