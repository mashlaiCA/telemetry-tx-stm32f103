#include "i2c_errors.h"


I2C_Status_t get_last_i2c_error(void) // Function to retrieve the last I2C error status
{
    return i2c_status_error; // Return the last I2C error status
}
