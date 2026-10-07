/**
 * @file  acd1.h
 * @brief Register-level ADC1 driver: single software-started conversions with bounded waits.
 * This file provides:
 * 1. adc1_init() - clock, power-up and calibration.
 * 2. adc1_read() - one 12-bit conversion of a regular channel.
 * 3. adc1_last_error - failure flag of the last operation.
 * 4. adc1_sleep() / adc1_wake() - power the ADC down before Stop mode and back up after it.
 * ADC clock: PCLK2 8 MHz / 2 = 4 MHz; sample time 239.5 cycles -> 252 cycles = 63 us per conversion.
 */

#ifndef ACD1_H
#define ACD1_H

#include <stdint.h>

/* extern "C" is required because this header is also included from main.cpp
   (adc1_sleep()/adc1_wake() around Stop mode); without it the C++ compiler
   would mangle the names and linking would fail. */
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initializes ADC1.
 *  This function performs the following steps:
 *  1. Enables the ADC1 clock (RCC APB2ENR.ADC1EN).
 *  2. Clears RCC CFGR.ADCPRE: ADC clock = PCLK2 / 2 = 4 MHz.
 *  3. Powers the ADC up (CR2.ADON = 1) and waits a short software delay (tSTAB).
 *  4. Resets the calibration (CR2.RSTCAL) and waits for it (bounded).
 *  5. Runs the calibration (CR2.CAL) and waits for it (bounded).
 *  6. Sets adc1_last_error = 1 if RSTCAL or CAL did not finish, 0 otherwise.
 */
void adc1_init(void);

/**
 * @brief Set to 1 when the last ADC operation (conversion or calibration) timed out.
 * adc1_read() returns 0 on timeout, and 0 is also a valid conversion result,
 * so callers must check this flag rather than the value. Code that drives an
 * electrode (watermark_sample(), resistive_probe_read()) MUST check it and
 * remove the excitation immediately.
 */
extern uint8_t adc1_last_error;

/** @brief Performs one conversion on the given ADC1 channel.
 *  This function performs the following steps:
 *  1. Selects a single conversion (SQR1.L = 0) of the channel (SQR3.SQ1).
 *  2. Sets the channel's sample time to 239.5 cycles (SMPR2 for 0..9, SMPR1 for 10..17).
 *  3. Reads DR to discard a stale result.
 *  4. Starts the conversion by writing ADON = 1 again (the ADC is already on;
 *     SWSTART is not used because EXTTRIG/EXTSEL are not configured).
 *  5. Waits for SR.EOC, at most ADC_EOC_MAX_SPINS (500) loop iterations.
 *  @param channel ADC1 channel, 0..17 (no range check). Channel n of 0..7 is PAn, 8 = PB0, 9 = PB1.
 *  @return 12-bit result 0..4095 (adc1_last_error = 0);
 *          0 on timeout (adc1_last_error = 1).
 */
uint16_t adc1_read(uint8_t channel);

/** @brief Powers ADC1 down before entering Stop mode.
 *  Clears ADON and gates the ADC clock. An ADC left with ADON = 1 draws about
 *  1 mA, which would dominate the Stop-mode budget of the whole board.
 */
void adc1_sleep(void);

/** @brief Powers ADC1 back up after Stop mode and recalibrates it.
 *  Same sequence as adc1_init() (except the prescaler setting): clock on,
 *  ADON, tSTAB delay, RSTCAL, CAL. Calibration is repeated because the ADC was
 *  powered down. Sets adc1_last_error if the calibration did not finish.
 */
void adc1_wake(void);

#ifdef __cplusplus
}
#endif

#endif
