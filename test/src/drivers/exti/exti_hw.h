
#ifndef EXTI_HW_H
#define EXTI_HW_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure PA0 as EXTI wakeup input.
 * This function performs the following steps:
 * 1. Enable GPIOA and AFIO clocks.
 * 2. Configure PA0 as input with pull-up.
 * 3. Map EXTI line 0 to port A.
 * 4. Configure EXTI line 0 for falling edge trigger.
 * 5. Unmask EXTI line 0 interrupt.
 * 6. Clear any pending EXTI0 interrupt flag.
 * 7. Enable EXTI0 interrupt in NVIC.
 */
void exti0_pa0_init(void);

#ifdef __cplusplus
}
#endif

#endif
