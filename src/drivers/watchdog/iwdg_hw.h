/**
 * @file  iwdg_hw.h
 * @brief Independent watchdog (IWDG) for unattended field operation.
 * This file provides:
 * 1. iwdg_start() - starts the watchdog with a ~26 s timeout (nominal LSI).
 * 2. iwdg_kick() - reloads the counter.
 * 3. iwdg_debug_freeze() - stops the watchdog while the core is halted by a debugger.
 * 4. iwdg_reset_occurred() - reports whether the last reset came from the watchdog.
 *
 * Purpose: a last line of defense against hangs that are not handled explicitly
 * (SRAM corruption, a corrupted FSM state, an endless loop inside RadioLib, an
 * unknown bug). Without it the only recovery would be a manual power cycle in the field.
 *
 * IWDG rather than WWDG: IWDG runs from its own LSI oscillator (~40 kHz),
 * independent of HSI and the system clock, so it keeps counting even if the
 * RCC configuration is corrupted or TIM2 has stopped. WWDG is clocked from APB1
 * and would stop together with it.
 *
 * Stop mode: IWDG keeps counting in Stop mode (LSI stays on), so the timeout
 * must exceed the longest Stop interval. main.cpp sleeps in chunks of at most
 * SLEEP_CHUNK_MAX_S = 12 s and kicks the watchdog before and after each chunk.
 *
 * LSI spread: LSI is 30..60 kHz. At 60 kHz the timeout drops to ~17.5 s
 * (still 5.5 s above a 12 s chunk); at 30 kHz it grows to ~35 s.
 *
 * After a watchdog reset: the DS3231 keeps time on its own battery, and the
 * startup code clears the alarm flag (ds3231_clear_alarm_flag()), releasing the
 * INT line, so the device resumes its cycle without intervention.
 */

#ifndef IWDG_HW_H
#define IWDG_HW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Starts the independent watchdog. It cannot be stopped afterwards (only by reset).
 * This function performs the following steps:
 * 1. Turns on LSI and waits (bounded) for LSIRDY.
 * 2. Unlocks PR/RLR, sets prescaler /256 (PR = 6) and reload 4095.
 * 3. Waits (bounded) until the values are taken over (IWDG_SR = 0).
 * 4. Reloads the counter and starts the watchdog.
 * Timeout = 4095 * 256 / f_LSI: ~26.2 s at 40 kHz, ~17.5 s at 60 kHz, ~35 s at 30 kHz.
 */
void iwdg_start(void);

/**
 * @brief Reloads the watchdog counter ("kick").
 * Must be called more often than the timeout from every path the firmware
 * can legitimately spend time in.
 */
void iwdg_kick(void);

/**
 * @brief Freezes the watchdog while the core is halted by the debugger (DBGMCU_CR.DBG_IWDG_STOP).
 * Development only - without it every breakpoint ends in a reset.
 */
void iwdg_debug_freeze(void);

/**
 * @brief Reports whether the last reset was caused by the watchdog.
 * Reads RCC_CSR.IWDGRSTF and then clears ALL reset flags (RMVF), so it must be
 * called once, early in main().
 * @return 1 if the last reset was an IWDG reset, 0 otherwise.
 */
uint8_t iwdg_reset_occurred(void);

#ifdef __cplusplus
}
#endif

#endif
