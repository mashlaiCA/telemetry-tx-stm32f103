#include "analog_sensor_soil_moisture.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpio_hw.h"




void analog_sensor_soil_moisture_init(uint8_t sensor_gpio)
{
    adc1_init();                           // Initialize ADC1 peripheral
    gpio_a_analog_input_init(sensor_gpio); // Initialize GPIOA pin as analog input
    adc1_read(sensor_gpio);                // Perform initial read to stabilize ADC
}

uint16_t analog_sensor_soil_moisture_read(uint8_t sensor_gpio)
{
    static float prev = -1; 

    uint16_t value = adc1_read(sensor_gpio); // Read and return soil moisture value from specified GPIO pin (ADC1 channel)
    float current = ((2960.0f - value)/(2960.0f - 320.0f))*100.0f; // Convert raw ADC value to percentage (assuming 320 is dry and 4000 is wet)
    
    if (current > 100) current = 100;
    if (current < 0) current = 0;

  
    if (prev < 0) {
        prev = current;
        return current;
    }

   
    float smooth = 0.15f * prev + 0.85f * current;

    prev = smooth; 

    return (uint16_t)smooth;
}


