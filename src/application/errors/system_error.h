/**
 * @file  system_error.h
 * @brief Latched system-level error code.
 * This file provides:
 * 1. The System_Error_t enumeration.
 * 2. The global system_error_flags.
 * 3. set_system_error() - records an error only if none is recorded yet.
 * 4. get_system_error() - reads the recorded error.
 * Only the FIRST error is kept (useful for debugging the root cause); there is
 * no function that clears it.
 */

#ifndef SYSTEM_ERROR_H
#define SYSTEM_ERROR_H

#include "stdint.h"

/**
 * @brief System error codes.
 * Values are exclusive codes, not bit flags: only one error is stored.
 */

typedef enum
{
    system_error_none = 0, // No error recorded
    system_error_i2c,      // I2C communication error (currently not set anywhere in the code)
    system_error_sht35,    // SHT35 disabled after sht_max_retry consecutive failures
} System_Error_t;

extern System_Error_t system_error_flags; // First recorded system error (system_error_none if none)

/**
 * @brief Records a system error.
 * This function performs the following steps:
 * 1. Stores the error only if no error is recorded yet, so the first
 *    (root-cause) error is preserved.
 * @param error The error code to record.
 */
void set_system_error(System_Error_t error);

/**
 * @brief Returns the recorded system error.
 * @return The first error passed to set_system_error(), or system_error_none.
 */
System_Error_t get_system_error(void); // Read the latched error code

#endif