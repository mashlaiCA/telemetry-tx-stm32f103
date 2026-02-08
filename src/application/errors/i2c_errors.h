#ifndef I2C_ERRORS_H
#define I2C_ERRORS_H
#include "../../drivers/I2C/i2c_hw.h"
#include "../../drivers/sht35/sht35.h"

/**
 * @brief Get the last I2C error status.
 * @return The last I2C error status as an I2C_Status_t value.
 */
I2C_Status_t get_last_i2c_error(void);


#endif 