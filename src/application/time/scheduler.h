#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "stdint.h"

/**
 * @file scheduler.h
 * @brief Scheduler module for time management on STM32F103xx microcontroller.
 * This module provides functions to initialize the scheduler, update system time,
 * and check for elapsed time intervals using TIM2 peripheral.
 */

 /**
  * @brief Initialize the scheduler with specified time interval.
  * This function initializes TIM2 peripheral for system timekeeping.
  * @param time_scheduler_ms Time interval in milliseconds for the scheduler tick.
  */
void scheduler_init(uint32_t time_scheduler_ms);

/**
 * @brief Update the scheduler tick.
 * This function should be called periodically to update the system time.
 * It calls the timer_tick function to increment the system millisecond counter.
 */
void scheduler_tick(void);

/** 
 * @brief Check if the specified time interval has elapsed.
 * This function compares the current system time with the provided time interval.
 * if the interval has elapsed, it returns 1 (true); otherwise, it returns 0 (false).
 * @param time_ms The time interval in milliseconds to check against the system time.
 * @return uint8_t Returns 1 if the specified time interval has elapsed, 0 otherwise.
 */
uint8_t scheduler_delay_ms(uint32_t time_ms);


#endif