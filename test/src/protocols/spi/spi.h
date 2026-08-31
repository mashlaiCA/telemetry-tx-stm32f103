#ifndef SPI_H
#define SPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
/**
 * @file spi.h
 * @brief High-level SPI communication interface for STM32F103C8T6.
 * This header defines the high-level functions for SPI communication. 
 * Including initialization, hardware checks, and data transfer. 
 * The implementation relies on a low-level SPI driver (spi_LL.h).
 * The high-level functions provide a simplified interface for application code to use SPI communication without needing to manage the low-level details of the SPI peripheral configuration and operation.
 */

 /*
  * @brief Starts the SPI communication by initializing the SPI peripheral.
  */
void spi_start(void);

/*
 * @brief Checks the hardware status of the SPI peripheral to ensure it is properly configured and ready for communication.
 * This function performs several checks on the SPI1 peripheral:
 * - Verifies that SPI is enabled
 * - Checks that SPI is in master mode
 * - Ensures NSS is configured for software management
 * - Confirms that the NSS internal signal is set
 * - Checks that the transmit buffer is empty
 * - Verifies that the SPI is not busy
 *
 * @return A status code indicating the result of the hardware checks:
 */
uint8_t spi_check(void);

/*
 * @brief Sends a byte of data over SPI and returns the received response.
 * This function transmits a byte of data to the SPI slave device and waits for the response.
 * It uses the low-level driver function to perform the SPI transfer.
 *
 * @param data The byte of data to be sent over SPI.
 * @return The byte of data received from the slave device in response to the transmitted byte.
 */
uint8_t spi_send(uint8_t data);

#ifdef __cplusplus
}
#endif

#endif // SPI_H