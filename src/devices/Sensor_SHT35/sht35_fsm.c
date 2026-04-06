#include "sht35_fsm.h"
#include "application/errors/system_error.h"
#include "sensor_sht35.h"
#include "drivers/timeout_hw/timeout_hw.h"

#include "application/errors/i2c_errors.h"

uint8_t sht_max_retry = 3;            // Maximum number of retry attempts in case of errors
static uint8_t error_retry_count = 0; // Counter for retry attempts in case of errors
timeout_t sht35_timeout;              // Timeout structure for SHT35 operations
uint8_t timeout_ms;                   // Timeout duration in milliseconds

SHT35_State_t sht35_state = st_idle; // Initial state of the FSM

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

typedef struct
{

    SHT35_State_t state; // Current state of the FSM
    SHT35_Status_t prev; // Previous state of the FSM for detecting state changes

    timeout_t timer; // Timer for the FSM
} sht35_cxt_t;       // Structure to hold the context of the SHT35 FSM, including the current state, previous state, and timer for managing timeouts

sht35_cxt_t sht35_context = {
    .state = st_idle, // Initial state of the FSM is set to idle
    .prev = st_idle,  // Previous state is also initialized to idle to allow for proper state change detection when the FSM starts running
    .timer = {0}      // Initialize the timer structure to zero
};

void SHT35_FSM_Run(void)
{
    if (sht35_context.state != sht35_context.prev) // Check if the current state has changed from the previous state to determine if we need to perform any actions upon entering a new state
    {
        SHT35_OnEnter(sht35_context.state);       // Call the function to handle actions that need to be performed when entering a new state, such as starting timers or resetting variables specific to that state
        sht35_context.prev = sht35_context.state; // Update the previous state to the current state after handling the state change to ensure that we only perform the on-enter actions once per state transition
    }
    sht35_context.state = state_table[sht35_context.state](); // Call the function corresponding to the current state from the state table to execute the logic for that state and update the current state based on the return value of the function, allowing the FSM to transition to the next appropriate state based on the logic defined in each state's function
}

void SHT35_OnEnter(SHT35_State_t st)
{
    switch (st) // Check the current state and perform actions specific to that state
    {
    case st_idle:
        timeout_start(&sht35_context.timer, 50); // Start the timeout for 50 ms when entering the idle state
        break;

    case st_wait_measurement:
        timeout_start(&sht35_context.timer, 20); // Start the timeout for 20 ms when entering the wait measurement state
        break;

    default: // Handle unexpected states
        break;
    }
}

SHT35_State_t State_Idle(void) // Idle state, waiting for trigger to start measurement
{

    if (!timeout_has_expired(&sht35_context.timer)) // Wait for 50 ms before starting the next measurement cycle to ensure the sensor is ready
    {
        return st_idle; // If the delay fails, transition to the idle state
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

    if (!timeout_has_expired(&sht35_context.timer)) // Wait for 20 ms before reading the data
    {
        return st_wait_measurement; // If the delay fails, transition to the error state
    }
    return st_read_data_measurement; // Transition to the state for reading data from the sensor
}

SHT35_State_t State_Read_Data_Measurement(void) // State for reading raw data from the sensor
{
    I2C_Read_Sensor_SHT35();            // Read raw data from the SHT35 sensor using I2C communication
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
    SHT35_Calculate();                      // Convert the raw data from the SHT35 sensor into temperature and humidity values
    if (get_last_sht35_error() != sht35_ok) // Check for calculation errors and transition to the error state if any errors occur
    {
        return st_error; // If the calculation is complete, transition to the idle state for the next measurement cycle
    }
    return st_idle; // Transition to the idle state for the next measurement cycle
}

SHT35_State_t State_Restart_Sensor(void) // State for restarting the sensor
{
    I2C_Restart_Sensor_SHT35(); // Send a soft reset command to the SHT35 sensor to restart it

    // Dealay!!!!!!!!!

    return st_idle; // Transition to the idle state after restarting the sensor to allow for the next measurement cycle to begin
}

SHT35_State_t State_Error(void) // State for handling errors
{
    error_retry_count++;                   // Increment the retry counter for error handling
    if (error_retry_count < sht_max_retry) // Check if the maximum retry limit has been reached
    {
        return st_restart_sensor; // If the retry limit has not been reached, transition to the state for restarting the sensor to attempt to recover from the error
    }

    set_system_error(system_error_sht35); // If the retry limit has been reached, set the system error to indicate a persistent issue with the SHT35 sensor
    error_retry_count = 0;                // Reset the retry counter after reaching the maximum retry limit to allow for future error handling attempts

    get_system_error(); // For debugging purposes, read the system error to ensure it is set correctly

    return st_disable; // Transition to the state for disabling the sensor to prevent further attempts to communicate with a potentially faulty sensor
}
