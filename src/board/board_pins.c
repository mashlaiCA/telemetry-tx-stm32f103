#include "board_pins.h"
#include "drivers/gpio/gpio_hw.h"


void pins_init(void)
{
    gpio_A_init(); // Initialize GPIOA peripheral
}