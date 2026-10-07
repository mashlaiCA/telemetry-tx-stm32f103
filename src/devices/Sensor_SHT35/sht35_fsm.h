/**
 * @file  sht35_fsm.h
 * @brief Non-blocking finite state machine (FSM) for the SHT35 air temperature/humidity sensor.
 * This file provides:
 * 1. The FSM state enumeration SHT35_State_t.
 * 2. The state-handler function type and one handler per state.
 * 3. SHT35_FSM_Run(), which executes one FSM step per call from the main loop.
 * 4. SHT35_OnEnter(), which arms the per-state timers on a state change.
 * The FSM starts a single-shot measurement whenever system_data has consumed the
 * previous result (DATA_SHT35_READY cleared), waits for the conversion, reads and
 * CRC-checks the data, converts it, and publishes it by setting DATA_SHT35_READY.
 * Consecutive errors trigger a soft reset; after sht_max_retry consecutive errors
 * the sensor is disabled and a recovery attempt is made every 5 minutes.
 */

#ifndef SHT35_FSM_H
#define SHT35_FSM_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief States of the SHT35 FSM.
 * The numeric values are used as indices into state_table[] in sht35_fsm.c,
 * so st_count must stay the last member.
 */
typedef enum
{
    st_idle = 0,              // Waiting until system_data consumes the last result (DATA_SHT35_READY cleared)
    st_start_sensor,          // Send the single-shot measurement command
    st_wait_measurement,      // Wait for the conversion to finish (20 ms timer)
    st_read_data_measurement, // Read 6 raw bytes: T(2) + CRC, RH(2) + CRC
    st_crc_check,             // Verify CRC-8 of both words and extract raw T/RH
    st_calculate_data,        // Convert raw values to degC / %RH and publish them
    st_error,                 // Count a consecutive failure and choose restart or disable
    st_disable,               // Sensor disabled after too many failures; retried every 5 min
    st_restart_sensor,        // Send soft reset; on failure try to recover the I2C bus

    st_count     // Number of states (size of state_table[])
} SHT35_State_t; // FSM state type

/**
 * @brief State handler type.
 * Each handler executes the logic of one state and returns the state the FSM
 * must be in on the next call (which may be the same state).
 */
typedef SHT35_State_t (*StateFunction_t)(void);

extern SHT35_State_t sht35_state; // Initialized to st_idle; not used by the FSM itself (the state lives in sht35_context)

/**
 * @brief Idle state: decides when to start a new measurement.
 * This function performs the following steps:
 * 1. Checks DATA_SHT35_READY in system_data.ready_sensors_flag.
 * 2. If the flag is clear (previous result already consumed), requests a new measurement.
 * The 50 ms timer armed by SHT35_OnEnter(st_idle) is currently NOT checked
 * (the check is commented out), so a new measurement starts on the first call
 * after the flag is cleared.
 * @return st_start_sensor if DATA_SHT35_READY is clear,
 *         st_idle while the previous result has not been consumed yet.
 */
SHT35_State_t State_Idle(void);

/**
 * @brief Sends the single-shot measurement command to the SHT35.
 * This function performs the following steps:
 * 1. Writes the measurement command over I2C (I2C_Write_Sensor_SHT35()).
 * 2. Checks the status of THIS transaction (not the global I2C status, which
 *    may hold an error from DS3231 on the same bus).
 * @return st_wait_measurement if the command was acknowledged,
 *         st_error on any I2C failure (bus busy, NACK, timeout).
 */
SHT35_State_t State_Start_Sensor(void);

/**
 * @brief Waits for the measurement conversion to finish.
 * This function performs the following steps:
 * 1. Checks the 20 ms timer armed by SHT35_OnEnter(st_wait_measurement).
 * 2. Moves on to reading once the timer has expired.
 * @return st_wait_measurement while the 20 ms have not elapsed,
 *         st_read_data_measurement once they have. This state never fails.
 */
SHT35_State_t State_Wait_Measurement(void);

/**
 * @brief Reads the raw measurement result from the sensor.
 * This function performs the following steps:
 * 1. Reads 6 bytes over I2C into the driver buffer (T MSB, T LSB, T CRC, RH MSB, RH LSB, RH CRC).
 * 2. Checks the status of this read transaction.
 * @return st_crc_check if the read succeeded,
 *         st_error on any I2C failure (bus busy, NACK, timeout).
 */
SHT35_State_t State_Read_Data_Measurement(void);

/**
 * @brief Verifies the CRC of the received data.
 * This function performs the following steps:
 * 1. Calls SHT35_CRC_Check(), which checks both CRC bytes and, on success,
 *    extracts the raw temperature and humidity words.
 * 2. Reads the result through get_last_sht35_error().
 * @return st_calculate_data if both CRCs match,
 *         st_error if either CRC is wrong.
 */
SHT35_State_t State_CRC_Check(void);

/**
 * @brief Converts the raw data and publishes the result.
 * This function performs the following steps:
 * 1. Calls SHT35_Calculate() to convert raw words to degC and %RH and range-check them.
 * 2. On success sets DATA_SHT35_READY in system_data.ready_sensors_flag.
 * 3. On success resets the consecutive-error counter.
 * @return st_idle after a successful conversion,
 *         st_error if a value is out of range.
 */
SHT35_State_t State_Calculate_Data(void);

/**
 * @brief Handles a failure reported by any other state.
 * This function performs the following steps:
 * 1. Increments the consecutive-error counter.
 * 2. While the counter is below sht_max_retry, requests a soft reset.
 * 3. Otherwise records system_error_sht35 via set_system_error(), resets the
 *    counter and disables the sensor.
 * @return st_restart_sensor while error count < sht_max_retry,
 *         st_disable once the limit is reached.
 */
SHT35_State_t State_Error(void);

/**
 * @brief Sends a soft reset to the SHT35.
 * This function performs the following steps:
 * 1. Writes the soft reset command over I2C.
 * 2. If the sensor does not acknowledge it, runs i2c_bus_recover() in case a
 *    slave is holding SDA low.
 * @return st_idle if the reset command was acknowledged,
 *         st_error if it failed (counted as another consecutive error).
 */
SHT35_State_t State_Restart_Sensor(void);

/**
 * @brief Disabled state, entered after sht_max_retry consecutive failures.
 * This function performs the following steps:
 * 1. Sets DATA_SHT35_READY on every call, so system_data_run() does not wait
 *    for this sensor; the packet then carries the last valid values.
 * 2. Does not access the I2C bus until the SHT35_RETRY_PERIOD_MS (5 min)
 *    timer armed by SHT35_OnEnter(st_disable) expires.
 * 3. On expiry re-arms the timer, clears the error counter and tries a soft reset.
 * @return st_disable while the retry period is running,
 *         st_restart_sensor when it is time for a recovery attempt.
 */
SHT35_State_t State_Disabled(void);

//void Sensor_FSM_Run(void);

/**
 * @brief Executes one step of the SHT35 FSM.
 * This function performs the following steps:
 * 1. If the state changed since the previous call, calls SHT35_OnEnter() for the new state.
 * 2. If the state is out of range or has no handler, forces st_disable and returns.
 * 3. Calls the handler of the current state and stores the returned next state.
 * Must be called periodically from the main loop; it never blocks.
 */
void SHT35_FSM_Run(void);

/**
 * @brief Entry actions executed once when the FSM enters a new state.
 * This function performs the following steps:
 * 1. st_idle: arms the FSM timer for 50 ms (currently not checked by State_Idle()).
 * 2. st_wait_measurement: arms the FSM timer for 20 ms (conversion time).
 * 3. st_disable: arms the FSM timer for SHT35_RETRY_PERIOD_MS (5 min).
 * Other states have no entry action.
 * @param st The state being entered.
 */
void SHT35_OnEnter(SHT35_State_t st);


#ifdef __cplusplus
}
#endif

#endif
