#include "stm32f103xb.h"
#include "i2c_hw.h"
//#include "../tim/tim2_hw.h"
#include "../timeout_hw/timeout_hw.h"

uint8_t APB1_MHZ = 8; //8MHz

timeout_t i2c_timeout; // Timeout structure for I2C operations
uint8_t i2c_timeout_ms = 5; // Timeout duration in milliseconds

I2C_Status_t i2c_status_error = i2c_ok; // Global variable to hold the last I2C error status    


I2C_Status_t I2C1_Init(void) // Initialize I2C1 peripheral
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // Enable I2C1 clock

    if (I2C1->SR2 & I2C_SR2_BUSY) // Check if I2C bus is busy
    {
        i2c_status_error = i2c_busy; // Set error status
        return i2c_busy; // Return busy status
    }
    I2C1->CR2 = 0; // Reset CR2 register
    I2C1->CR2 = APB1_MHZ; // Set peripheral clock frequency

    I2C1->CCR = 40; // Set clock control register for 100kHz standard mode
    I2C1->TRISE = APB1_MHZ + 1; // Set rise time register

    I2C1->CR1 |= I2C_CR1_PE; // Enable I2C1 peripheral

    return i2c_ok; // Return success status
}

I2C_Status_t I2C_Write(uint8_t addr, uint8_t *data, uint8_t len) // Write data to I2C device
{

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for start bit
    I2C1->CR1 |= I2C_CR1_START; // Generate start condition
    while (!(I2C1->SR1 & I2C_SR1_SB)) // Wait for start bit flag
    {
        if (timeout_has_expired(&i2c_timeout)) // Check for timeout
        {
            i2c_status_error = i2c_timeout_err_sb_write; // Set error status
            return i2c_error; // Return error status
        }
    }

    I2C1->DR = addr << 1; // Send device address with write bit
    while (!(I2C1->SR1 & I2C_SR1_ADDR)) // Wait for address acknowledgment flag
    {
        timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for address acknowledgment
        if (I2C1->SR1 & I2C_SR1_AF) // Check for NACK
        {
            I2C1->SR1 &= ~I2C_SR1_AF; // Clear NACK flag
            I2C1->CR1 |= I2C_CR1_STOP; // Generate stop condition

            i2c_status_error = i2c_nack_addr_write; // Set error status
            return i2c_error; // Return error status
        }
        if (timeout_has_expired(&i2c_timeout)) // Check for timeout
        {
            i2c_status_error = i2c_timeout_err_addr_write; // Set error status
            return i2c_error; // Return error status
        }
    }
    (void)I2C1->SR2; // Read SR2 to clear ADDR flag

    for (uint8_t i = 0; i < len; i++) // Send data bytes
    {
        timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for TXE flag
        while (!(I2C1->SR1 & I2C_SR1_TXE)) // Wait for TXE flag
        {
            if (I2C1->SR1 & I2C_SR1_AF) // Check for NACK
            {
                I2C1->SR1 &= ~I2C_SR1_AF; // Clear NACK flag
                I2C1->CR1 |= I2C_CR1_STOP; // Generate stop condition

                i2c_status_error = i2c_nack_txe; // Set error status
                return i2c_error; // Return error status
            }
            if (timeout_has_expired(&i2c_timeout)) // Check for timeout
            {
                i2c_status_error = i2c_timeout_err_txe; // Set error status
                return i2c_error; // Return error status
            }
        }
        I2C1->DR = data[i]; // Write data to DR
    }

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for BTF flag
    while (!(I2C1->SR1 & I2C_SR1_BTF)) // Wait for BTF flag
    {
        if (I2C1->SR1 & I2C_SR1_AF) // Check for NACK
        {
            I2C1->SR1 &= ~I2C_SR1_AF; // Clear NACK flag
            I2C1->CR1 |= I2C_CR1_STOP; // Generate stop condition

            i2c_status_error = i2c_nack_btf; // Set error status
            return i2c_error; // Return error status
        }
        if (timeout_has_expired(&i2c_timeout)) // Check for timeout
        {
            i2c_status_error = i2c_timeout_err_btf; // Set error status
            return i2c_error; // Return error status
        }
    }

    I2C1->CR1 |= I2C_CR1_STOP; // Generate stop condition

    return i2c_ok; // Return success status
}

I2C_Status_t I2C_Read(uint8_t addr, uint8_t *data, uint8_t len) // Read data from I2C device
{
    I2C1->CR1 |= I2C_CR1_ACK; // Enable ACK for received bytes
    I2C1->CR1 |= I2C_CR1_START; // Generate start condition

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for start bit
    while (!(I2C1->SR1 & I2C_SR1_SB)) // Wait for start bit flag
    {
        if (timeout_has_expired(&i2c_timeout)) // Check for timeout
        {
           i2c_status_error = i2c_timeout_err_sb_read; // Set error status
           return i2c_error; // Return error status
        }
    }
    I2C1->DR = (addr << 1) | 1; // Send device address with read bit

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for address acknowledgment
    while (!(I2C1->SR1 & I2C_SR1_ADDR)) // Wait for address acknowledgment flag
    {
        if (I2C1->SR1 & I2C_SR1_AF) // Check for NACK
        {
            I2C1->SR1 &= ~I2C_SR1_AF; // Clear NACK flag
            I2C1->CR1 |= I2C_CR1_STOP; // Generate stop condition

            i2c_status_error = i2c_nack_addr_read; // Set error status
            return i2c_error; // Return error status
        }

        if (timeout_has_expired(&i2c_timeout)) // Check for timeout 
        {
            i2c_status_error = i2c_timeout_err_addr_read; // Set error status
            return i2c_error; // Return error status
        }
    }
    (void)I2C1->SR2; // Read SR2 to clear ADDR flag

    for (uint8_t i = 0; i < len; i++) // Read data bytes
    {
        timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for RXNE flag
        while (!(I2C1->SR1 & I2C_SR1_RXNE)) // Wait for RXNE flag
        {
            if (timeout_has_expired(&i2c_timeout)) // Check for timeout
            {
                i2c_status_error = i2c_timeout_err_rxne; // Set error status
                return i2c_error; // Return error status
            }
        }
        data[i] = I2C1->DR; // Read data from DR
    }
    I2C1->CR1 |= I2C_CR1_ACK; // Re-enable ACK for future receptions

    return i2c_ok; // Return success status
}