#include "analog_sensor_soil_moisture.h"
#include "analog_sensors_manager.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpio_hw.h"



static analog_sensor_t sensors[SENSOR_COUNT];

void analog_sensors_init(void)
{
    adc1_init();
}

int8_t analog_sensor_add(uint8_t pin,
                         uint16_t cal_dry,
                         uint16_t cal_wet)
{
    for (uint8_t i = 0; i < SENSOR_COUNT; i++)
    {
        if (!sensors[i].enabled)
        {
            sensors[i].pin = pin;

            sensors[i].cal_dry = cal_dry;

            sensors[i].cal_wet = cal_wet;

            sensors[i].current_value = 0;

            sensors[i].previous_value = -1.0f;

           // sensors[i].measurement_index = 0;

            sensors[i].enabled = 1;
/*
            for (uint8_t k = 0; k < AVERAGE_COUNT; k++)
            {
                sensors[i].measurements[k] = 0;
            }
*/
            gpio_a_analog_input_init(pin);

            for (uint8_t i = 0; i < 5; i++)
            {
                adc1_read(pin);
            }
            

            return i;
        }
    }

    return -1;
}

int16_t analog_sensor_read_average(uint8_t sensor_id)
{
    if (sensor_id >= SENSOR_COUNT)
    {
        return -1;
    }

    analog_sensor_t *sensor = &sensors[sensor_id];

    if (!sensor->enabled)
    {
        return -1;
    }

    if (sensor->cal_dry == sensor->cal_wet)
    {
        return -1;
    }

    uint16_t raw =
        adc1_read(sensor->pin);

    float percent = 0.0f;
    if (sensor->cal_wet != sensor->cal_dry)
    {
        percent = ((float)raw - (float)sensor->cal_dry) /
                  ((float)sensor->cal_wet - (float)sensor->cal_dry) * 100.0f;
    }
    
    if (percent > 100.0f) percent = 100.0f;
    

    if (percent < 0.0f) percent = 0.0f;

    float smooth;

    if (sensor->previous_value < 0.0f) smooth = percent;

    else
    {
        smooth = 0.2f * sensor->previous_value + 0.8f * percent;
    }

    sensor->previous_value = smooth;

    //sensor->measurements[sensor->measurement_index] = (uint16_t)smooth;

    //sensor->measurement_index++;

/*
    if (sensor->measurement_index >= AVERAGE_COUNT)
    {
        uint32_t sum = 0;
        

        for (uint8_t i = 0; i < AVERAGE_COUNT; i++)
        {
            sum += sensor->measurements[i];
        }

        sensor->current_value =
            (uint16_t)(sum / AVERAGE_COUNT);

        sensor->measurement_index = 0;

    }
*/
  return sensor->current_value = (uint16_t)smooth;
}