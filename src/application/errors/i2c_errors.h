#ifndef I2C_ERRORS_H
#define I2C_ERRORS_H
#include "../../drivers/I2C/i2c_hw.h"
#include "../../drivers/sht35/sht35.h"

I2C_Status_t get_last_i2c_error(void);

SHT35_Status_t get_last_sht35_error(void);

#endif 