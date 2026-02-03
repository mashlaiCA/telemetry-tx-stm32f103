#include "led_light_bulb.h"
#include "drivers/gpio/gpio_hw.h"

void led_light_init(uint8_t led_gpio)
{
   gpio_a_led_output_init(led_gpio);// Initialize GPIOA pin as LED output
}

void led_ON(uint8_t led_gpio)
{
    gpio_a_set(led_gpio); // Turn on LED light bulb
}

void led_OFF(uint8_t led_gpio)
{
    gpio_a_reset(led_gpio); // Turn off LED light bulb
}