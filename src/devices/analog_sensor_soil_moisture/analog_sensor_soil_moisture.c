#include "analog_sensor_soil_moisture.h"
#include "analog_sensors_manager.h"

static int8_t soil_sensor_id = -1; // Slot in the analog sensor manager; -1 = not registered

/* Registers the soil sensor. Channel 8 is PB0, the same ADC input that
   watermark_200ss.c uses (WM_ADC_CH = 8). gpio_a_analog_input_init() ignores
   pin 8; PB0 is switched to analog mode later, by watermark_init(). */
void soil_sensor_init(void)
{
    soil_sensor_id =
        analog_sensor_add(8, 4047, 1350); // ADC channel 8, raw dry = 4047 (0 %), raw wet = 1350 (100 %)
}

/* One smoothed reading of the soil sensor. */
int16_t soil_sensor_read_average(void)
{
    return
        analog_sensor_read_average(
            soil_sensor_id // -1 (not registered) becomes 255 and is rejected by the manager
        );
}