
#ifndef SLEEP_HW_H
#define SLEEP_HW_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enter Stop mode and wait for a wakeup interrupt.
 * This function performs the following steps:
 * 1. Enable the PWR peripheral clock.
 * 2. Clear PDDS so the low-power mode entered is Stop, not Standby.
 * 3. Set LPDS so the voltage regulator runs in low-power mode during Stop.
 * 4. Clear the wakeup flag (CWUF).
 * 5. Set SLEEPDEEP in the Cortex-M3 SCB so WFI enters Stop mode instead of Sleep mode.
 * 6. Execute WFI and wait for a wakeup event.
 * 7. Clear SLEEPDEEP on wake.
 * @note The MCU runs on HSI 8MHz with no PLL in this project, so no clock
 *       reconfiguration is needed after wake (HSI is already the active
 *       source on Stop mode exit).
 */
void sleep_enter_stop(void);

/**
 * @brief Keep the debugger (SWD) connected while in Stop mode.
 * This function sets DBG_STOP in DBGMCU->CR so the debug interface stays
 * clocked during Stop mode, for use during development only.
 */
void sleep_debug_enable(void);

#ifdef __cplusplus
}
#endif

#endif
