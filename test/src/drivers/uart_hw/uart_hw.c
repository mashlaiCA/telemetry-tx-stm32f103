#include "stm32f103xb.h"
#include "uart_hw.h"

/**
 * @brief Initialize USART1 for UART communication.
 */
void uart_hw_init(void) // Initialize UART peripheral
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
void uart_hw_send_byte(uint8_t c) // Send a single byte over UART
{
    while (!(USART1->SR & USART_SR_TXE)) // Wait until transmit data register is empty
        ;
    USART1->DR = c; // Send the byte
}
