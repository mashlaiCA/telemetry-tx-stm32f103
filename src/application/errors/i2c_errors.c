#include "i2c_errors.h"

/* Getter for the shared I2C status. */
I2C_Status_t get_last_i2c_error(void)
{
    return i2c_status_error; // Status of the most recent transfer on the bus
}
