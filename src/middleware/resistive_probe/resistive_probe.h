#ifndef RESISTIVE_PROBE_H
#define RESISTIVE_PROBE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    uint8_t pin_a;
    uint8_t pin_b;
    uint8_t adc_channel;
    uint16_t set_us;

} resistive_probe_t;

void resistive_probe_init(const resistive_probe_t *p);

uint16_t resistive_probe_read(const resistive_probe_t *p);

#ifdef __cplusplus
}
#endif

#endif 