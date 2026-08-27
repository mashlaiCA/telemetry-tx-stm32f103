#ifndef ANALOG_LEAF_SENSOR_H
#define ANALOG_LEAF_SENSOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    LEAF_DRY = 0,
    LEAF_WET = 1,
    LEAF_FAULT = 2
} leaf_state_t;

typedef struct {
    uint16_t     raw;            
    leaf_state_t state;    
} leaf_data_t;

void leaf_wetness_init(void);
uint16_t leaf_wetness_read(void);


#ifdef __cplusplus
}
#endif

#endif