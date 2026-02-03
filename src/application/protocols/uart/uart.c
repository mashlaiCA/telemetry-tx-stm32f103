#include "drivers/uart_hw/uart_hw.h"
#include "uart.h"

void uart_init(void)
{
    uart_hw_init();// Initialize hardware UART
}

void uart_send_string(const char *str) 
{
    while (*str != '\0') // Loop until null terminator
    {
        uart_hw_send_byte((uint8_t)*str); // Send each character    
        str++;// Move to next character                      
    }
}

void uart_send_uint16_t(uint16_t value) // Send a uint16_t value over UART as a string
{
    char buffer[6] = {0}; // Buffer to hold string representation
    int index = 0;        // Index for buffer

    if (value == 0) // Handle zero case
    {
        uart_hw_send_byte('0'); // Send '0' for zero value
        return;              // Return early
    }
    while (value > 0 && index < sizeof(buffer) - 1) // Convert value to string in reverse order
    {
        buffer[index++] = (value % 10) + '0'; // Get least significant digit and convert to character
        value /= 10;                          // Remove least significant digit
    }
    while (index > 0) // Send the string in correct order
    {
        uart_hw_send_byte(buffer[--index]); // Send each character
    }
}

void uart_send_uint16_t2(uint16_t value1, 
                        uint16_t value2, 
                        uint16_t value3, 
                        uint16_t value4) 
{
    uint16_t values[4] = {value1, value2, value3, value4};
    char buffer[24] = {0}; 
    uint8_t len = 4;     

    for(uint8_t i = 0; i < len; i++)
    {
        uint16_t value = values[i];
        uint8_t index = 0;

        while (value > 0 && index < sizeof(buffer) - 1) 
        {
            buffer[index++] = (value % 10) + '0';
            value /= 10;
        }
        buffer[index++] = ' ';

        while (index > 0)
        {
            uart_hw_send_byte(buffer[--index]);
        }
    }
             
             uart_hw_send_byte('\n');
}

