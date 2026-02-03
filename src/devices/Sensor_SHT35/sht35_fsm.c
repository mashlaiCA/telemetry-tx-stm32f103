#include "sensor_sht35.h"
#include "application/protocols/i2c/i2c.h"
#include "drivers/sht35/sht35.h"
#include "application/time/scheduler.h"

#include "application/errors/i2c_errors.h"


StateFunction_t state_table[st_count] = {
    State_Idle,
    State_Start_Sensor,
    State_Wait_Measurement,
    State_Read_Data_Measurement,
    State_CRC_Check,
    State_Calculate_Data,
    State_Error};


SHT35_State_t sht35_state = st_idle;

SHT35_State_t State_Idle(void)
{
    if (scheduler_delay_ms(50))
    {
        return st_error;
    }
    return st_start_sensor;
}

SHT35_State_t State_Start_Sensor(void)
{
    I2C_Write_Sensor_SHT35();
    if (get_last_i2c_error() != i2c_ok)
    {
        return st_error;
    }
    return st_wait_measurement;
}

SHT35_State_t State_Wait_Measurement(void)
{
    if (scheduler_delay_ms(70))
    {
        return st_error;
    }
    return st_read_data_measurement;
}

SHT35_State_t State_Read_Data_Measurement(void)
{
    I2C_Read_Sensor_SHT35();
    if (get_last_i2c_error() != i2c_ok)
    {
        return st_error;
    }
    return st_crc_check;
}


SHT35_State_t State_CRC_Check(void)
{
    SHT35_CRC_Check();

    if (get_last_sht35_error() != sht35_ok)
    {
        return st_error;
    }
    return st_calculate_data;
}
SHT35_State_t State_Calculate_Data(void)
{
    SHT35_Calculate();
    if (get_last_sht35_error() != sht35_ok)
    {
        return st_error;
    }
    return st_idle;
}

SHT35_State_t State_Error(void)
{
    return st_idle;
}

void Sensor_FSM_Run(void)
{
   sht35_state = state_table[sht35_state]();
}