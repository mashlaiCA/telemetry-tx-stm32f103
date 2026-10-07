
#include "ds3231_rtc.h"

/* DS3231 = 0x68, SHT35 = 0x44 - no address conflict on the shared I2C1
   bus. The ZS-042 module also carries an AT24C32 EEPROM (0x57), which
   doesn't overlap either. */
#define DS3231_ADDR               0x68 // 7-bit I2C address
#define REG_TIME                  0x00 // Seconds; 0x00..0x06 = sec, min, hour, day, date, month, year
#define REG_ALARM1_SEC            0x07 // Alarm1 seconds (bit 7 = A1M1)
#define REG_ALARM1_MIN            0x08 // Alarm1 minutes (bit 7 = A1M2)
#define REG_ALARM1_HOUR           0x09 // Alarm1 hours (bit 7 = A1M3)
#define REG_ALARM1_DAYDATE        0x0A // Alarm1 day/date (bit 7 = A1M4)
#define REG_CONTROL               0x0E // Control register
#define REG_STATUS                0x0F // Status register

/* Named bits instead of magic numbers, so a read-modify-write does not
   accidentally clear an unrelated bit. */
#define CTRL_A1IE                 0x01  // Alarm1 interrupt enable
#define CTRL_A2IE                 0x02  // Alarm2 interrupt enable
#define CTRL_INTCN                0x04  // INT/SQW pin outputs the alarm interrupt
#define STATUS_A1F                0x01  // Alarm1 flag
#define STATUS_A2F                0x02  // Alarm2 flag
#define STATUS_EN32KHZ            0x08  // 32 kHz output enable
#define STATUS_OSF                0x80  // Oscillator stop flag (power was lost)

/* Packed BCD -> binary. */
static uint8_t bcd_to_bin(uint8_t v){
    return (v >> 4) * 10u + (v & 0x0F); // Tens nibble * 10 + units nibble
}

/* Binary (0..99) -> packed BCD. */
static uint8_t bin_to_bcd(uint8_t v){
    return ((v / 10u) << 4) | (v % 10u); // Tens in the high nibble, units in the low nibble
}

/* Writes v (0..99) as two ASCII digits at out (no terminator). */
static void byte_to_dec2(uint8_t v, char *out)
{
    out[0] = '0' + (v / 10u); // Tens digit
    out[1] = '0' + (v % 10u); // Units digit
}

/* Reads n consecutive registers starting at reg: write the register pointer, then read. */
static I2C_Status_t ds3231_read_regs(uint8_t reg, uint8_t *buf, uint8_t n){
    I2C_Status_t st = I2C_Write(DS3231_ADDR, &reg, 1); // Set the register pointer
    if (st != i2c_ok) return st;

    return I2C_Read(DS3231_ADDR, buf, n); // Read n bytes (the pointer auto-increments)
}

/* Writes one register. */
static I2C_Status_t ds3231_write_reg(uint8_t reg, uint8_t val){
    uint8_t buf[2] = {reg, val}; // Register pointer + value
    return I2C_Write(DS3231_ADDR, buf, 2);
}

/* Configures INT/SQW for alarms and clears stale flags (see ds3231_rtc.h). */
I2C_Status_t ds3231_init(void){
    uint8_t status; // Status register value

    // INTCN=1, both alarms disabled for now -> no square wave on INT/SQW,
    // the pin is held at "1" by the external/internal pull-up.
    I2C_Status_t st = ds3231_write_reg(REG_CONTROL, CTRL_INTCN);
    if(st != i2c_ok) return st;

    st = ds3231_read_regs(REG_STATUS, &status, 1);
    if (st != i2c_ok) return st;

    /* Together with EN32kHz, leftover alarm flags are cleared here: if A1F
       remained set from a previous cycle, INT would already be low, no new
       falling edge could occur on PA0 and the MCU would not wake up. */
    return ds3231_write_reg(REG_STATUS,
                            (uint8_t)(status & ~(STATUS_EN32KHZ | STATUS_A1F | STATUS_A2F)));
}

/* 1 if OSF is set or the RTC cannot be read. */
uint8_t ds3231_lost_power(void){
    uint8_t status;

    if (ds3231_read_regs(REG_STATUS, &status, 1) != i2c_ok) return 1; // Unreadable: treat as invalid time
    return (status & STATUS_OSF) ? 1 : 0; // OSF = oscillator stopped at some point
}


/* Writes date/time (registers 0x00..0x06) and clears OSF. */
I2C_Status_t ds3231_set_time(const ds3231_time_t *t)
{
    uint8_t buf[8]; // Register pointer + 7 time registers
    uint8_t status;

    buf[0] = REG_TIME;
    buf[1] = bin_to_bcd(t->sec);
    buf[2] = bin_to_bcd(t->min);
    buf[3] = bin_to_bcd(t->hour);      // Bit 6 = 0: 24-hour mode
    buf[4] = bin_to_bcd(t->day);
    buf[5] = bin_to_bcd(t->date);
    buf[6] = bin_to_bcd(t->month);     // Bit 7 (century) = 0
    buf[7] = bin_to_bcd(t->year);

    I2C_Status_t st = I2C_Write(DS3231_ADDR, buf, 8); // One burst write of all 7 registers
    if (st != i2c_ok) return st;


    st = ds3231_read_regs(REG_STATUS, &status, 1);
    if (st != i2c_ok) return st;

    return ds3231_write_reg(REG_STATUS, (uint8_t)(status & ~STATUS_OSF)); // Mark the time as valid
}

/* Reads date/time (registers 0x00..0x06), masking control bits. */
I2C_Status_t ds3231_get_time(ds3231_time_t *t)
{
    uint8_t buf[7]; // Raw BCD registers

    I2C_Status_t st = ds3231_read_regs(REG_TIME, buf, 7);
    if (st != i2c_ok) return st;

    t->sec   = bcd_to_bin(buf[0] & 0x7F);
    t->min   = bcd_to_bin(buf[1] & 0x7F);
    t->hour  = bcd_to_bin(buf[2] & 0x3F);  // 24-hour mode: bits 5..0 are the BCD hour
    t->day   = bcd_to_bin(buf[3] & 0x07);
    t->date  = bcd_to_bin(buf[4] & 0x3F);
    t->month = bcd_to_bin(buf[5] & 0x1F);   // Bit 7 = century flag, masked out
    t->year  = bcd_to_bin(buf[6]);

    return i2c_ok;
}

/* Arms Alarm1 at hh:mm:ss and confirms A1F is clear (see ds3231_rtc.h). */
I2C_Status_t ds3231_set_alarm_at(uint8_t hour, uint8_t min, uint8_t sec)
{
    uint8_t buf[5]; // Register pointer + 4 Alarm1 registers
    uint8_t status;

    if (hour > 23u || min > 59u || sec > 59u)
    {
        return i2c_error; // Invalid target - better not to set the alarm at all
    }


    buf[0] = REG_ALARM1_SEC;
    buf[1] = bin_to_bcd(sec);   // A1M1=0: match seconds
    buf[2] = bin_to_bcd(min);   // A1M2=0: match minutes
    buf[3] = bin_to_bcd(hour);  // A1M3=0, 24-hour mode: match hours
    buf[4] = 0x80;              // A1M4=1: ignore date, match hours+min+sec only

    I2C_Status_t st = I2C_Write(DS3231_ADDR, buf, 5);
    if (st != i2c_ok) return st;

    st = ds3231_write_reg(REG_CONTROL, CTRL_INTCN | CTRL_A1IE); // INT/SQW = alarm interrupt, Alarm1 enabled
    if (st != i2c_ok) return st;


    st = ds3231_read_regs(REG_STATUS, &status, 1);
    if (st != i2c_ok) return st;

    st = ds3231_write_reg(REG_STATUS, (uint8_t)(status & ~(STATUS_A1F | STATUS_A2F))); // Clear both alarm flags, keep other bits
    if (st != i2c_ok) return st;


    /* Read back: while A1F stays set INT/SQW is held low and the alarm can
       never produce a falling edge, so the caller must not go to sleep. */
    st = ds3231_read_regs(REG_STATUS, &status, 1);
    if (st != i2c_ok) return st;

    if (status & STATUS_A1F)
    {
        return i2c_error; // A1F did not clear
    }

    return i2c_ok;
}


/* Arms Alarm1 seconds_ahead seconds from the current RTC time. */
I2C_Status_t ds3231_set_alarm_in(uint8_t seconds_ahead)
{
    uint8_t buf[3]; // Raw sec, min, hour

    /* An alarm set "in the past" (or for the current second) would not fire
       until the same time next day, and the MCU would not wake up. A minimum
       of 2 s ahead guarantees that a rollover of the seconds register between
       reading the time and writing the alarm cannot reach the target. */
    if (seconds_ahead < 2u)
    {
        seconds_ahead = 2u;
    }

    I2C_Status_t st = ds3231_read_regs(REG_TIME, buf, 3); // Read current sec, min, hour
    if (st != i2c_ok) return st;

    uint8_t sec  = bcd_to_bin(buf[0] & 0x7F);
    uint8_t min  = bcd_to_bin(buf[1] & 0x7F);
    uint8_t hour = bcd_to_bin(buf[2] & 0x3F);

    uint16_t total_sec = sec + seconds_ahead;
    sec = total_sec % 60;               // Seconds after add, wrapped to 0-59
    uint8_t carry_min = total_sec / 60; // Minutes carried out of the seconds add

    uint16_t total_min = min + carry_min;
    min = total_min % 60;                // Minutes after carry, wrapped to 0-59
    uint8_t carry_hour = total_min / 60; // Hours carried out of the minutes add

    hour = (hour + carry_hour) % 24; // Hours after carry, wrapped at 24

    return ds3231_set_alarm_at(hour, min, sec);
}

/* Reads A1F; an unreachable RTC is reported as "flag set". */
uint8_t ds3231_alarm_flag(void)
{
    uint8_t status;

    if (ds3231_read_regs(REG_STATUS, &status, 1) != i2c_ok)
    {
        return 1; // Cannot talk to the RTC: treat as "flag set" so we do not sleep
    }

    return (status & STATUS_A1F) ? 1u : 0u;
}

/* Clears A1F/A2F if set, releasing INT/SQW. */
I2C_Status_t ds3231_clear_alarm_flag(void)
{
    uint8_t status;

    I2C_Status_t st = ds3231_read_regs(REG_STATUS, &status, 1);
    if (st != i2c_ok) return st;

    if (!(status & (STATUS_A1F | STATUS_A2F)))
    {
        return i2c_ok; // Already clear, no bus traffic needed
    }

    return ds3231_write_reg(REG_STATUS, (uint8_t)(status & ~(STATUS_A1F | STATUS_A2F))); // Clear both flags, keep the rest
}

/* Formats "DD-MM-20YY  HH:MM:SS AM\r\n" with a 12-hour clock (buf >= 26 bytes). */
void ds3231_datetime_to_str(const ds3231_time_t *t, char *buf)
{
    uint8_t hour12; // Hour on a 12-hour clock, 1..12
    char ampm[2];   // "AM" / "PM"

    if (t->hour == 0)
    {
        hour12 = 12; // 00:xx -> 12 AM
        ampm[0] = 'A'; ampm[1] = 'M';
    }
    else if (t->hour < 12)
    {
        hour12 = t->hour; // 01..11 AM
        ampm[0] = 'A'; ampm[1] = 'M';
    }
    else if (t->hour == 12)
    {
        hour12 = 12; // 12:xx -> 12 PM
        ampm[0] = 'P'; ampm[1] = 'M';
    }
    else
    {
        hour12 = t->hour - 12; // 13..23 -> 1..11 PM
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
