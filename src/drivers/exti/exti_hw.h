/**
 * @file  exti_hw.h
 * @brief EXTI lines used for wakeup: line 0 (PA0, DS3231 INT/SQW) and line 1 (PB1, SX1276 DIO0).
 * This file provides:
 * 1. exti0_wakeup_flag - set by EXTI0_IRQHandler when the DS3231 alarm fires.
 * 2. exti0_pa0_init() - PA0 as falling-edge wakeup input.
 * 3. exti0_pa0_level() - raw level of the RTC interrupt line.
 * 4. exti_clear_pending_wakeup_lines() - clears stale EXTI/NVIC pending bits before sleep.
 * 5. exti1_dio0_mask() / exti1_dio0_unmask() - keep DIO0 from waking the MCU from Stop mode.
 * EXTI line 1 itself is configured in drivers/external_interrupt/exti_1.cpp.
 */

#ifndef EXTI_HW_H
#define EXTI_HW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint8_t exti0_wakeup_flag; // Set to 1 by EXTI0_IRQHandler (DS3231 alarm); cleared by the application before Stop and after handling the wakeup

/**
 * @brief Configures PA0 as the EXTI0 wakeup input from the DS3231.
 * This function performs the following steps:
 * 1. Enables the GPIOA and AFIO clocks.
 * 2. Configures PA0 as input with pull-up (CNF = 10, ODR0 = 1); INT/SQW is open-drain.
 * 3. Maps EXTI line 0 to port A (AFIO_EXTICR1.EXTI0 = 0000).
 * 4. Enables the falling-edge trigger and disables the rising-edge trigger.
 * 5. Unmasks EXTI line 0.
 * 6. Clears any pending EXTI0 request (EXTI->PR and NVIC).
 * 7. Enables EXTI0_IRQn in the NVIC.
 */
void exti0_pa0_init(void);

/**
 * @brief Reads the raw level of PA0 (DS3231 INT/SQW).
 * The DS3231 output is open-drain and stays low as long as A1F is set. If
 * PA0 already reads 0, no further falling edge can be produced, and entering
 * Stop mode would only end through the watchdog reset.
 * @return 1 if the line is released (high), 0 if the RTC is holding it low.
 */
uint8_t exti0_pa0_level(void);

/**
 * @brief Clears stale pending requests on EXTI lines 0 and 1.
 * Clears both EXTI->PR and the NVIC pending bits. A request still latched
 * from before would make __WFI() return immediately instead of sleeping.
 */
void exti_clear_pending_wakeup_lines(void);

/**
 * @brief Masks EXTI line 1 (SX1276 DIO0 on PB1) so it cannot wake the MCU from Stop mode.
 */
void exti1_dio0_mask(void);

/**
 * @brief Unmasks EXTI line 1 after a wakeup.
 * Clears a pending flag that may have been latched while the line was masked
 * (EXTI->PR and NVIC) before unmasking it, so no phantom "TX done" is delivered.
 */
void exti1_dio0_unmask(void);

#ifdef __cplusplus
}
#endif

#endif
