#include "drivers/soil_moisture_sensors/soil_moisture_sensors.h"
#include "drivers/uart/uart.h"
#include "drivers/time/tim2.h"

uint16_t count = 1; // Counter for readings

int main(void)// Main function
{
    timer_init(); // Initialize TIM2 timer
    adc_init();   // Initialize ADC for soil moisture sensor
    uart_init();  // Initialize UART for communication
    

    while (1)// Main loop
    {
        timer_tick(); // Update system time
        if (timer_sec(1000)) // Check if 1000 ms has elapsed
        {

            uint16_t adc_value = adc_read(); // Read ADC value from soil moisture sensor
            //(void)adc_value; // Check value to avoid compiler warning
            uart_send_uint16_t(count++); // Send reading count over UART
            uart_send_string(": ");// Send separator
            uart_send_uint16_t(adc_value); // Send ADC value over UART
            uart_send_string("\r\n"); // Send newline
        }
    }
}
