#include "board_pins.h"
#include "drivers/gpio/gpio_hw.h"

void pins_init(void)
{
    gpio_A_init(); // Initialize GPIOA peripheral
    gpio_B_init(); // Initialize GPIOB peripheral
    gpio_SPI_init();
    lora_ctrl_gpio_init();
}

void polarity_init(uint8_t pin_1){

    gpio_A_polarity_init(pin_1);
}