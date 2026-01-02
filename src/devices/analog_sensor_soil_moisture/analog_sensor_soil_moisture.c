#include "analog_sensor_soil_moisture.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpioA.h"

void analog_sensor_soil_moisture_init(uint8_t sensor_gpio)
{
    adc1_init(); 
    gpio_a_analog_input_init(sensor_gpio);  
    adc1_read(sensor_gpio);     
}

uint16_t analog_sensor_soil_moisture_read(uint8_t sensor_gpio)
{
   return adc1_read(sensor_gpio);
}