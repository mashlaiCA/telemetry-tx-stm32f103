#include "i2c_errors.h"




I2C_Status_t get_last_i2c_error(void)
{
    return i2c_status_error;
}   

SHT35_Status_t get_last_sht35_error(void)
{
    return sht35_status;
}