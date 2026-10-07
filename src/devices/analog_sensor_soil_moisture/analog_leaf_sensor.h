/**
 * @file  analog_leaf_sensor.h
 * @brief Resistive leaf wetness sensor (drive PA3/PA4, sense ADC channel 2 = PA2).
 * This file provides:
 * 1. leaf_state_t / leaf_data_t - classified state and last raw value.
 * 2. leaf_wetness_init() - probe setup.
 * 3. leaf_wetness_read() - 8-sample averaged raw reading; also updates the
 *    internal wet/dry state with hysteresis.
 */

#ifndef ANALOG_LEAF_SENSOR_H
#define ANALOG_LEAF_SENSOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Classified leaf surface state. */
typedef enum {
    LEAF_DRY = 0,  // Surface dry (raw rose above the dry threshold)
    LEAF_WET = 1,  // Surface wet (raw fell below the wet threshold)
    LEAF_FAULT = 2 // Raw value implausibly low (short circuit / failed read)
} leaf_state_t;

/** @brief Last leaf sensor result. */
typedef struct {
    uint16_t     raw;   // Averaged raw value, 0..4095 (lower = wetter)
    leaf_state_t state; // Classified state
} leaf_data_t;

void leaf_wetness_init(void);      // Configures the probe pins (PA3/PA4 drive, PA2 analog) and resets the state
uint16_t leaf_wetness_read(void);  // Measures; returns the averaged raw value 0..4095 (0 if every sample failed)


#ifdef __cplusplus
}
#endif

#endif