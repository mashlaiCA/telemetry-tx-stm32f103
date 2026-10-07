/**
 * @file  resistive_probe.h
 * @brief AC-excited resistive probe on GPIOA (used by the leaf wetness sensor).
 * This file provides:
 * 1. resistive_probe_t - pins, ADC channel and settling time of one probe.
 * 2. resistive_probe_init() - pin setup, probe left unpowered.
 * 3. resistive_probe_read() - one forward + one reverse measurement with equal
 *    duration, so no net DC current flows through the electrodes.
 */

#ifndef RESISTIVE_PROBE_H
#define RESISTIVE_PROBE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Configuration of one resistive probe. */
typedef struct {
    uint8_t pin_a;       // GPIOA drive pin A, 0..7
    uint8_t pin_b;       // GPIOA drive pin B, 0..7
    uint8_t adc_channel; // ADC1 channel of the divider midpoint (channel n = PAn for 0..7)
    uint16_t set_us;     // Settling time after applying the excitation, us

} resistive_probe_t;

/**
 * @brief Configures the probe pins and leaves the probe unpowered.
 * This function performs the following steps:
 * 1. pin_a and pin_b: push-pull outputs, 2 MHz.
 * 2. adc_channel pin: analog input (only for channels 0..7, i.e. PA0..PA7).
 * 3. Releases both drive pins (floating inputs).
 * @param p Probe configuration.
 */
void resistive_probe_init(const resistive_probe_t *p);

/**
 * @brief Measures the probe with forward and reverse excitation.
 * This function performs the following steps:
 * 1. Disables interrupts (PRIMASK saved) so both directions last equally long.
 * 2. Forward (pin_a = 1, pin_b = 0), waits set_us, converts.
 * 3. Reverse (pin_a = 0, pin_b = 1), waits set_us, converts.
 * 4. Releases both pins and restores PRIMASK.
 * If a conversion fails, the excitation is removed immediately.
 * @param p Probe configuration.
 * @return (fwd + (4095 - rev)) / 2, 0..4095, on success;
 *         0 if a conversion failed (adc1_last_error = 1).
 */
uint16_t resistive_probe_read(const resistive_probe_t *p);

#ifdef __cplusplus
}
#endif

#endif