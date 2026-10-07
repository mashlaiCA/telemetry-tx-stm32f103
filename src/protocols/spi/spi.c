#include "spi.h"
#include "drivers/spi/spi_LL.h"


/* Initializes SPI1 through the driver. */
void spi_start(void)
{
    spi_init(); // Master, mode 0, 8-bit, MSB first, 125 kHz
}

/* Configuration sanity check through the driver. */
uint8_t spi_check(void){
    return spi_check_hw(); // 0 = OK, 1..6 = first failed check
}

/* One byte exchange through the driver. */
uint8_t spi_send(uint8_t data)
{
    return spi_transfer(data); // Received byte, or 0 on timeout
}