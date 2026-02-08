#ifndef SHT35_FSM_H
#define SHT35_FSM_H
 
#include "stdint.h"

typedef enum
{
    st_idle = 0, // Sensor is idle, waiting for trigger to start measurement
    st_start_sensor, // Send command to start measurement
    st_wait_measurement, // Wait for measurement to be ready
    st_read_data_measurement, // Read raw data from sensor
    st_crc_check, // Check CRC of received data
    st_calculate_data, // Calculate temperature and humidity from raw data
    st_error, // Handle errors
    st_disable, // Disable sensor
    st_restart_sensor, // Restart sensor

    st_count // Total number of states
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
SHT35_State_t State_Wait_Measurement(void);
SHT35_State_t State_Read_Data_Measurement(void);
SHT35_State_t State_CRC_Check(void);
SHT35_State_t State_Calculate_Data(void); 
SHT35_State_t State_Error(void);
SHT35_State_t State_Restart_Sensor(void);

void Sensor_FSM_Run(void);

#endif
