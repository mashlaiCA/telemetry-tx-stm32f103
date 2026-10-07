/**
 * @file  i2c_errors.h
 * @brief Read access to the detailed status of the last I2C transfer.
 * This file provides:
 * 1. get_last_i2c_error() - returns i2c_status_error from the I2C driver.
 * The status is shared by all devices on the bus (SHT35, DS3231), so it only
 * describes the most recent transfer, whichever device it was for.
 */

#ifndef I2C_ERRORS_H
#define I2C_ERRORS_H
#include "../../drivers/I2C/i2c_hw.h"
#include "../../drivers/sht35/sht35.h"

/**
 * @brief Returns the detailed status of the last I2C transfer.
 * @return i2c_status_error: i2c_ok after a successful transfer, otherwise the
 *         specific cause (see I2C_Status_t), i2c_bus_recovered / i2c_bus_stuck
 *         after a recovery.
 */
I2C_Status_t get_last_i2c_error(void);

#endif