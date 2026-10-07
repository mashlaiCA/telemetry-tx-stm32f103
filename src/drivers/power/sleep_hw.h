/**
 * @file  sleep_hw.h
 * @brief Stop-mode entry for the low-power measurement cycle.
 * This file provides:
 * 1. sleep_enter_stop() - enters Stop mode (regulator in low-power mode) and returns after wakeup.
 * 2. sleep_enter_stop_for() - the same, plus compensation of the software clock for the sleep time.
 * 3. sleep_debug_enable() - keeps SWD usable while in Stop mode (development).
 * The wakeup source is the DS3231 alarm on PA0 / EXTI0 (see exti_hw.h).
 */

#ifndef SLEEP_HW_H
#define SLEEP_HW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enters Stop mode and waits for a wakeup interrupt.
 * This function performs the following steps:
 * 1. Enables the PWR peripheral clock.
 * 2. Clears PWR_CR.PDDS so deep sleep is Stop, not Standby.
 * 3. Sets PWR_CR.LPDS: voltage regulator in low-power mode during Stop.
 * 4. Clears the wakeup flag (PWR_CR.CWUF).
 * 5. Sets SCB_SCR.SLEEPDEEP so WFI enters Stop instead of Sleep.
 * 6. Disables interrupts (PRIMASK), executes DSB/ISB and WFI.
 * 7. After wakeup clears SLEEPDEEP and restores PRIMASK, so the wakeup ISR runs.
 * @note The MCU runs on HSI 8 MHz without PLL. On Stop exit the hardware
 *       selects HSI as the system clock, so no clock reconfiguration is needed.
 */
void sleep_enter_stop(void);

/**
 * @brief Enters Stop mode and adds the expected sleep time to the software clock.
 * In Stop mode HSI and all peripheral clocks are stopped, TIM2 included, so
 * sys_ms/sys_us do not advance during sleep. timeout_t timers (which compare
 * sys_ms differences) therefore do not expire during sleep, and millis_time()
 * would fall behind wall-clock time. This wrapper adds expected_ms after
 * wakeup. The authoritative date/time source remains the DS3231.
 * @param expected_ms Time the DS3231 alarm was armed for, in milliseconds.
 *        It is added even if the MCU woke up earlier.
 */
void sleep_enter_stop_for(uint32_t expected_ms);

/**
 * @brief Keeps the debugger (SWD) connected while in Stop mode.
 * This function sets DBGMCU_CR.DBG_STOP so the debug interface stays clocked
 * during Stop mode. Development only: it increases Stop-mode current.
 */
void sleep_debug_enable(void);

#ifdef __cplusplus
}
#endif

#endif
