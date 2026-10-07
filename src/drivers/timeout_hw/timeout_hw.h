/**
 * @file  timeout_hw.h
 * @brief Non-blocking software timeouts based on the sys_ms millisecond clock (TIM2).
 * This file provides:
 * 1. The timeout_t structure (start time + duration).
 * 2. timeout_start() - arm a timeout.
 * 3. timeout_has_expired() - poll it.
 * Elapsed time is computed as an unsigned difference, so sys_ms wrap-around
 * (every ~49.7 days) is handled for durations below 2^32 ms.
 */

#ifndef TIMEOUT_HW_H
#define TIMEOUT_HW_H

#include "stdint.h"

/**
 * @brief Software timeout state.
 * Holds the sys_ms value when the timeout was armed and its duration.
 */
typedef struct
{
  uint32_t start_ms;   // sys_ms value when the timeout was armed
  uint32_t timeout_ms; // Duration in milliseconds
} timeout_t;

/**
 * @brief Arms a timeout.
 * This function performs the following steps:
 * 1. Stores the current sys_ms as the start time.
 * 2. Stores the duration.
 * @param t          Timeout to arm.
 * @param timeout_ms Duration in milliseconds; 0 makes the timeout expire immediately.
 */
void timeout_start(timeout_t *t, uint32_t timeout_ms);

/**
 * @brief Checks whether a timeout has expired.
 * @param t Timeout armed with timeout_start().
 * @return 1 if at least timeout_ms milliseconds have passed since timeout_start(), 0 otherwise.
 */
uint8_t timeout_has_expired(timeout_t *t);

#endif
