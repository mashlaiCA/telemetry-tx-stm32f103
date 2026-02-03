#ifndef SENSOR_SHT35_H
#define SENSOR_SHT35_H


#include "stdint.h"

typedef enum
{
    st_idle = 0,
    st_start_sensor,
    st_wait_measurement,
    st_read_data_measurement,
    st_crc_check,
    st_calculate_data,
    st_error,

    st_count
} SHT35_State_t;


uint16_t temperatureSHT35(void);
uint16_t humiditySHT35(void);

typedef SHT35_State_t (*StateFunction_t)(void);

extern SHT35_State_t sht35_state;

SHT35_State_t State_Idle(void);
SHT35_State_t State_Start_Sensor(void);
SHT35_State_t State_Wait_Measurement(void);
SHT35_State_t State_Read_Data_Measurement(void);
SHT35_State_t State_CRC_Check(void);
SHT35_State_t State_Calculate_Data(void);
SHT35_State_t State_Error(void);

void Sensor_FSM_Run(void);
#endif
