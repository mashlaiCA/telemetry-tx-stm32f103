#ifndef SPI_LL_H
#define SPI_LL_H

#include <stdint.h>

void spi_init();
uint8_t spi_transfer(uint8_t data);

#endif