#include "analog_sensors.h"
#include "drivers/acd1/acd1.h"
#include "drivers/gpio/gpio_hw.h"

void analog_sensors_init(void)
{
    adc1_init();
}

void analog_sensor_init(uint8_t sensor_gpio)
{
    static uint8_t initialized = 0;

    if (initialized)
    {
        // already initialized, but ensure pin configured (harmless if called repeatedly)
        gpio_a_analog_input_init(sensor_gpio);
        return;
    }
    // Configure the GPIO pin as analog input
    gpio_a_analog_input_init(sensor_gpio);

    // Do a few dummy reads to let ADC sample/settle
    for (int i = 0; i < 3; ++i)
    {
        (void)adc1_read(sensor_gpio);
        // If you have a millisecond delay helper, add a small delay here, e.g. delay_ms(5);
    }

    initialized = 1;
}
