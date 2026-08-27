
#include "ds3231_rtc.h"

#define DS3231_ADDR               0x68
#define REG_TIME                  0x00
#define REG_CONTROL               0x0E
#define REG_STATUS                0x0F

static uint8_t bcd_to_bin(uint8_t v){
    return (v >> 4) * 10u + (v & 0x0F);
}

static uint8_t bin_to_bcd(uint8_t v){
    return ((v / 10u) << 4) | (v % 10u);
}

static void byte_to_dec2(uint8_t v, char *out)
{
    out[0] = '0' + (v / 10u);
    out[1] = '0' + (v % 10u);
}

static I2C_Status_t ds3231_read_regs(uint8_t reg, uint8_t *buf, uint8_t n){
    I2C_Status_t st = I2C_Write(DS3231_ADDR, &reg, 1);
    if (st != i2c_ok) return st;

    return I2C_Read(DS3231_ADDR, buf, n);
}

static I2C_Status_t ds3231_write_reg(uint8_t reg, uint8_t val){
    uint8_t buf[2] = {reg, val};
    return I2C_Write(DS3231_ADDR, buf, 2);
}

I2C_Status_t ds3231_init(void){
    uint8_t status;

    I2C_Status_t st = ds3231_write_reg(REG_CONTROL, 0x04);
    if(st != i2c_ok) return st;

    st = ds3231_read_regs(REG_STATUS, &status, 1);
    if (st != i2c_ok) return st;

    return ds3231_write_reg(REG_STATUS, status & ~0x08);
}

uint8_t ds3231_lost_power(void){
    uint8_t status;

    if (ds3231_read_regs(REG_STATUS, &status, 1) != i2c_ok) return 1;
    return (status & 0x80) ? 1 : 0;
}


I2C_Status_t ds3231_set_time(const ds3231_time_t *t)
{
    uint8_t buf[8];
    uint8_t status;

    buf[0] = REG_TIME;
    buf[1] = bin_to_bcd(t->sec);
    buf[2] = bin_to_bcd(t->min);
    buf[3] = bin_to_bcd(t->hour);      
    buf[4] = bin_to_bcd(t->day);
    buf[5] = bin_to_bcd(t->date);
    buf[6] = bin_to_bcd(t->month);     
    buf[7] = bin_to_bcd(t->year);

    I2C_Status_t st = I2C_Write(DS3231_ADDR, buf, 8);
    if (st != i2c_ok) return st;

    
    st = ds3231_read_regs(REG_STATUS, &status, 1);
    if (st != i2c_ok) return st;

    return ds3231_write_reg(REG_STATUS, status & ~0x80);
}

I2C_Status_t ds3231_get_time(ds3231_time_t *t)
{
    uint8_t buf[7];

    I2C_Status_t st = ds3231_read_regs(REG_TIME, buf, 7);
    if (st != i2c_ok) return st;

    t->sec   = bcd_to_bin(buf[0] & 0x7F);
    t->min   = bcd_to_bin(buf[1] & 0x7F);
    t->hour  = bcd_to_bin(buf[2] & 0x3F);  
    t->day   = bcd_to_bin(buf[3] & 0x07);
    t->date  = bcd_to_bin(buf[4] & 0x3F);
    t->month = bcd_to_bin(buf[5] & 0x1F);   
    t->year  = bcd_to_bin(buf[6]);

    return i2c_ok;
}

void ds3231_datetime_to_str(const ds3231_time_t *t, char *buf)
{
    uint8_t hour12;
    char ampm[2];

    if (t->hour == 0)
    {
        hour12 = 12;
        ampm[0] = 'A'; ampm[1] = 'M';
    }
    else if (t->hour < 12)
    {
        hour12 = t->hour;
        ampm[0] = 'A'; ampm[1] = 'M';
    }
    else if (t->hour == 12)
    {
        hour12 = 12;
        ampm[0] = 'P'; ampm[1] = 'M';
    }
    else
    {
        hour12 = t->hour - 12;
        ampm[0] = 'P'; ampm[1] = 'M';
    }

    // "DD-MM-20YY  HH:MM:SS AM\r\n\0"
    byte_to_dec2(t->date,  &buf[0]);
    buf[2] = '-';
    byte_to_dec2(t->month, &buf[3]);
    buf[5] = '-';
    buf[6] = '2';
    buf[7] = '0';
    byte_to_dec2(t->year,  &buf[8]);
    buf[10] = ' ';
    buf[11] = ' ';
    byte_to_dec2(hour12,   &buf[12]);
    buf[14] = ':';
    byte_to_dec2(t->min,   &buf[15]);
    buf[17] = ':';
    byte_to_dec2(t->sec,   &buf[18]);
    buf[20] = ' ';
    buf[21] = ampm[0];
    buf[22] = ampm[1];
    buf[23] = '\r';
    buf[24] = '\n';
    buf[25] = '\0';
}
    