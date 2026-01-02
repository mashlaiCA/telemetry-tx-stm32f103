#include "drivers/uart_hw/uart_hw.h"
#include "uart.h"

void uart_init(void)
{
    uart_hw_init();
}

void uart_send_string(const char *str) 
{
    while (*str != '\0') 
    {
        uart_hw_send_byte((uint8_t)*str); 
        str++;                        
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