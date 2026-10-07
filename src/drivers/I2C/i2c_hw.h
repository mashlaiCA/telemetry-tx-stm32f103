/**
 * @file  i2c_hw.h
 * @brief Register-level polling driver for I2C1 (PB6 = SCL, PB7 = SDA), 100 kHz standard mode.
 * This file provides:
 * 1. The I2C_Status_t result codes.
 * 2. The global i2c_status_error holding the detailed cause of the last failure.
 * 3. I2C1_Init(), I2C_Write() and I2C_Read() (master mode, 7-bit addressing, blocking with timeouts).
 * 4. i2c_bus_recover() - software recovery of a bus held low by a slave.
 * 5. i2c_bus_is_free() - BUSY flag check.
 * The bus is shared by SHT35 (0x44) and DS3231 (0x68), so every transfer starts by
 * clearing leftovers of the previous one and every failure leaves the bus released.
 */

#ifndef I2C_HW_H
#define I2C_HW_H

#include "stdint.h"

/**
 * @brief I2C result and error codes.
 * I2C_Write()/I2C_Read() return only i2c_ok, i2c_busy or i2c_error; the
 * detailed cause of an i2c_error is stored in i2c_status_error.
 */
typedef enum
{
    i2c_ok = 0,                 // Operation completed successfully
    i2c_busy,                   // Bus could not be acquired (BUSY stuck and recovery failed)
    i2c_timeout_err_sb_write,   // Write: timeout waiting for SB (START not generated)
    i2c_nack_addr_write,        // Write: address not acknowledged (AF)
    i2c_timeout_err_addr_write, // Write: timeout waiting for ADDR
    i2c_nack_txe,               // Write: data byte not acknowledged (AF) while waiting for TXE
    i2c_timeout_err_txe,        // Write: timeout waiting for TXE
    i2c_nack_btf,               // Write: last byte not acknowledged (AF) while waiting for BTF
    i2c_timeout_err_btf,        // Write: timeout waiting for BTF
    i2c_timeout_err_sb_read,    // Read: timeout waiting for SB
    i2c_nack_addr_read,         // Read: address not acknowledged (AF)
    i2c_timeout_err_addr_read,  // Read: timeout waiting for ADDR
    i2c_timeout_err_rxne,       // Read: timeout waiting for RXNE
    i2c_timeout_err_stop,       // Write/read: timeout waiting for BUSY to clear after STOP (bus recovered)
    i2c_error,                  // Generic failure returned by I2C_Write()/I2C_Read(); cause in i2c_status_error

    // Codes describing the bus itself rather than one device, so diagnostics
    // can tell "bus hung" from a plain NACK on the shared SHT35/DS3231 bus.
    i2c_timeout_err_btf_read,   // Read: timeout waiting for BTF (2-byte or N>2-byte reception)
    i2c_bus_stuck,              // SDA/SCL still low after software recovery (hardware fault / missing pull-ups)
    i2c_bus_recovered,          // Bus was stuck and has been released by i2c_bus_recover()

    i2c_count // Number of status codes
} I2C_Status_t;

extern I2C_Status_t i2c_status_error; // Detailed status of the last transfer: reset to i2c_ok at the start of each transfer, set on failure

/**
 * @brief Configures and enables the I2C1 peripheral.
 * This function performs the following steps:
 * 1. Enables the I2C1 clock (RCC APB1ENR.I2C1EN).
 * 2. Clears PE (CCR/TRISE may only be written with the peripheral disabled).
 * 3. CR2.FREQ = 8 (APB1 = 8 MHz).
 * 4. CCR = 40: Thigh = Tlow = 40 * 125 ns = 5 us -> 100 kHz standard mode.
 * 5. TRISE = 9: 1000 ns max rise time / 125 ns + 1.
 * 6. Sets PE, then ACK.
 * GPIO (PB6/PB7 alternate function open-drain) must be configured separately.
 * @return Always i2c_ok.
 */
I2C_Status_t I2C1_Init(void);

/**
 * @brief Writes a block of bytes to a slave (START, address+W, data, STOP).
 * This function performs the following steps:
 * 1. Prepares the bus: re-initializes I2C1 if PE is off, waits up to 10 ms for
 *    BUSY to clear (then tries i2c_bus_recover()), clears AF/BERR/ARLO/OVR.
 * 2. Generates START and waits for SB.
 * 3. Sends the address with the write bit and waits for ADDR (AF = NACK).
 * 4. Clears ADDR by reading SR1 then SR2.
 * 5. For each byte waits for TXE (AF = NACK) and writes it to DR.
 * 6. Waits for BTF (last byte shifted out; AF = NACK).
 * 7. Generates STOP and waits for BUSY to clear.
 * Every wait step is limited to 5 ms. On a NACK or timeout in steps 2-6 the
 * transfer is aborted with STOP (and bus recovery if the bus stays busy).
 * @param addr 7-bit slave address (not shifted).
 * @param data Bytes to send.
 * @param len  Number of bytes, 1..255 (len == 0 is not handled specially, unlike I2C_Read()).
 * @return i2c_ok on success;
 *         i2c_busy if the bus could not be acquired (i2c_status_error = i2c_bus_stuck);
 *         i2c_error on NACK or timeout (cause in i2c_status_error: i2c_timeout_err_sb_write,
 *         i2c_nack_addr_write, i2c_timeout_err_addr_write, i2c_nack_txe, i2c_timeout_err_txe,
 *         i2c_nack_btf, i2c_timeout_err_btf, i2c_timeout_err_stop).
 */
I2C_Status_t I2C_Write(uint8_t addr, uint8_t *data, uint8_t len);

/**
 * @brief Reads a block of bytes from a slave (START, address+R, data, NACK, STOP).
 * This function performs the following steps:
 * 1. Returns i2c_ok immediately if len == 0.
 * 2. Prepares the bus as in I2C_Write(), clears POS, sets ACK, generates START, waits for SB.
 * 3. Sends the address with the read bit and waits for ADDR (AF = NACK).
 * 4. Receives the data using the RM0008 (26.3.3) closing sequence for the given length:
 *    - len == 1: ACK=0, clear ADDR, STOP, then wait RXNE and read;
 *    - len == 2: ACK=0 + POS=1, clear ADDR, wait BTF, STOP, read both bytes;
 *    - len > 2:  RXNE reads until 3 bytes remain, wait BTF, ACK=0, read N-2,
 *                wait BTF, STOP, read N-1 and N.
 *    The register sequences that must not be delayed run with interrupts disabled.
 * 5. Waits for BUSY to clear, then re-enables ACK.
 * Every wait step is limited to 5 ms.
 * @param addr 7-bit slave address (not shifted).
 * @param data Buffer for the received bytes (at least len bytes).
 * @param len  Number of bytes to read, 0..255.
 * @return i2c_ok on success (or len == 0);
 *         i2c_busy if the bus could not be acquired (i2c_status_error = i2c_bus_stuck);
 *         i2c_error on NACK or timeout (cause in i2c_status_error: i2c_timeout_err_sb_read,
 *         i2c_nack_addr_read, i2c_timeout_err_addr_read, i2c_timeout_err_rxne,
 *         i2c_timeout_err_btf_read, i2c_timeout_err_stop).
 */
I2C_Status_t I2C_Read(uint8_t addr, uint8_t *data, uint8_t len);

/**
 * @brief Releases a bus that a slave is holding low and re-initializes I2C1.
 * A slave (DS3231 or SHT35) interrupted in the middle of a byte keeps SDA low,
 * so BUSY never clears and every further transfer times out.
 * This function performs the following steps:
 * 1. Disables I2C1 and switches PB6/PB7 to open-drain GPIO outputs.
 * 2. Clocks SCL up to 9 times until the slave releases SDA.
 * 3. Generates a START followed by a STOP manually.
 * 4. Samples SDA/SCL, returns the pins to the peripheral, pulses SWRST and calls I2C1_Init().
 * Also stores its result in i2c_status_error.
 * @return i2c_bus_recovered if both lines read high after the sequence;
 *         i2c_bus_stuck if SDA or SCL is still low.
 */
I2C_Status_t i2c_bus_recover(void);

/**
 * @brief Reports whether the I2C bus is idle.
 * @return 1 if SR2.BUSY is clear (bus free), 0 otherwise.
 */
uint8_t i2c_bus_is_free(void);

#endif
