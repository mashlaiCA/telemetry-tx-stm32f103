#include "system_data.h"
#include "devices/Sensor_SHT35/sensor_sht35.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture_fsm.h"
#include "devices/analog_sensor_soil_moisture/analog_leaf_sensor.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"
#include "devices/watermark_200ss/watermark_fsm.h"

#include "devices/ds3231_rtc/ds3231_rtc.h"
#include "protocols/uart/uart.h"

#include "stm32f1xx.h"


uint16_t count; // Not used in this file; main.cpp also defines a global named count
system_data_t system_data = {0}; // All values 0, no flags set

#define DATA_ALL_READY (DATA_SHT35_READY | DATA_ANALOG_READY | DATA_WATERMARK_READY) // Every source required for a packet


static uint8_t processed_flags = 0; // DATA_*_READY bits already copied into system_data in this cycle

/* Collects sensor results as their ready bits appear and builds the payload
   once all sources are ready and no previous packet is waiting. */
void system_data_run(void)
{
    uint8_t flags; // Snapshot of ready_sensors_flag


    /* Read the flags inside a critical section so the snapshot and the clear
       at the end stay consistent if an ISR ever modifies them (currently only
       main-loop code writes ready_sensors_flag). */
    __disable_irq();
    flags = system_data.ready_sensors_flag;
    __enable_irq();


    if ((flags & DATA_SHT35_READY) && !(processed_flags & DATA_SHT35_READY)) // New SHT35 result not copied yet
    {
        sensor_update_SHT35(&system_data);   // Copy temperature/humidity
        processed_flags |= DATA_SHT35_READY; // Copy only once per packet
    }

    if ((flags & DATA_ANALOG_READY) && !(processed_flags & DATA_ANALOG_READY)) // Leaf sensor not read yet in this cycle
    {

        sensor_update_leaf_sensor(&system_data); // Blocking leaf measurement
        processed_flags |= DATA_ANALOG_READY;    // Read only once per packet
    }


    if ((flags & DATA_ALL_READY) != DATA_ALL_READY)
    {
        return; // Wait for the remaining sources (Watermark is consumed in data_creation())
    }


    if (system_data.ready_data_creation_flag != 0 || system_data.lora_busy != 0)
    {
        return; // Previous packet not transmitted yet: do not overwrite data_string
    }

    data_creation(&system_data);          // Build data_string
    system_data.ready_data_creation_flag = 1; // Hand it to the LoRa FSM
    processed_flags = 0;                  // Next packet copies fresh values


    /* Clear the consumed ready bits so the sensor FSMs start new measurements. */
    __disable_irq();
    system_data.ready_sensors_flag &= ~DATA_ALL_READY;
    __enable_irq();
}

/* Writes value as decimal text at str (no terminator); returns the position after it. */
char *int_to_str(int value, char *str)
{
    char buffer[12]; // Digits in reverse order (up to 10 digits for int)
    int i = 0, j;

    if (value == 0)
    {
        *str++ = '0'; // Special case: the loop below would produce no digits
        return str;
    }

    if (value < 0)
    {
        *str++ = '-';   // Sign
        value = -value; // Work with the magnitude
    }

    while (value > 0)
    {
        buffer[i++] = (value % 10) + '0'; // Least significant digit first
        value /= 10;
    }

    for (j = i - 1; j >= 0; j--)
    {
        *str++ = buffer[j]; // Copy digits most significant first
    }

    return str;
}

/* Copies the last SHT35 result (whole degC / %RH). */
void sensor_update_SHT35(system_data_t *data)
{
    data->humidity = humiditySHT35();
    data->temperature = temperatureSHT35();
}
/* Copies the analog soil sensor value (one smoothed reading). Not called in
   the current data path: soil_moisture_10 is filled from Watermark in data_creation(). */
void sensor_update_soil_moisture(system_data_t *data)
{
    data->soil_moisture_10 = soil_sensor_read_average();
}

/* Measures the leaf sensor (blocking, 8 forward/reverse probe reads). */
void sensor_update_leaf_sensor(system_data_t *data)
{
    data->leaf_moisture = leaf_wetness_read(); // Averaged raw value 0..4095
}


/* Writes v as exactly 2 decimal digits (v mod 100), so field widths are fixed
   even for out-of-range RTC values. */
static char *pack_two_digits(char *p, uint8_t v)
{
    *p++ = (char)('0' + (v / 10u) % 10u); // Tens digit
    *p++ = (char)('0' + (v % 10u));       // Units digit

    return p;
}

/* Appends ",DD,MM,YYYY,hh,mm,ss" from the DS3231, or ",E,E,E,E,E,E" if it does not respond. */
static char *pack_rtc_time(char *p)
{
    ds3231_time_t now;

    if (ds3231_get_time(&now) != i2c_ok)
    {
        return pack_str(p, ",E,E,E,E,E,E"); // 6 error fields: DD,MM,YYYY,hh,mm,ss
    }

    /* date ",DD,MM,YYYY" */
    p = pack_str(p, ",");
    p = pack_two_digits(p, now.date);
    p = pack_str(p, ",");
    p = pack_two_digits(p, now.month);
    p = pack_str(p, ",20"); // Century: DS3231 stores a 2-digit year
    p = pack_two_digits(p, now.year);

    /* time ",hh,mm,ss" */
    p = pack_str(p, ",");
    p = pack_two_digits(p, now.hour);
    p = pack_str(p, ",");
    p = pack_two_digits(p, now.min);
    p = pack_str(p, ",");
    p = pack_two_digits(p, now.sec);

    return p;
}

/* Builds data_string: "cb,humidity,temperature,leaf,DD,MM,YYYY,hh,mm,ss". */
void data_creation(system_data_t *data)
{
    char *p = data->data_string; // Write position

    /* soil_moisture_10 = cb from Watermark. This is the single source: the same
       field goes to LoRa (via data_string) and to any UART output that uses it.
       Rounded to an integer because int_to_str() prints integers only. The cb
       range is guaranteed by watermark_classify() (0..239, 240 SHORT, 255 OPEN),
       but NaN or an out-of-range value is still mapped to WM_CB_OPEN, so an
       explicit fault code goes on air instead of garbage from a float -> uint16 cast. */
    float cb = watermark_get_cb();

    if (!(cb >= 0.0f) || cb > (float)WM_CB_OPEN) // !(cb >= 0) is also true for NaN
    {
        cb = (float)WM_CB_OPEN;
    }

    data->soil_moisture_10 = (uint16_t)(cb + 0.5f); // Round to nearest

    p = int_to_str(data->soil_moisture_10, p); // Field 1: cb
    *p++ = ',';

    p = int_to_str(data->humidity, p); // Field 2: %RH

    *p++ = ',';

    p = int_to_str(data->temperature, p); // Field 3: degC

    *p++ = ',';

    p = int_to_str(data->leaf_moisture, p); // Field 4: leaf raw value

    p = pack_rtc_time(p); // Fields 5-10: date/time from DS3231 at the moment the packet is built

    *p = '\0';
}
