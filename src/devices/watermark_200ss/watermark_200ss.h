/**
 * @file  watermark_200ss.h
 * @brief Watermark 200SS soil water tension sensor: AC excitation, resistance and centibar conversion.
 * This file provides:
 * 1. Status codes and reserved cb values for SHORT / OPEN probes.
 * 2. watermark_data_t - result of one measurement.
 * 3. watermark_init() - pin setup (drive PB13/PB14, sense PB0 = ADC channel 8).
 * 4. watermark_sample() - one forward + reverse excitation with interrupts disabled.
 * 5. watermark_resistance() / watermark_classify() - resistance and cb from raw codes.
 * 6. watermark_read() - blocking complete measurement (WM_SAMPLES samples).
 * 7. watermark_pack_string() - debug text "cb=..,R=..,T=..,st=..,f=..,r=..".
 * The non-blocking measurement used by the application is in watermark_fsm.h.
 */

#ifndef WATERMARK_200SS_H
#define WATERMARK_200SS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WM_CB_SHORT   240 // cb code reported for a shorted probe (valid readings are 0..239)
#define WM_CB_OPEN    255 // cb code reported for an open / disconnected probe

#define WM_PACKET_LEN   56 // Size of the debug text buffer, including '\0'

#define WM_SAMPLES        3 // Forward/reverse samples averaged per measurement
#define WM_REST_MS        5 // Pause between samples for probe depolarization, ms

/** @brief Probe status derived from the measured resistance. */
typedef enum {
    WM_OK = 0, // Resistance in the valid range; cb computed
    WM_SHORT,  // Resistance below WM_R_SHORT (300 Ohm)
    WM_OPEN    // Resistance at or above WM_R_OPEN (35 kOhm), or no valid sample
} wm_status_t;

/** @brief Result of one Watermark measurement. */
typedef struct {
    uint16_t     raw_fwd; // ADC code of the last successful forward excitation
    uint16_t     raw_rev; // ADC code of the last successful reverse excitation
    float        r_ohm;   // Average probe resistance, Ohm (0 = no valid sample)
    float        cb;      // Soil water tension, centibar: 0..239, or WM_CB_SHORT / WM_CB_OPEN
    wm_status_t  status;  // Probe status
} watermark_data_t;

void watermark_init(void); // PB0 analog input, PB13/PB14 released (Hi-Z)
watermark_data_t watermark_read(float soil_temp_c); // Blocking measurement: WM_SAMPLES samples with WM_REST_MS pauses, classified
void watermark_pack_string(const watermark_data_t *d, float ntc_temp, char *out); // Debug text into out (>= WM_PACKET_LEN bytes)

/**
 * @brief One forward + reverse excitation of the probe.
 * Interrupts are disabled for the whole excitation so both directions last
 * equally long (no net DC current through the electrodes).
 * @param fwd Receives the forward ADC code (only on success).
 * @param rev Receives the reverse ADC code (only on success).
 * @return 1 on success; 0 if a conversion failed (excitation already removed).
 */
uint8_t watermark_sample(uint16_t *fwd, uint16_t *rev);

/* Probe resistance derived from a single fwd/rev pair, Ohm (999999 if a code is at the rail). */
float watermark_resistance(uint16_t fwd, uint16_t rev);

/* Fill status and cb from an already averaged d->r_ohm (soil_temp_c used for compensation). */
void  watermark_classify(watermark_data_t *d, float soil_temp_c);

#ifdef __cplusplus
}
#endif

#endif
