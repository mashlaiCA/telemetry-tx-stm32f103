#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include "stdint.h"

typedef struct
{
    uint16_t temperature; // Temperature value in hundredths of degrees Celsius
    uint16_t humidity;    // Humidity value in hundredths of percent
    uint16_t soil_moisture; 
    char data_string[32]; 

    uint8_t data_string_ready; // Flag to indicate if new data is ready
    uint8_t lora_busy;   // Flag to indicate if LoRa module is busy

} system_data_t;

extern system_data_t system_data; // Global variable to hold system data

void sensor_update(system_data_t* data, 
                   uint8_t analog_sensor); // Function to update sensor data

void build_payload(system_data_t* data); // Function to build the payload string from sensor data

char* int_to_str(int value, char* str); // Function to convert an integer to a string and append it to the provided buffer

#endif // SYSTEM_DATA_H