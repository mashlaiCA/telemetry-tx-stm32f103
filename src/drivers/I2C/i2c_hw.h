#ifndef I2C_HW_H
#define I2C_HW_H

#include "stdint.h"

/**
 * @file i2c_hw.h
 * @brief I2C hardware interface for STM32F103xx microcontroller.
 * This module provides functions to initialize the I2C peripheral and perform read/write operations.
 */


/**
 * @brief I2C status enumeration.
 * This enumeration defines various status codes for I2C operations.
 * Values:
 * - i2c_ok: Operation completed successfully.
 * - i2c_busy: I2C bus is busy.
 * - i2c_timeout_err_sb_write: Timeout error while waiting for start bit during write
 * - i2c_nack_addr_write: NACK received after sending address during write.
 * - i2c_timeout_err_addr_write: Timeout error while waiting for address acknowledgment during write.
 * - i2c_nack_txe: NACK received while waiting for transmit buffer empty during write.
 * - i2c_timeout_err_txe: Timeout error while waiting for transmit buffer empty during write.
 * - i2c_nack_bft: NACK received while waiting for byte transfer finished during write.
 * - i2c_timeout_err_btf: Timeout error while waiting for byte transfer finished during write.
 * - i2c_timeout_err_sb_read: Timeout error while waiting for start bit during read.
 * - i2c_nack_addr_read: NACK received after sending address during read.
 * - i2c_timeout_err_addr_read: Timeout error while waiting for address acknowledgment during read.
 * - i2c_timeout_err_rxne: Timeout error while waiting for receive buffer not empty during read.
 * - i2c_error: General I2C error.
 *
 * @typedef I2C_Status_t (I2C status type).
 */
typedef enum
{
    i2c_ok = 0,
    i2c_busy,
    i2c_timeout_err_sb_write,
    i2c_nack_addr_write,
    i2c_timeout_err_addr_write,
    i2c_nack_txe,
    i2c_timeout_err_txe,
    i2c_nack_btf,
    i2c_timeout_err_btf,
    i2c_timeout_err_sb_read,
    i2c_nack_addr_read,
    i2c_timeout_err_addr_read,
    i2c_timeout_err_rxne,
    i2c_error,

    i2c_count
} I2C_Status_t;

/**
 * @brief Global variable to hold the last I2C error status.
 * This variable is updated whenever an I2C operation fails.
 */
extern I2C_Status_t i2c_status_error;


/**
 * @brief Initialize the I2C1 peripheral.
 * This function configures the I2C1 peripheral for communication.
 * This function performs the following steps:
 * 1. Enable the I2C1 clock in the RCC APB1ENR register.
 * 2. Check if the I2C1 bus is busy, return i2c_busy if i2c is busy.
 * 3. Configure the I2C1 timing settings:
 *   - Set the peripheral clock frequency in CR2 register.
 *   - Set the clock control register (CCR) for standard mode (100 kHz).
 *   - Set the rise time register (TRISE).
 * 4. Enable the I2C1 peripheral by setting the PE bit in the CR1 register.
 * @return I2C_Status_t Status of the initialization operation.
 */
I2C_Status_t I2C1_Init(void);

/**
 * @brief Write data to an I2C device.
 * This function sends data to a specified I2C device address.
 * This function performs the following steps:
 * 1. Timeout start for checking start bit.
 * 2. Generate start condition.
 * 3. Wait for start bit (SB) flag:
 *  - If timeout occurs, set error status i2c_timeout_err_sb_write and return i2c_error.
 *  4. Send the device address with write bit.
 *  5. Timeout start for checking address acknowledgment.
 *  6. Wait for address acknowledgment (ADDR) flag:
 *   - If NACK is received, set error status i2c_status_error = i2c_nack_addr_write; and return i2c_error.
 *   - If timeout occurs, set error status i2c_status_error = i2c_timeout_err_addr_write and return i2c_error.
 *  7. SR2 read to clear ADDR flag.
 *  8. Send data bytes in a data register:
 *   - Timeout start for checking transmit buffer empty (TXE) flag.
 *   - Wait for TXE flag:
 *     - If NACK is received:
 *           - Clear NACK flag.
 *           - Generate stop condition. 
 *           - set error status i2c_status_error = i2c_nack_txe; and return i2c_error.
 *    - If timeout occurs, set error status i2c_status_error = i2c_timeout_err_txe and return i2c_error.
 *  - Write data to DR.
 * 9. BTF wait (byte transfer finished):
 *  - Timeout start for checking byte transfer finished (BTF) flag.
 *  - Wait for BTF flag:
 *    - If NACK is received:
 *       - Clear NACK flag.
 *       - Generate stop condition.
 *       - set error status i2c_status_error = i2c_nack_btf; and return i2c_error.
 *   - If timeout occurs, set error status i2c_status_error = i2c_timeout_err_btf and return i2c_error.
 * 10. Generate stop condition.
 * 11. Return i2c_ok on successful completion.
 * @param addr The 7-bit I2C device address.
 * @param data Pointer to the data buffer to be sent.
 * @param len Length of the data to be sent in bytes.
 * @return I2C_Status_t Status of the write operation.
 */
I2C_Status_t I2C_Write(uint8_t addr, uint8_t *data, uint8_t len);


/**
 * @brief Read data from an I2C device.
 * This function reads data from a specified I2C device address.
 * This function performs the following steps:
 * 1. Enable ACK for received bytes.
 * 2. Generate start condition.
 * 3. Wait for start bit (SB) flag:
 *   - If timeout occurs, set error status i2c_timeout_err_sb_read and return i2c_error.
 * 4. Send the device address with read bit.
 * 5. Timeout start for checking address acknowledgment.
 * 6. Wait for address acknowledgment (ADDR) flag:
 *   - If NACK is received:
 *       - clear NACK flag
 *       - generate stop condition
 *       - set error status i2c_nack_addr_read and return i2c_error.
 *   - If timeout occurs, set error status i2c_timeout_err_addr_read and return i2c_error.
 * 7. SR2 read to clear ADDR flag.
 * 8. Read data bytes in a loop:
 *   - For each byte except the last one:
 *     - Wait for receive buffer not empty (RXNE) flag:
 *       - If timeout occurs, set error status i2c_timeout_err_rxne and return i2c_error.
 *     - Read data from DR.
 *   - For the last byte:
 *     - Disable ACK.
 *     - Generate stop condition.
 *     - Wait for RXNE flag:
 *       - If timeout occurs, set error status i2c_timeout_err_rxne and return i2c_error.
 *     - Read data from DR.
 * 9. Re-enable ACK for future receptions.
 * 10. Return i2c_ok on successful completion.
 * @param addr The 7-bit I2C device address.
 * @param data Pointer to the buffer to store received data.
 * @param len Length of the data to be read in bytes.
 * @return I2C_Status_t Status of the read operation.
 */
I2C_Status_t I2C_Read(uint8_t addr, uint8_t *data, uint8_t len);

#endif
