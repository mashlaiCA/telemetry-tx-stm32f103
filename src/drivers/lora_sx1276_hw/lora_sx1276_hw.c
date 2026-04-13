#include "lora_sx1276_hw.h"
#include "../SPI/spi_LL.h"
#include "../gpio/gpio_hw.h"

void sx_write(uint8_t addr, uint8_t data) {
   nss_low(); // Pull NSS low to select the SX1276
   spi_transfer(addr | 0x80); // Send address with write bit (MSB = 1)
   spi_transfer(data); // Send data byte for writing to the register
   nss_high(); // Pull NSS high to deselect the SX1276
}

uint8_t sx_read(uint8_t addr){
   uint8_t value; // Variable to store the received value
   nss_low(); // Pull NSS low to select the SX1276
   spi_transfer(addr & 0x7F); // Send address with write bit (MSB = 1)
   value = spi_transfer(0x00); // Send data byte and save the received byte
   nss_high(); // Pull NSS high to deselect the SX1276

   return value; // Return the value read from the register
}