#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DATA_SHT35_READY (1 << 1)
#define DATA_ANALOG_READY (1 << 0)


typedef struct
{
    uint16_t temperature; // Temperature value in hundredths of degrees Celsius
    uint16_t humidity;    // Humidity value in hundredths of percent
    uint16_t soil_moisture_10; 
    
    char data_string[32]; 

    uint8_t ready_sensors_flag;
    uint8_t ready_data_creation_flag; // Flag to indicate if the payload string is ready for transmission
    uint8_t lora_busy;   // Flag to indicate if LoRa module is busy

} system_data_t;

extern system_data_t system_data; // Global variable to hold system data

void sensor_update(system_data_t* data); // Function to update sensor data

void data_creation(system_data_t* data); // Function to build the payload string from sensor data

char* int_to_str(int value, char* str); // Function to convert an integer to a string and append it to the provided buffer

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_DATA_H