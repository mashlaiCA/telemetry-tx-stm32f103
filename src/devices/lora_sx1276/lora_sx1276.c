#include "lora_sx1276.h"
#include "drivers/gpio/gpio_hw.h"

#include <stdint.h>


void lora_init(void)
{
    lora_ctrl_gpio_init(); // Initialize the GPIO pins for controlling the LoRa SX1276 module, including the RST and DIO0 pins.
}

void lora_rst_high(void)
{
rst_high(); // Set the RST pin high to complete the reset process of the LoRa SX1276 module.
}

void lora_rst_low(void)
{
rst_low(); // Set the RST pin low to reset the LoRa SX1276 module.
}

void lora_nss_low(void)
{
    nss_low(); // Set the NSS pin low to select the LoRa SX1276 module for SPI communication.
}
void lora_nss_high(void)
{
    nss_high(); // Set the NSS pin high to deselect the LoRa SX1276 module.
}

uint8_t lora_dio0_read(void)
{
    return dio0_read(); // Read the state of the DIO0 pin.
}