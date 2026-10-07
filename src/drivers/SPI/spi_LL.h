/**
 * @file  spi_LL.h
 * @brief SPI1 master driver for the SX1276, built on the STM32 LL (Low-Layer) library.
 * This file provides:
 * 1. spi_error_t and the global spi_error with the last SPI failure.
 * 2. spi_init() - SPI1 setup: master, mode 0, 8-bit, MSB first, software NSS, 125 kHz.
 * 3. spi_transfer() - one full-duplex byte exchange with bounded waits.
 * 4. spi_check_hw() - sanity check of the SPI1 configuration.
 * Pins (PA5 SCK, PA6 MISO, PA7 MOSI) and NSS (PB12, driven as GPIO) are set up in gpio_hw.c.
 */

#ifndef SPI_LL_H
#define SPI_LL_H

#include <stdint.h>

 /** @brief SPI failure codes stored in spi_error. */
 typedef enum {
     spi_error_none = 0,      // No error recorded
     spi_error_init,          // SPE not set after spi_init()
     spi_error_tx,            // Timeout waiting for TXE
     spi_error_bsy,           // Timeout waiting for BSY to clear
     spi_error_rx,            // Timeout waiting for RXNE
 } spi_error_t;

 extern volatile spi_error_t spi_error; // Last SPI failure; set on error, never cleared by the driver
/**
 * @brief Initializes SPI1 as master for the SX1276.
 * This function performs the following steps:
 * 1. Enables the SPI1 clock and disables SPI1 during configuration.
 * 2. Master, full duplex, 8-bit frames.
 * 3. CPOL = 0, CPHA = 0 (mode 0: sample on the first, rising edge) - required by the SX1276.
 * 4. Software NSS (SSM = 1, SSI = 1); the chip select is driven by nss_low()/nss_high().
 * 5. Baud-rate prescaler /64: 8 MHz / 64 = 125 kHz SCK.
 * 6. MSB first, then enables SPI1 and reads DR and SR to discard stale state.
 * Sets spi_error = spi_error_init if SPE did not become 1.
 */
void spi_init(void);

/**
 * @brief Exchanges one byte over SPI1 (full duplex).
 * This function performs the following steps:
 * 1. Waits for TXE, writes the byte to DR.
 * 2. Waits for BSY to clear (the byte has been clocked out).
 * 3. Waits for RXNE and returns the received byte.
 * Each wait is limited to 100 ms; on timeout spi_error is set.
 * @param data Byte to send.
 * @return The byte received during the transfer, or 0 on timeout
 *         (spi_error = spi_error_tx / spi_error_bsy / spi_error_rx).
 */
uint8_t spi_transfer(uint8_t data);

/**
 * @brief Checks that SPI1 is configured and idle.
 * This function checks, in order: SPE set, master mode, software NSS (SSM),
 * internal NSS high (SSI), TXE set, BSY clear.
 * @return 0: all checks passed;
 *         1: SPI not enabled;
 *         2: not in master mode;
 *         3: NSS not in software mode;
 *         4: NSS internal signal (SSI) not set;
 *         5: transmit buffer not empty;
 *         6: SPI is busy.
 */
uint8_t spi_check_hw(void);

#endif