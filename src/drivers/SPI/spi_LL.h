#ifndef SPI_LL_H
#define SPI_LL_H

#include <stdint.h>

/**
 * @file spi_LL.h
 * @brief SPI hardware interface for STM32F103xx microcontroller using Low-Layer (LL) drivers.
 * This module provides functions to initialize the SPI peripheral and perform data transfer operations.
 */

/*
 * @brief Initialize the SPI1 peripheral.
 * This function configures the SPI1 peripheral for communication.
 * This function performs the following steps:
 * 1. Enable the SPI1 clock in the RCC APB2ENR register.
 * 2. Disable SPI1 before configuration.
 * 3. Configure SPI1 settings:
 *   - Set to master mode.
 *   - Set to full duplex mode.
 *   - Set data width to 8 bits.
 *   - Set clock polarity to low.
 *   - Set clock phase to first edge.
 *   - Set NSS mode to software.
 *   - Set baud rate prescaler to 16 (for 8 MHz APB2 clock, this gives 0.5 MHz SPI clock).
 *   - Set to transmit MSB first.
 * 4. Enable SPI1 after configuration.
 */
void spi_init();

/*
 * @brief Transfer a byte of data over SPI.
 * This function sends a byte of data and simultaneously receives a byte of data from the SPI slave device.
 * This function performs the following steps:
 * 1. Wait until the transmit buffer is empty (TXE flag is set).
 * 2. Send the data byte using the SPI data register.
 * 3. Wait until the receive buffer is not empty (RXNE flag is set).
 * 4. Return the received data byte from the SPI data register.
 * @param data The byte of data to be sent over SPI.
 * @return The byte of data received from the SPI slave device.
 */
uint8_t spi_transfer(uint8_t data);

#endif