#include "drivers/soil_moisture_sensors/soil_moisture_sensors.h"
#include "drivers/uart/uart.h"
#include "drivers/time/tim2.h"

uint16_t count = 1; // Counter for readings

int main(void)// Main function
{
    timer_init(); // Initialize TIM2 timer
    adc_init_pin(4);   // Initialize ADC for soil moisture sensor
    adc_init_pin(3);   // Initialize ADC for soil moisture sensor on PA4
    uart_init();  // Initialize UART for communication
    

    while (1)// Main loop
    {
        timer_tick(); // Update system time
        if (timer_sec(1000)) // Check if 1000 ms has elapsed
        {

            uint16_t adc_value3 = adc_read(3); // Read ADC value from soil moisture sensor on PA3
            uint16_t adc_value4 = adc_read(4); // Read ADC value from soil moisture sensor on PA4
            //(void)adc_value; // Check value to avoid compiler warning
            uart_send_uint16_t(count++); // Send reading count over UART
            uart_send_string(": ");// Send separator
            uart_send_uint16_t(adc_value3); // Send ADC value from PA3 over UART
            uart_send_string(", ");// Send separator
            uart_send_uint16_t(adc_value4); // Send ADC value from PA4  over UART
            uart_send_string("\r\n"); // Send newline
        }
    }
}
