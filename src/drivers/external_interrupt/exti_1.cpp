#include "stm32f1xx.h"
#include "exti_1.h"
#include "drivers/gpio/gpio_hw.h"
#include "protocols/uart/uart.h"

volatile uint32_t exti1_interrupt_count = 0; // Counter for EXTI1 interrupts
volatile uint32_t exti1_spurious_interrupt_count = 0; // Counter for spurious EXTI1 interrupts
volatile uint8_t exti1_last_state = 0; // Last known state of the EXTI1 pin

void EXTI1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // Enable AFIO clock

    AFIO->EXTICR[0] &= ~(0xF << 4);
    AFIO->EXTICR[0] |= (0x0 << 4); // Map EXTI1 to PA1

    EXTI->IMR |= EXTI_IMR_MR1; // Unmask EXTI1 interrupt
    EXTI->RTSR |= EXTI_RTSR_TR1; // Trigger on rising edge
    EXTI->FTSR &= ~EXTI_FTSR_TR1; // Disable falling edge trigger

    EXTI->PR = EXTI_PR_PR1; // Clear any pending EXTI1 interrupts

    NVIC_ClearPendingIRQ(EXTI1_IRQn); // Clear any pending EXTI1 interrupts
    NVIC_EnableIRQ(EXTI1_IRQn);       // Enable EXTI1 interrupt in NVIC
}

extern "C" void EXTI1_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR1)
    {
        exti1_interrupt_count++; 
   
        EXTI->PR = EXTI_PR_PR1;  // Clear the EXTI1 pending interrupt flag
        if (hal.dio0Callback)
        {
            hal.dio0Callback(); // Call the registered callback function for DIO0
        }
    }
    else
    {
        exti1_spurious_interrupt_count++; // Increment spurious interrupt counter
    }
}