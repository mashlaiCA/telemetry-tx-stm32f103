#include "stm32f1xx.h"
#include "exti_1.h"
#include "drivers/gpio/gpio_hw.h"
#include "protocols/uart/uart.h"

volatile uint32_t exti1_interrupt_count = 0; // DIO0 interrupts handled
volatile uint32_t exti1_spurious_interrupt_count = 0; // Handler entries without PR1 set
volatile uint8_t exti1_last_state = 0; // Not written anywhere yet

/* PB1 (DIO0) -> EXTI line 1, rising edge: SX1276 raises DIO0 on TxDone. */
void EXTI1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // Enable AFIO clock (needed for EXTICR)

    AFIO->EXTICR[0] &= ~(0xF << 4); // Clear EXTI1 source selection (bits 7:4)
    AFIO->EXTICR[0] |= (0x1 << 4); // Map EXTI1 to PB1 (LoRa DIO0)

    EXTI->IMR |= EXTI_IMR_MR1; // Unmask EXTI1 interrupt
    EXTI->RTSR |= EXTI_RTSR_TR1; // Trigger on rising edge
    EXTI->FTSR &= ~EXTI_FTSR_TR1; // Disable falling edge trigger

    EXTI->PR = EXTI_PR_PR1; // Clear a pending EXTI1 request (write 1 to clear)

    NVIC_ClearPendingIRQ(EXTI1_IRQn); // Clear a pending EXTI1 interrupt in NVIC
    NVIC_EnableIRQ(EXTI1_IRQn);       // Enable EXTI1 interrupt in NVIC
}

/* DIO0 edge: clear the request, then call the RadioLib callback (onTxDone). */
extern "C" void EXTI1_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR1) // Real EXTI1 request
    {
        exti1_interrupt_count++; // Diagnostics

        EXTI->PR = EXTI_PR_PR1;  // Clear the EXTI1 pending interrupt flag
        if (hal.dio0Callback) // Callback registered via attachInterrupt() / setDio0Action()
        {
            hal.dio0Callback(); // Runs in interrupt context
        }
    }
    else
    {
        exti1_spurious_interrupt_count++; // Entered without a pending EXTI1 request
    }
}