#include "system_data.h"
#include "devices/Sensor_SHT35/sensor_sht35.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture_fsm.h"
#include "devices/analog_sensor_soil_moisture/analog_leaf_sensor.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"

#include "stm32f1xx.h"


uint16_t count;
system_data_t system_data = {0};

#define DATA_ALL_READY (DATA_SHT35_READY | DATA_ANALOG_READY)

/* Sensor bits already folded into system_data, held until the payload is built. */
static uint8_t processed_flags = 0;

void system_data_run(void)
{
    uint8_t flags;

    /* tiny critical section: copy flags */
    __disable_irq();
    flags = system_data.ready_sensors_flag;
    __enable_irq();

    /* Each sensor is sampled once, when its bit first shows up. */
    if ((flags & DATA_SHT35_READY) && !(processed_flags & DATA_SHT35_READY))
    {
        sensor_update_SHT35(&system_data);
        processed_flags |= DATA_SHT35_READY;
    }

    if ((flags & DATA_ANALOG_READY) && !(processed_flags & DATA_ANALOG_READY))
    {
        sensor_update_soil_moisture(&system_data);
        sensor_update_leaf_sensor(&system_data);
        processed_flags |= DATA_ANALOG_READY;
    }

    /* The two sensor FSMs finish several loop passes apart, so the bits are
       latched until both have reported. Clearing them on every call meant the
       two never overlapped and the payload was never built. */
    if ((flags & DATA_ALL_READY) != DATA_ALL_READY)
    {
        return;
    }

    /* Do not rebuild data_string while the LoRa FSM still needs the previous
       one - it transmits straight out of this buffer. */
    if (system_data.ready_data_creation_flag != 0 || system_data.lora_busy != 0)
    {
        return;
    }

    data_creation(&system_data);
    system_data.ready_data_creation_flag = 1;
    processed_flags = 0;

    /* release both sensor FSMs for the next measurement (atomic) */
    __disable_irq();
    system_data.ready_sensors_flag &= ~DATA_ALL_READY;
    __enable_irq();
}

char *int_to_str(int value, char *str)
{
    char buffer[12];
    int i = 0, j;

    if (value == 0)
    {
        *str++ = '0';
        return str;
    }

    if (value < 0)
    {
        *str++ = '-';
        value = -value;
    }

    while (value > 0)
    {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }

    for (j = i - 1; j >= 0; j--)
    {
        *str++ = buffer[j];
    }

    return str;
}

void sensor_update_SHT35(system_data_t *data)
{
    data->humidity = humiditySHT35();
    data->temperature = temperatureSHT35();
}
void sensor_update_soil_moisture(system_data_t *data)
{
    data->soil_moisture_10 = soil_sensor_read_average();
}

void sensor_update_leaf_sensor(system_data_t *data)
{
    data->leaf_moisture = leaf_wetness_read();
}

void data_creation(system_data_t *data)
{
    char *p = data->data_string;

    p = int_to_str(data->soil_moisture_10, p);
    *p++ = ',';

    p = int_to_str(data->humidity, p);

    *p++ = ',';

    p = int_to_str(data->temperature, p);

    *p++ = ',';

    p = int_to_str(data->leaf_moisture, p);

    *p = '\0';
}
