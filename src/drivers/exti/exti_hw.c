#include "exti_hw.h"
#include "stm32f103xb.h"

volatile uint8_t exti0_wakeup_flag = 0; // volatile: written in EXTI0_IRQHandler, read in the main loop

/* PA0 = DS3231 INT/SQW (open-drain, active low), falling-edge EXTI0 wakeup source. */
void exti0_pa0_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; // Enable GPIOA clock
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // Enable AFIO clock (needed for EXTICR)

    GPIOA->CRL &= ~(0xF << (0 * 4)); // Clear CNF[1:0] and MODE[1:0] bits for PA0
    GPIOA->CRL |= (0x8 << (0 * 4));  // Set PA0 as input with pull-up/pull-down (CNF=10, MODE=00)
    GPIOA->BSRR = (1 << 0);          // ODR=1 selects pull-up (not pull-down) for PA0

    AFIO->EXTICR[0] &= ~(0xF << 0); // Clear EXTI0 source selection bits
    AFIO->EXTICR[0] |= (0x0 << 0);  // EXTI0 = port A (value 0000; the OR is a no-op kept for clarity)

    EXTI->FTSR |= EXTI_FTSR_TR0;  // Trigger on falling edge (alarm asserts INT low)
    EXTI->RTSR &= ~EXTI_RTSR_TR0; // No trigger on rising edge (flag cleared, line released)

    EXTI->IMR |= EXTI_IMR_MR0; // Unmask EXTI line 0 interrupt

    EXTI->PR = EXTI_PR_PR0; // Clear any pending EXTI0 request (rc_w1: write 1 to clear)

    NVIC_ClearPendingIRQ(EXTI0_IRQn); // Clear any pending EXTI0 interrupt in NVIC
    NVIC_EnableIRQ(EXTI0_IRQn);       // Enable EXTI0 interrupt in NVIC
}

/* Raw level of the RTC interrupt line. */
uint8_t exti0_pa0_level(void)
{

    return (GPIOA->IDR & (1u << 0)) ? 1u : 0u; // 1 = released, 0 = DS3231 holds it low (A1F set)
}

/* Drops stale requests on lines 0 and 1 so WFI does not return immediately. */
void exti_clear_pending_wakeup_lines(void)
{

    EXTI->PR = EXTI_PR_PR0 | EXTI_PR_PR1; // Clear EXTI pending bits (write 1 to clear)

    NVIC_ClearPendingIRQ(EXTI0_IRQn); // Clear the NVIC side as well: an NVIC pending bit alone also wakes WFI
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
}

/* DIO0 must not wake the MCU from Stop mode. */
void exti1_dio0_mask(void)
{
    EXTI->IMR &= ~EXTI_IMR_MR1; // Mask EXTI line 1 (edge detection still latches PR)
}

/* Re-enables DIO0 after a wakeup without delivering an edge latched while masked. */
void exti1_dio0_unmask(void)
{

    EXTI->PR = EXTI_PR_PR1;          // Drop an edge latched while the line was masked
    NVIC_ClearPendingIRQ(EXTI1_IRQn); // ... and its NVIC pending bit

    EXTI->IMR |= EXTI_IMR_MR1; // Unmask EXTI line 1
}

/* DS3231 alarm: clear the request and report the wakeup to the main loop. */
void EXTI0_IRQHandler(void)
{
    EXTI->PR = EXTI_PR_PR0; // Clear the EXTI0 pending interrupt flag

    exti0_wakeup_flag = 1; // Alarm wakeup happened
}
