#ifndef SYSTEM_ERROR_H
#define SYSTEM_ERROR_H

#include "stdint.h"

typedef enum
{
    system_error_none = 0,
    system_error_i2c,
    system_error_sht35,
} System_Error_t;

extern System_Error_t system_error_flags;

void set_system_error(System_Error_t error);
System_Error_t get_system_error(void);

#endif