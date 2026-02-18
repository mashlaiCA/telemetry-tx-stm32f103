#include "sht35_fsm.h"
#include "application/errors/system_error.h"
#include "sensor_sht35.h"
#include "application/time/scheduler.h"

#include "application/errors/i2c_errors.h"

uint8_t sht_max_retry = 3;            // Maximum number of retry attempts in case of errors
static uint8_t error_retry_count = 0; // Counter for retry attempts in case of errors

StateFunction_t state_table[st_count] = {
    State_Idle,                  // function for idle state
    State_Start_Sensor,          // function for starting the sensor measurement
    State_Wait_Measurement,      // function for waiting for the measurement to be ready
    State_Read_Data_Measurement, // function for reading raw data from the sensor
    State_CRC_Check,             // function for checking the CRC of the received data
    State_Calculate_Data,        // function for calculating the temperature and humidity values
    State_Restart_Sensor,        // function for restarting the sensor
    State_Error                  // function for handling errors
};

SHT35_State_t sht35_state = st_idle; // Initial state of the FSM

SHT35_State_t State_Idle(void) // Idle state, waiting for trigger to start measurement
{
    if (scheduler_delay_ms(50)) // Wait for 50 ms before starting the next measurement cycle to ensure the sensor is ready
    {
        return st_error; // If the delay fails, transition to the error state
    }
    return st_start_sensor; // Transition to the state for starting the sensor measurement
}

SHT35_State_t State_Start_Sensor(void) // State for starting the sensor measurement
{
    I2C_Write_Sensor_SHT35();           // Send the command to the SHT35 sensor to start a measurement using I2C communication
    if (get_last_i2c_error() != i2c_ok) // Check for I2C communication errors and transition to the error state if any errors occur
    {
        return st_error; // If the command is sent successfully, transition to the state for waiting for the measurement to be ready
    }
    return st_wait_measurement; // Transition to the state for waiting for the measurement to be ready
}

SHT35_State_t State_Wait_Measurement(void) // State for waiting for the measurement to be ready
{
    if (scheduler_delay_ms(70)) // Wait for 70 ms before reading the data
    {
        return st_error; // If the delay fails, transition to the error state
    }
    return st_read_data_measurement; // Transition to the state for reading data from the sensor
}

SHT35_State_t State_Read_Data_Measurement(void) // State for reading raw data from the sensor
{
    I2C_Read_Sensor_SHT35(); // Read raw data from the SHT35 sensor using I2C communication
    if (get_last_i2c_error() != i2c_ok) // Check for I2C communication errors and transition to the error state if any errors occur
    {
        return st_error; // If the data is read successfully, transition to the state for checking the CRC of the received data
    }
    return st_crc_check; // Transition to the state for checking the CRC of the received data
}

SHT35_State_t State_CRC_Check(void) // State for checking the CRC of the received data
{
    SHT35_CRC_Check(); // Perform a CRC check on the raw data received from the SHT35 sensor to ensure data integrity

    if (get_last_sht35_error() != sht35_ok) // Check for CRC errors and transition to the error state if any errors are detected
    {
        return st_error; // If the CRC check passes successfully, transition to the state for calculating temperature and humidity from the raw data
    }
    return st_calculate_data; // Transition to the state for calculating temperature and humidity from the raw data
}
SHT35_State_t State_Calculate_Data(void) // State for calculating temperature and humidity from the raw data
{
    SHT35_Calculate(); // Convert the raw data from the SHT35 sensor into temperature and humidity values
    if (get_last_sht35_error() != sht35_ok) // Check for calculation errors and transition to the error state if any errors occur
    {
        return st_error; // If the calculation is complete, transition to the idle state for the next measurement cycle
    }
    return st_idle; // Transition to the idle state for the next measurement cycle
}

SHT35_State_t State_Restart_Sensor(void)// State for restarting the sensor
{
    I2C_Restart_Sensor_SHT35();// Send a soft reset command to the SHT35 sensor to restart it

//Dealay!!!!!!!!!

    return st_idle; // Transition to the idle state after restarting the sensor to allow for the next measurement cycle to begin
}

SHT35_State_t State_Error(void) // State for handling errors
{
    error_retry_count++; // Increment the retry counter for error handling
    if (error_retry_count < sht_max_retry) // Check if the maximum retry limit has been reached
    {
        return st_restart_sensor; // If the retry limit has not been reached, transition to the state for restarting the sensor to attempt to recover from the error
    }

    set_system_error(system_error_sht35); // If the retry limit has been reached, set the system error to indicate a persistent issue with the SHT35 sensor
    error_retry_count = 0;// Reset the retry counter after reaching the maximum retry limit to allow for future error handling attempts

    get_system_error(); // For debugging purposes, read the system error to ensure it is set correctly

    return st_disable;// Transition to the state for disabling the sensor to prevent further attempts to communicate with a potentially faulty sensor
}

void Sensor_FSM_Run(void) // Run the state machine for the SHT35 sensor
{
    sht35_state = state_table[sht35_state]();// Call the function corresponding to the current state and update the state based on the return value of the function
}
