#include "exti_hw.h"
#include "stm32f103xb.h"

// Configure PA0 as EXTI wakeup input, falling edge trigger
void exti0_pa0_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // Enable AFIO clock (needed for EXTICR)

    GPIOA->CRL &= ~(0xF << (0 * 4)); // Clear CNF[1:0] and MODE[1:0] bits for PA0
    GPIOA->CRL |= (0x8 << (0 * 4));  // Set PA0 as input with pull-up/pull-down (CNF=10, MODE=00)
    GPIOA->BSRR = (1 << 0);          // ODR=1 selects pull-up (not pull-down) for PA0

    AFIO->EXTICR[0] &= ~(0xF << 0); // Clear EXTI0 source selection bits
    AFIO->EXTICR[0] |= (0x0 << 0);  // Map EXTI line 0 to port A (PA0)

    EXTI->FTSR |= EXTI_FTSR_TR0;  // Trigger on falling edge
    EXTI->RTSR &= ~EXTI_RTSR_TR0; // Disable rising edge trigger

    EXTI->IMR |= EXTI_IMR_MR0; // Unmask EXTI line 0 interrupt

    EXTI->PR = EXTI_PR_PR0; // Clear any pending EXTI0 interrupt flag

    NVIC_ClearPendingIRQ(EXTI0_IRQn); // Clear any pending EXTI0 interrupt in NVIC
    NVIC_EnableIRQ(EXTI0_IRQn);       // Enable EXTI0 interrupt in NVIC
}

void EXTI0_IRQHandler(void)
{
    EXTI->PR = EXTI_PR_PR0; // Clear the EXTI0 pending interrupt flag
}
