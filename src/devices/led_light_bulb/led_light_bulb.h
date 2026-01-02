#ifndef LED_LIGHT_BULB_H
#define LED_LIGHT_BULB_H

#include <stdint.h>

void led_light_init(uint8_t led_gpio);

void led_ON(uint8_t led_gpio);

void led_OFF(uint8_t led_gpio);

#endif
