#include "stm32f103xb.h"
#include "uart_hw.h"

/**
 * @brief Initialize USART1: 9600 baud 8N1 at PCLK2 = 8 MHz.
 */
void uart_hw_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN; // Enable USART1 clock
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   // Enable GPIOA clock

    GPIOA->CRH &= ~(0xF << 4); // Clear CNF9[1:0] and MODE9[1:0] (PA9 = CRH nibble 1)
    GPIOA->CRH |= (0x9 << 4);  // 0x9: CNF9 = 10 alternate function push-pull, MODE9 = 01 output 10 MHz

    GPIOA->CRH &= ~(0xF << 8); // Clear CNF10[1:0] and MODE10[1:0] (PA10 = CRH nibble 2)
    GPIOA->CRH |= (0x4 << 8);  // 0x4: CNF10 = 01 floating input, MODE10 = 00 input

    USART1->BRR = (52 << 4) | 1; // USARTDIV = 8 MHz / (16 * 9600) = 52.083 -> mantissa 52, fraction 1/16

    USART1->CR1 = 0;                                           // 8 data bits, no parity, interrupts off
    USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE; // Enable USART, transmitter and receiver
}

/**
 * @brief Send a single byte over UART (bounded wait for TXE).
 */
void uart_hw_send_byte(uint8_t c)
{
    for (uint32_t i = 0; !(USART1->SR & USART_SR_TXE) && i < 50000u; i++)
        ; // Wait for SR.TXE; the limit keeps a stuck USART from hanging the caller

    USART1->DR = c; // Send the byte
}

/* Waits for SR.TC: last stop bit sent (bounded). */
void uart_hw_flush(void)
{
    for (uint32_t i = 0; !(USART1->SR & USART_SR_TC) && i < 50000u; i++)
        ; // Wait for transmission complete; limited to 50000 iterations
}
