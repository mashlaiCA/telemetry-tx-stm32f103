
#ifndef DS3231_RTC_H
#define DS3231_RTC_H

#include <stdint.h>
#include "drivers/I2C/i2c_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} ds3231_time_t;

I2C_Status_t ds3231_init(void);
uint8_t ds3231_lost_power(void);
I2C_Status_t ds3231_set_time(const ds3231_time_t *t);
I2C_Status_t ds3231_get_time(ds3231_time_t *t);
I2C_Status_t ds3231_set_alarm_in(uint8_t seconds_ahead);
void ds3231_datetime_to_str(const ds3231_time_t *t, char *buf);

#ifdef __cplusplus
}
#endif

#endif 
