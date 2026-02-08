#ifndef TIMEOUT_HW_H
#define TIMEOUT_HW_H

#include "stdint.h"

/**
 * @file timeout_hw.h
 * @brief Hardware timeout management for STM32F103xx microcontroller.
 * This module provides functions to start a timeout and check if it has expired.
 */

 /**
  * @brief Structure to hold timeout information
  * This structure contains the start time in milliseconds and the timeout duration in milliseconds.
  */
typedef struct
{
    uint32_t start_ms; // Start time in milliseconds
    uint32_t timeout_ms; // Timeout duration in milliseconds
} timeout_t;

/**
 * @brief Start a timeout.
 * This function initializes the timeout structure with the current system time and the specified timeout duration.
 * @param t Pointer to the timeout_t structure to initialize.
 * @param timeout_ms Timeout duration in milliseconds.
 */
void timeout_start(timeout_t *t, uint32_t timeout_ms);

/**
 * @brief Check if the timeout has expired.
 * This function checks if the current system time has exceeded the start time plus the timeout duration.
 * @param t Pointer to the timeout_t structure to check.
 * @return returns 1 if the timeout has expired, 0 otherwise.
 */
uint8_t timeout_has_expired(timeout_t *t);

#endif
