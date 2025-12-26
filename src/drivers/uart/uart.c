#include "stm32f103xb.h"

/**
 * @brief Initialize USART1 for UART communication.
 */
void uart_init(void) // Initialize UART peripheral
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN; // Enable USART1 clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   // Enable GPIOA clock

    GPIOA->CRH &= ~(0xF << 4); // Clear CNF9[1:0] and MODE9[0:1] bits
    GPIOA->CRH |= (0x9 << 4);  // Set CNF9[1:0] to alternate function and MODE9[0:1] to push-pull output

    GPIOA->CRH &= ~(0xF << 8); // Clear CNF10[1:0] and MODE10[0:1] bits
    GPIOA->CRH |= (0x4 << 8);  // Set CNF10[1:0] to floating input and MODE10[0:0] to input mode

    USART1->BRR = (52 << 4) | 1; // Set baud rate to 9600 bps assuming 8 MHz PCLK2

    USART1->CR1 = 0;                                           // Reset USART1 control register
    USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE; // Enable USART, transmitter and receiver
}

/**
 * @brief Send a single byte over UART.
 */
void uart_send_byte(uint8_t c) // Send a single byte over UART
{
    while (!(USART1->SR & USART_SR_TXE)) // Wait until transmit data register is empty
        ;
    USART1->DR = c; // Send the byte
}

/**
 * @brief Send a zero-terminated string over UART.
 */
void uart_send_string(const char *str) // Send string to zero-terminated string over UART
{
    while (*str != '\0') // Loop until null terminator
    {
        uart_send_byte((uint8_t)*str); // Send each character
        str++;                         // Move to the next character
    }
}

/**
 * @brief Send a uint16_t value over UART as a string.
 */
void uart_send_uint16_t(uint16_t value) // Send a uint16_t value over UART as a string
{
    char buffer[6] = {0}; // Buffer to hold string representation
    int index = 0;        // Index for buffer

    if (value == 0) // Handle zero case
    {
        uart_send_byte('0'); // Send '0' for zero value
        return;              // Return early
    }
    while (value > 0 && index < sizeof(buffer) - 1) // Convert value to string in reverse order
    {
        buffer[index++] = (value % 10) + '0'; // Get least significant digit and convert to character
        value /= 10;                          // Remove least significant digit
    }
    while (index > 0) // Send the string in correct order
    {
        uart_send_byte(buffer[--index]); // Send each character
    }
}