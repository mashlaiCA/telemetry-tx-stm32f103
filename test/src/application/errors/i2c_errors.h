#ifndef I2C_ERRORS_H
#define I2C_ERRORS_H
#include "../../drivers/I2C/i2c_hw.h"
#include "../../drivers/sht35/sht35.h"

/**
 * @file i2c_errors.h
 * @brief I2C error handling for STM32F103xx microcontroller.
 * This module provides functions to manage and retrieve I2C error statuses.
 */

/**
 * @brief Get the last I2C error status.
 * @return The last I2C error status as an I2C_Status_t value.
 */
I2C_Status_t get_last_i2c_error(void);

#endif