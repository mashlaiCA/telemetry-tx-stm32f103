#include "system_data.h"
#include "devices/Sensor_SHT35/sensor_sht35.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture_fsm.h"



system_data_t system_data = {0}; 


char* int_to_str(int value, char* str) {
    char buffer[12];
    int i = 0, j;

    if (value == 0) {
        *str++ = '0';
        return str;
    }

    if (value < 0) {
        *str++ = '-';
        value = -value;
    }

    while (value > 0) {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }

    for (j = i - 1; j >= 0; j--) {
        *str++ = buffer[j];
    }

    return str;
}

void sensor_update(system_data_t* data) // Function to update sensor data
{
    data->temperature = temperatureSHT35();
    data->humidity = humiditySHT35();
    data->soil_moisture_10 = system_analog_sensors_data.sensor_soil_10;
    data->soil_moisture_20 = system_analog_sensors_data.sensor_soil_20;
}

void build_payload(system_data_t* data)
{
    char *p = data->data_string;

    p = int_to_str(data->temperature, p);
    *p++ = ',';

    p = int_to_str(data->humidity, p);
    *p++ = ',';

    p = int_to_str(data->soil_moisture_10, p);

    *p++ = ',';

    p = int_to_str(data->soil_moisture_20, p);

    *p = '\0';
}

