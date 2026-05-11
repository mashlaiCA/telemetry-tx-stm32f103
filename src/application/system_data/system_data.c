#include "system_data.h"
#include "devices/Sensor_SHT35/sensor_sht35.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture_fsm.h"
#include "stm32f1xx.h"

uint16_t count;
system_data_t system_data = {0};

void system_data_run(void)
{
    uint8_t flags;

    /* tiny critical section: copy flags */
    __disable_irq();
    flags = system_data.ready_sensors_flag;
    __enable_irq();

    if (flags & DATA_SHT35_READY)
    {
        sensor_update_SHT35(&system_data);
    }

    if (flags & DATA_ANALOG_READY)
    {
        sensor_update_soil_moisture(&system_data);
    }

    /* both ready? create & send */
    if ((flags & (DATA_SHT35_READY | DATA_ANALOG_READY)) == (DATA_SHT35_READY | DATA_ANALOG_READY))
    {
        data_creation(&system_data);
        system_data.ready_data_creation_flag = 1;
        // do not call uart_send_string here; let LoRa FSM perform transmission
    }

    /* clear only the bits we processed (atomic) */
    if (flags)
    {
        __disable_irq();
        system_data.ready_sensors_flag &= ~flags;
        __enable_irq();
    }
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

    data->soil_moisture_10 = system_analog_sensors_data.sensor_soil_10;
}

void data_creation(system_data_t *data)
{
    char *p = data->data_string;

    p = int_to_str(data->soil_moisture_10, p);
    *p++ = ',';

    p = int_to_str(data->humidity, p);

    *p++ = ',';

    p = int_to_str(data->temperature, p);

    *p = '\0';
}
