#ifndef NTC_H
#define NTC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ntc_init(void);
float temp_c_ntc(uint8_t chanel_adc);

#ifdef __cplusplus
}
#endif

#endif 