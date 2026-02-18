#ifndef SHT35_FSM_H
#define SHT35_FSM_H

#include "stdint.h"

typedef enum
{
    st_idle = 0,              // Sensor is idle, waiting for trigger to start measurement
    st_start_sensor,          // Send command to start measurement
    st_wait_measurement,      // Wait for measurement to be ready
    st_read_data_measurement, // Read raw data from sensor
    st_crc_check,             // Check CRC of received data
    st_calculate_data,        // Calculate temperature and humidity from raw data
    st_error,                 // Handle errors
    st_disable,               // Disable sensor
    st_restart_sensor,        // Restart sensor

    st_count     // Total number of states
} SHT35_State_t; // Enumeration for FSM states

/**
 * @brief State function type definition.
 * This typedef defines a function pointer type for state functions in the FSM. Each state function
 * returns the next state to transition to after executing its logic. This allows for a flexible and
 * modular design of the FSM, where each state can determine the next state based on its execution
 * outcome, such as success or error conditions.
 */
typedef SHT35_State_t (*StateFunction_t)(void);

extern SHT35_State_t sht35_state; // Global variable to hold the current state of the FSM

/**
 * @brief State function prototype for the idle state.
 * This function performs the following steps:
 * 1. Waits for a specified delay (e.g., 50 ms) to ensure the sensor is ready for the next measurement cycle.
 * 2. Transitions to the next state (e.g., starting the sensor) after the delay.
 * @return The next state to transition to starting the sensor, or an error state if the delay fails.
 */
SHT35_State_t State_Idle(void);

/**
 * @brief State function prototype for starting the sensor.
 * This function performs the following steps:
 * 1. Sends the command to the SHT35 sensor to start a measurement using I2C communication.
 * 2. Checks for I2C communication errors and transitions to an error state if any errors occur.
 * 3. If the command is sent successfully, transitions to the next state to wait for the measurement to be ready.
 * @return The next state to transition to waiting for the measurement, or an error state if the I2C communication fails.
 */
SHT35_State_t State_Start_Sensor(void);

/**
 * @brief State function prototype for waiting for the measurement to be ready.
 * This function performs the following steps:
 * 1. Waits for a specified delay (e.g., 70 ms) to allow the SHT35 sensor to complete the measurement.
 * 2. Transitions to the next state (e.g., reading data) after the delay.
 * @return The next state to transition to reading data, or an error state if the delay fails.
 */
SHT35_State_t State_Wait_Measurement(void);

/**
 * @brief State function prototype for reading data from the sensor.
 * This function performs the following steps:
 * 1. Reads raw data from the SHT35 sensor using I2C communication.
 * 2. Checks for I2C communication errors and transitions to an error state if any errors occur.
 * 3. If the data is read successfully, transitions to the next state to check the CRC of the received data.
 * @return The next state to transition to checking CRC, or an error state if the I2C communication fails.
 */
SHT35_State_t State_Read_Data_Measurement(void);

/**
 * @brief State function prototype for checking the CRC of the received data.
 * This function performs the following steps:
 * 1. Performs a CRC check on the raw data received from the SHT35 sensor to ensure data integrity.
 * 2. Checks for CRC errors and transitions to an error state if any errors are detected.
 * 3. If the CRC check passes successfully, transitions to the next state to calculate temperature and humidity from the raw data.
 * @return The next state to transition to calculating data, or an error state if the CRC check fails.
 */
SHT35_State_t State_CRC_Check(void);

/**
 * @brief State function prototype for calculating temperature and humidity from the raw data.
 * This function performs the following steps:
 * 1. Converts the raw data from the SHT35 sensor into temperature and humidity values.
 * 2. Stores the calculated values in the appropriate variables or data structures.
 * 3. Transitions to the next state (e.g., idle) after the calculation is complete.
 * @return The next state to transition to idle, or an error state if the calculation fails.
 */
SHT35_State_t State_Calculate_Data(void);

/**
 * @brief State function prototype for handling errors.
 * This function performs the following steps:
 * 1. Handles any errors that occurred during the FSM execution.
 * 2. Transitions to the appropriate error state based on the error type.
 * @return The next state to transition to, which is an error state.
 */
SHT35_State_t State_Error(void);

/**
 * @brief State function prototype for restarting the sensor.
 * This function performs the following steps:
 * 1. Sends a command to the SHT35 sensor to perform a soft reset using I2C communication.
 * 2. Transitions to the next state (e.g., idle) after sending the reset command.
 * @return The next state to transition to idle, or an error state if the I2C communication fails.
 */
SHT35_State_t State_Restart_Sensor(void);

/**
 * @brief Function prototype for running the FSM.
 * This function executes the current state function of the FSM and updates the current state based on the return value of the state function.
 * It should be called periodically (e.g., in a main loop
 * or a scheduler) to allow the FSM to progress through its states and perform the necessary operations for the SHT35 sensor.
 */
void Sensor_FSM_Run(void);

#endif
