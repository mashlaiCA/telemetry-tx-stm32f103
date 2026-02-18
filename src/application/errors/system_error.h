#ifndef SYSTEM_ERROR_H
#define SYSTEM_ERROR_H

#include "stdint.h"

/**
 * @file system_error.h
 * @brief Header file for system error management.
 * This module defines the enumeration for system error types and provides function prototypes for setting and getting the current system error state.
 * The system error management allows for centralized handling of errors across different components of the application, such as I2C communication errors and sensor-specific errors.
 * By using a global variable to store the current system error state, different parts of the application can set and check for errors in a consistent manner, facilitating debugging and error recovery processes.
 * The enumeration `System_Error_t` defines specific error types that can occur in the system, such as I2C errors and SHT35 sensor errors.
 * The functions `set_system_error` and `get_system_error` provide an interface for other modules to interact with the system error state, allowing for error reporting and retrieval of the current error status.
 */

/**
 * @brief Enumeration for system error types.
 * This enumeration defines the different types of errors that can occur in the system. Each error type is represented by a unique value, allowing for easy identification and handling of specific error conditions. The defined error types include:
 * - `system_error_none`: Indicates that there are no errors in the system.
 * - `system_error_i2c`: Represents an error related to I2C communication, which could occur during interactions with sensors or other I2C devices.
 * - `system_error_sht35`: Represents an error specific to the SHT35 sensor, which could occur during measurement or communication with the sensor.
 */

typedef enum
{
    system_error_none = 0, // No errors in the system
    system_error_i2c,      // Error related to I2C communication
    system_error_sht35,    // Error specific to the SHT35 sensor
} System_Error_t;

extern System_Error_t system_error_flags; // Global variable to hold the current system error state

void set_system_error(System_Error_t error); // Function prototype for setting the system error state
System_Error_t get_system_error(void); // Function prototype for getting the current system error state

#endif