/**
 * @file  spi.h
 * @brief Protocol-level wrapper over the SPI1 driver (spi_LL.h), used by the RadioLib HAL.
 * This file provides:
 * 1. spi_start() - SPI1 initialization.
 * 2. spi_check() - configuration sanity check.
 * 3. spi_send() - one full-duplex byte exchange.
 */

#ifndef SPI_H
#define SPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

 /**
  * @brief Initializes SPI1 (master, mode 0, 8-bit, MSB first, 125 kHz) via spi_init().
  */
void spi_start(void);

/**
 * @brief Checks that SPI1 is configured and idle (see spi_check_hw()).
 * @return 0 if all checks passed; 1..6 identifying the first failed check:
 *         1 not enabled, 2 not master, 3 NSS not software, 4 SSI not set,
 *         5 TXE not set, 6 BSY set.
 */
uint8_t spi_check(void);

/**
 * @brief Sends one byte over SPI1 and returns the byte received at the same time.
 * @param data Byte to send.
 * @return Received byte, or 0 on timeout (spi_error is then set).
 */
uint8_t spi_send(uint8_t data);

#ifdef __cplusplus
}
#endif

#endif // SPI_H