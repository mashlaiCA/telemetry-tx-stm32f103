#include "analog_sensor_soil_moisture.h"
#include "analog_sensors_manager.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpio_hw.h"



static analog_sensor_t sensors[SENSOR_COUNT]; // Sensor slots (zero-initialized: all disabled)

/* Initializes the ADC shared by all analog sensors. */
void analog_sensors_init(void)
{
    adc1_init(); // ADC1 clock, power-up and calibration
}

/* Registers a sensor in the first free slot (see analog_sensors_manager.h). */
int8_t analog_sensor_add(uint8_t pin,
                         uint16_t cal_dry,
                         uint16_t cal_wet)
{
    for (uint8_t i = 0; i < SENSOR_COUNT; i++) // Search for a free slot
    {
        if (!sensors[i].enabled)
        {
            sensors[i].pin = pin; // ADC channel

            sensors[i].cal_dry = cal_dry; // Raw code for 0 %

            sensors[i].cal_wet = cal_wet; // Raw code for 100 %

            sensors[i].current_value = 0; // No reading yet

            sensors[i].previous_value = -1.0f; // Marks "first reading" for the filter

           // sensors[i].measurement_index = 0;

            sensors[i].enabled = 1; // Slot in use
/*
            for (uint8_t k = 0; k < AVERAGE_COUNT; k++)
            {
                sensors[i].measurements[k] = 0;
            }
*/
            gpio_a_analog_input_init(pin); // PA<pin> analog input (no effect for pin > 7)

            for (uint8_t i = 0; i < 5; i++) // Note: shadows the outer i; the outer i is returned below
            {
                adc1_read(pin); // Dummy conversions, results discarded
            }


            return i; // Index of the registered slot
        }
    }

    return -1; // No free slot
}

/* One reading, scaled to percent and smoothed (see analog_sensors_manager.h). */
int16_t analog_sensor_read_average(uint8_t sensor_id)
{
    if (sensor_id >= SENSOR_COUNT)
    {
        return -1; // Invalid index (also catches -1 from a failed add, passed as 255)
    }

    analog_sensor_t *sensor = &sensors[sensor_id];

    if (!sensor->enabled)
    {
        return -1; // Slot not registered
    }

    if (sensor->cal_dry == sensor->cal_wet)
    {
        return -1; // Calibration would divide by zero
    }

    uint16_t raw =
        adc1_read(sensor->pin); // One conversion (adc1_last_error is not checked)

    float percent = 0.0f;
    if (sensor->cal_wet != sensor->cal_dry) // Always true here (checked above)
    {
        percent = ((float)raw - (float)sensor->cal_dry) /
                  ((float)sensor->cal_wet - (float)sensor->cal_dry) * 100.0f; // Two-point linear scale
    }

    if (percent > 100.0f) percent = 100.0f; // Clamp above wet point


    if (percent < 0.0f) percent = 0.0f; // Clamp below dry point

    float smooth;

    if (sensor->previous_value < 0.0f) smooth = percent; // First reading: no history to smooth with

    else
    {
        smooth = 0.2f * sensor->previous_value + 0.8f * percent; // Exponential smoothing, 80 % weight on the new value
    }

    sensor->previous_value = smooth; // Filter state for the next call

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
  return sensor->current_value = (uint16_t)smooth; // Store and return the whole-percent value
}