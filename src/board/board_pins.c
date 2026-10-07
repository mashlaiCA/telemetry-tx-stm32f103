#include "board_pins.h"
#include "drivers/gpio/gpio_hw.h"

/* Port clocks and the pins used by SPI1 and the SX1276 control lines. */
void pins_init(void)
{
    gpio_A_init(); // GPIOA clock
    gpio_B_init(); // GPIOB clock
    gpio_SPI_init();       // PA5/PA6/PA7 SPI1, PB12 NSS
    lora_ctrl_gpio_init(); // PB5 RST, PB1 DIO0
}

/* Configures one GPIOA pin as a probe drive output. */
void polarity_init(uint8_t pin_1){

    gpio_A_polarity_init(pin_1); // PA<pin_1> -> push-pull output, 2 MHz
}