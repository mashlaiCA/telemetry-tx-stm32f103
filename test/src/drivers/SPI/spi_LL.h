#ifndef SPI_LL_H
#define SPI_LL_H

#include <stdint.h>

/**
 * @file spi_LL.h
 * @brief Low-level SPI driver header for STM32F103C8T6 using LL library.
 * This header defines the interface for initializing the SPI peripheral and performing data transfers.
 * The implementation is designed for the STM32F103C8T6 microcontroller, 
 * uses the STM32 LL (Low-Layer) library for direct hardware access.
 */

 typedef enum {
     spi_error_none = 0,      
     spi_error_init,
     spi_error_tx,
     spi_error_bsy,
     spi_error_rx,
 } spi_error_t;

 extern volatile spi_error_t spi_error;
/**
 * @brief Initializes the SPI peripheral (SPI1) in master mode with specific settings.
 * This function configures SPI1 with the following settings:
 * - Master mode
 * - Full duplex communication
 * - 8-bit data width
 * - Clock polarity low
 * - Clock phase first edge
 * - Software NSS management
 * - Baud rate prescaler of 16 (for 8 MHz APB2 clock, this gives a 0.5 MHz SPI clock)
 * - MSB first transmission
 * - Enables the SPI1 peripheral after configuration.
 */
void spi_init(void);

/**
 * @brief Transfers a single byte of data over SPI and receives the response.
 * This function sends a byte of data through SPI1 and waits for the response from the slave device.
 * It first waits until the transmit buffer is empty, then sends the data byte.
 * After sending, it waits until the receive buffer is not empty, indicating that a response has been received,
 * and returns the received byte.
 *
 * @param data The byte of data to be sent over SPI.
 * @return The byte of data received from the slave device in response to the transmitted byte.
 */
uint8_t spi_transfer(uint8_t data);

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
    *         0: All checks passed, SPI is ready for communication
    *         1: SPI not enabled
    *         2: Not in master mode
    *         3: NSS not in software mode
    *         4: NSS internal signal not set
    *         5: Transmit buffer not empty
    *         6: SPI is busy
*/
uint8_t spi_check_hw(void);

#endif