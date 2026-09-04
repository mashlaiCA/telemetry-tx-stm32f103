#ifndef WATERMARK_200SS_H
#define WATERMARK_200SS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WM_CB_SHORT   240
#define WM_CB_OPEN    255

#define WM_PACKET_LEN   56

#define WM_SAMPLES        3
#define WM_REST_MS        5

typedef enum {
    WM_OK = 0,
    WM_SHORT,
    WM_OPEN
} wm_status_t;

typedef struct {
    uint16_t     raw_fwd;
    uint16_t     raw_rev;
    float        r_ohm;
    float        cb;
    wm_status_t  status;
} watermark_data_t;

void             watermark_init(void);
watermark_data_t watermark_read(float soil_temp_c);
void watermark_pack_string(const watermark_data_t *d, float ntc_temp, char *out);

/* Non-blocking primitives used by the watermark FSM.
   They split watermark_read() into steps so the caller keeps control of timing. */

/* One forward/reverse excitation, ~2 * WM_SET_US with interrupts disabled. */
void  watermark_sample(uint16_t *fwd, uint16_t *rev);

/* Probe resistance derived from a single fwd/rev pair. */
float watermark_resistance(uint16_t fwd, uint16_t rev);

/* Fill status and cb from an already averaged d->r_ohm. */
void  watermark_classify(watermark_data_t *d, float soil_temp_c);

#ifdef __cplusplus
}
#endif

#endif
