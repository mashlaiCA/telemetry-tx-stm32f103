#include "spi.h"
#include "drivers/spi/spi_LL.h"


void spi_start(void) 
{
    spi_init(); // Initialize the SPI peripheral using the low-level driver function
}

uint8_t spi_check(void){
    return spi_check_hw(); // Check the hardware status of the SPI peripheral using the low-level driver function and return the result
}

uint8_t spi_send(uint8_t data) 
{
    return spi_transfer(data); // Send a byte of data over SPI and return the received response using the low-level driver function
}