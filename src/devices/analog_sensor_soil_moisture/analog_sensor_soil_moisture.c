#include "analog_sensor_soil_moisture.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpio.h"

void analog_sensor_soil_moisture_init(uint8_t sensor_gpio)
{
    adc1_init(); // Initialize ADC1 peripheral
    gpio_a_analog_input_init(sensor_gpio); // Initialize GPIOA pin as analog input
    adc1_read(sensor_gpio); // Perform initial read to stabilize ADC 
}

uint16_t analog_sensor_soil_moisture_read(uint8_t sensor_gpio)
{
   return adc1_read(sensor_gpio);// Read and return soil moisture value from specified GPIO pin (ADC1 channel)
}