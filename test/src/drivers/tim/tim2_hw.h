#ifndef TIM2_H
#define TIM2_H

#include "stdint.h"

extern volatile uint32_t sys_ms; // System milliseconds counter
extern volatile uint64_t sys_us; 

/**
 * @file tim2.h
 * @brief Driver for TIM2 timer on STM32F103xx microcontroller.
 * This module provides functions to initialize TIM2 for system timekeeping,
 * update the system time, and check for elapsed time intervals.
 */

/**
 * @brief Initialize TIM2 for system timekeeping.
 * This function performs the following steps:
 * 1. Enable the TIM2 clock in the RCC APB1ENR register.
 * 2. Configure the prescaler (PSC) to achieve a 1 ms tick rate assuming an 8 MHz clock.
 * 3. Set the auto-reload register (ARR) to the specified time_ms value.
 * 4. Reset the counter (CNT) to 0.
 * 5. Enable the TIM2 counter.
 * @param time_ms The time interval in milliseconds for the timer.
 */
void timer_init(void);

void delay_hw_ms(uint32_t ms);
void delay_hw_us(uint32_t us);
uint32_t micros_hw(void);
uint32_t millis_hw(void);
#endif
