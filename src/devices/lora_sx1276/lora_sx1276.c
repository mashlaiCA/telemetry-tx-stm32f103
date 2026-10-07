#include "lora_sx1276.h"
#include "drivers/gpio/gpio_hw.h"

#include <stdint.h>


/* Configures the SX1276 control pins. */
void lora_init(void)
{
    lora_ctrl_gpio_init(); // PB5 RST output, PB1 DIO0 input with pull-down
}

/* Releases the SX1276 reset. */
void lora_rst_high(void)
{
rst_high(); // PB5 = 1, then 10 ms for the chip to start
}

/* Asserts the SX1276 reset (active low). */
void lora_rst_low(void)
{
rst_low(); // PB5 = 0, held for 10 ms
}

/* Selects the SX1276 for an SPI transaction. */
void lora_nss_low(void)
{
    nss_low(); // PB12 = 0
}
/* Ends the SPI transaction. */
void lora_nss_high(void)
{
    nss_high(); // PB12 = 1
}

/* Reads the DIO0 interrupt line. */
uint8_t lora_dio0_read(void)
{
    return dio0_read(); // PB1 level
}