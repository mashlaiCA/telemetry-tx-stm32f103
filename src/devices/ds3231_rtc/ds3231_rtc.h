/**
 * @file  ds3231_rtc.h
 * @brief DS3231 real-time clock (I2C address 0x68): date/time and Alarm1 used as the Stop-mode wakeup source.
 * This file provides:
 * 1. ds3231_time_t - date/time in binary (24-hour mode).
 * 2. ds3231_init(), ds3231_lost_power(), ds3231_set_time(), ds3231_get_time().
 * 3. Alarm1 control: ds3231_set_alarm_at() (absolute hh:mm:ss), ds3231_set_alarm_in()
 *    (relative), ds3231_alarm_flag(), ds3231_clear_alarm_flag().
 * 4. ds3231_datetime_to_str() - human-readable text for debugging.
 * The INT/SQW output (open-drain, active low) is connected to PA0 / EXTI0. It stays
 * low while A1F is set, so A1F must be cleared after every alarm, otherwise no new
 * falling edge (and no wakeup) can occur.
 */

#ifndef DS3231_RTC_H
#define DS3231_RTC_H

#include <stdint.h>
#include "drivers/I2C/i2c_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Date and time in binary form (the driver converts to/from BCD). */
typedef struct
{
    uint8_t sec;   // Seconds, 0..59
    uint8_t min;   // Minutes, 0..59
    uint8_t hour;  // Hours, 0..23 (24-hour mode)
    uint8_t day;   // Day of week, 1..7
    uint8_t date;  // Day of month, 1..31
    uint8_t month; // Month, 1..12 (century bit ignored)
    uint8_t year;  // Year within the century, 0..99 (printed as 20YY)
} ds3231_time_t;

/**
 * @brief Prepares the DS3231 for alarm-driven wakeup.
 * This function performs the following steps:
 * 1. Control register = INTCN only: INT/SQW outputs alarm interrupts (no square
 *    wave), both alarm interrupts disabled, oscillator enabled.
 * 2. Status register: clears EN32kHz, A1F and A2F (OSF is kept).
 * @return i2c_ok on success; otherwise the status of the failed I2C transfer.
 */
I2C_Status_t ds3231_init(void);

/**
 * @brief Reports whether the RTC time is invalid.
 * @return 1 if the oscillator-stop flag (OSF) is set, i.e. the time was lost,
 *         or if the RTC cannot be read; 0 if the time is valid.
 */
uint8_t ds3231_lost_power(void);

/**
 * @brief Sets date and time and clears OSF.
 * This function performs the following steps:
 * 1. Writes seconds..year (registers 0x00..0x06) in BCD, 24-hour mode.
 * 2. Clears the oscillator-stop flag in the status register.
 * @param t Date/time in binary (fields in the ranges of ds3231_time_t).
 * @return i2c_ok on success; otherwise the status of the failed I2C transfer.
 */
I2C_Status_t ds3231_set_time(const ds3231_time_t *t);

/**
 * @brief Reads the current date and time.
 * @param t Receives the date/time (unchanged on failure).
 * @return i2c_ok on success; otherwise the status of the failed I2C transfer.
 */
I2C_Status_t ds3231_get_time(ds3231_time_t *t);
/**
 * @brief Arms Alarm1 to fire seconds_ahead seconds from now.
 * This function performs the following steps:
 * 1. Forces seconds_ahead to at least 2.
 * 2. Reads the current hh:mm:ss and adds seconds_ahead with minute/hour/day wrap.
 * 3. Calls ds3231_set_alarm_at() with the result (which also verifies A1F is clear).
 * Because the current position inside the second is unknown, the actual delay is
 * seconds_ahead - 1 .. seconds_ahead seconds.
 * @param seconds_ahead Delay in seconds (values below 2 are raised to 2).
 * @return i2c_ok if the alarm is armed and A1F is confirmed clear;
 *         i2c_error if A1F could not be cleared;
 *         otherwise the status of the failed I2C transfer.
 */
I2C_Status_t ds3231_set_alarm_in(uint8_t seconds_ahead);

/**
 * @brief Arms Alarm1 at an absolute time of day (hh:mm:ss, any date).
 * Used for a fixed cycle period: a target computed from the previous wakeup
 * (which happened exactly on a second boundary) gives a period of exactly
 * CYCLE_PERIOD_S seconds regardless of how long the active phase took, while
 * ds3231_set_alarm_in() counts from an arbitrary point inside a second.
 * This function performs the following steps:
 * 1. Writes the Alarm1 registers: match seconds, minutes and hours (A1M4 = 1: date ignored).
 * 2. Control = INTCN | A1IE (Alarm2 interrupt disabled).
 * 3. Clears A1F/A2F and reads the status back to confirm A1F is clear.
 * While A1F is set INT/SQW stays low, so no falling edge (wakeup) could occur.
 * @param hour 0..23.
 * @param min  0..59.
 * @param sec  0..59.
 * @return i2c_ok if the alarm is armed and A1F is confirmed clear;
 *         i2c_error if an argument is out of range (nothing written) or A1F is still set;
 *         otherwise the status of the failed I2C transfer.
 */
I2C_Status_t ds3231_set_alarm_at(uint8_t hour, uint8_t min, uint8_t sec);

/**
 * @brief Reads the Alarm1 flag (A1F).
 * @return 1 if A1F is set (or the RTC cannot be reached), 0 if it is clear.
 */
uint8_t ds3231_alarm_flag(void);

/**
 * @brief Clears A1F/A2F, releasing the INT/SQW line back to high.
 * Called right after waking up, so the next alarm can produce a fresh
 * falling edge on PA0. Does not write if both flags are already clear.
 * @return i2c_ok on success; otherwise the status of the failed I2C transfer.
 */
I2C_Status_t ds3231_clear_alarm_flag(void);

/**
 * @brief Formats date/time as "DD-MM-20YY  HH:MM:SS AM\r\n" (12-hour clock).
 * @param t   Date/time to format.
 * @param buf Output buffer, at least 26 bytes (25 characters + '\0').
 */
void ds3231_datetime_to_str(const ds3231_time_t *t, char *buf);

#ifdef __cplusplus
}
#endif

#endif
