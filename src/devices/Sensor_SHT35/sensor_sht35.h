#ifndef SENSOR_SHT35_H
#define SENSOR_SHT35_H

#include "stdint.h"
/**
 * @file sensor_sht35.h
 * @brief Header file for SHT35 sensor interface.
 * This module provides function prototypes for interfacing with the SHT35 temperature and humidity sensor.
 * It includes functions for writing commands to the sensor, reading data from the sensor, and retrieving calculated temperature and humidity values. The functions are designed to be used in conjunction with the SHT35
 * state machine defined in sht35_fsm.h and the error status management in sht35_status.h. This header file abstracts the low-level I2C communication details, allowing higher-level code to interact with the SHT35 sensor in a more straightforward manner.
 * The functions defined in this header file are intended to be called by the state functions in the SHT35 FSM to perform the necessary operations for starting measurements, reading data, and handling sensor restarts.
 * The implementation of these functions should handle the specific I2C communication protocol required by the SHT35 sensor, including sending the correct commands and reading the appropriate number of bytes for temperature and humidity data.
 * The temperature and humidity values returned by the respective functions are expected to be in hundredths of degrees Celsius and hundredths of percent, respectively, to allow for precise representation of the sensor  readings.
 */

/**
 * @brief Function prototype for writing commands to the SHT35 sensor.
 * This function sends the appropriate command to the SHT35 sensor to initiate a measurement.
 * It should use the I2C communication protocol to send the command bytes to the sensor's I2C address. 
 * The specific command to start a measurement (e.g., high repeatability, clock stretching disabled) should be defined in the implementation of this function.
 * This function is typically called in the state function responsible for starting the sensor measurement in the SHT35 FSM.
 */
void I2C_Write_Sensor_SHT35();

/**
 * @brief Function prototype for reading data from the SHT35 sensor.
 * This function reads the raw data from the SHT35 sensor after a measurement has been initiated.
 * It should use the I2C communication protocol to read the appropriate number of bytes from the sensor's I2C address, which includes raw temperature and humidity values along with their CRC checksums.
 */
void I2C_Read_Sensor_SHT35();

/**
 * @brief Function prototype for retrieving the calculated temperature value from the SHT35 sensor.
 * This function returns the calculated temperature value in hundredths of degrees Celsius, which is derived from the raw data read from the sensor. The calculation of the temperature value should be performed in the appropriate state function of the SHT35 FSM after the raw data has been read and the CRC check has been performed.
 * @return The calculated temperature value in hundredths of degrees Celsius.
 */
uint16_t temperatureSHT35(void);

/**
 * @brief Function prototype for retrieving the calculated humidity value from the SHT35 sensor.
 * This function returns the calculated humidity value in hundredths of percent, which is derived from the raw data read from the sensor. The calculation of the humidity value should be performed in the appropriate state function of the SHT35 FSM after the raw data has been read and the CRC check has been performed.
 * @return The calculated humidity value in hundredths of percent.
 */
uint16_t humiditySHT35(void);

/**
 * @brief Function prototype for restarting the SHT35 sensor.
 * This function sends a soft reset command to the SHT35 sensor to restart it.
 */
void I2C_Restart_Sensor_SHT35();
#endif
