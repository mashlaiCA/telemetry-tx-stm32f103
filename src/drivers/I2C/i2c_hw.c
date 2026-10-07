#include "stm32f103xb.h"
#include "i2c_hw.h"
#include "../timeout_hw/timeout_hw.h"
#include "../tim/tim2_hw.h"
uint8_t APB1_MHZ = 8; // PCLK1 in MHz (HSI 8 MHz, APB1 prescaler 1); written to CR2.FREQ and used for TRISE

timeout_t i2c_timeout;      // Timeout of the current wait step
uint8_t i2c_timeout_ms = 5; // Limit for each wait step (SB, ADDR, TXE, BTF, RXNE, BUSY), ms; one byte at 100 kHz takes ~90 us

I2C_Status_t i2c_status_error = i2c_ok; // Detailed status of the last transfer (see i2c_hw.h)

/* PB6 = SCL, PB7 = SDA. Named because the same pins are also driven
   manually by the software bus recovery (i2c_bus_recover). */
#define I2C_SCL_PIN 6
#define I2C_SDA_PIN 7

/* How long to wait for the bus to become free (BUSY = 0) before a transfer or
   after an abort STOP, ms. Longer than a single step timeout: a transfer of the
   other device on the shared bus may still be finishing. */
#define I2C_BUS_FREE_MS 10


/* Clears the error flags left by a previous transfer.
   AF/BERR/ARLO/OVR are rc_w0 bits (cleared by writing 0). Leftovers from a failed
   transfer to one device (e.g. DS3231) would otherwise be seen as an error in
   the next transfer to the other device (SHT35) on the same bus. */
static void i2c_clear_error_flags(void)
{
    I2C1->SR1 &= ~(I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR); // Write 0 to AF, BERR, ARLO, OVR
}

/* Switches PB6/PB7 to general purpose output open-drain 50 MHz
   (CNF = 01, MODE = 11 -> 0x7) so SCL/SDA can be driven manually. */
static void i2c_gpio_to_bitbang(void)
{
    GPIOB->CRL &= ~((0xFu << (I2C_SCL_PIN * 4)) | (0xFu << (I2C_SDA_PIN * 4))); // Clear CNF/MODE of PB6 and PB7
    GPIOB->CRL |= ((0x7u << (I2C_SCL_PIN * 4)) | (0x7u << (I2C_SDA_PIN * 4)));  // 0x7: GP output open-drain, 50 MHz
}

/* Returns PB6/PB7 to I2C1 control: alternate function open-drain 50 MHz
   (CNF = 11, MODE = 11 -> 0xF). Setting all four bits with OR gives 0xF
   regardless of the previous value, so the clearing step is not required. */
static void i2c_gpio_to_af(void)
{
    //GPIOB->CRL &= ~((0xFu << (I2C_SCL_PIN * 4)) | (0xFu << (I2C_SDA_PIN * 4)));
    GPIOB->CRL |= ((0xFu << (I2C_SCL_PIN * 4)) | (0xFu << (I2C_SDA_PIN * 4))); // 0xF: AF open-drain, 50 MHz
}

/* Reports whether the bus is idle (SR2.BUSY). */
uint8_t i2c_bus_is_free(void)
{
    return (I2C1->SR2 & I2C_SR2_BUSY) ? 0u : 1u; // BUSY = 1 while SDA or SCL is low or a transfer is in progress
}

/* Software bus recovery: clocks out a slave stuck mid-byte, issues
   START + STOP, then resets and re-initializes I2C1. */
I2C_Status_t i2c_bus_recover(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN; // GPIOB clock must be on to drive the pins

    I2C1->CR1 &= ~I2C_CR1_PE; // CR1.PE = 0: peripheral releases the lines before bit-banging

    i2c_gpio_to_bitbang(); // PB6/PB7 -> open-drain GPIO outputs
    GPIOB->BSRR = (1u << I2C_SCL_PIN) | (1u << I2C_SDA_PIN); // Release both lines (pulled high externally)
    wm_delay_hw_us(10); // Let the lines settle

    /* Up to 9 clocks: 8 data bits + ACK is the most a slave can still owe,
       enough for any slave to finish the byte it was transmitting and release SDA. */
    for (uint8_t i = 0; i < 9u; i++)
    {
        if (GPIOB->IDR & (1u << I2C_SDA_PIN)) // SDA reads high
        {
            break; // SDA released, the slave let go of the bus
        }

        GPIOB->BRR = (1u << I2C_SCL_PIN); // SCL low
        wm_delay_hw_us(5);                 // 5 us low + 5 us high = 100 kHz clock
        GPIOB->BSRR = (1u << I2C_SCL_PIN); // SCL high
        wm_delay_hw_us(5);
    }

    /* SCL is high here. SDA falling while SCL is high is a START, SDA rising
       while SCL is high is a STOP: the pair resets the slaves' bus logic. */
    GPIOB->BRR = (1u << I2C_SDA_PIN);  // SDA low (START)
    wm_delay_hw_us(5);
    GPIOB->BSRR = (1u << I2C_SCL_PIN); // SCL high (already high after the loop)
    wm_delay_hw_us(5);
    GPIOB->BSRR = (1u << I2C_SDA_PIN); // SDA high (STOP)
    wm_delay_hw_us(10);

    uint8_t sda_high = (GPIOB->IDR & (1u << I2C_SDA_PIN)) ? 1u : 0u; // Sample SDA after the sequence
    uint8_t scl_high = (GPIOB->IDR & (1u << I2C_SCL_PIN)) ? 1u : 0u; // Sample SCL after the sequence

    i2c_gpio_to_af(); // Give the pins back to I2C1

    /* CR1.SWRST resets the peripheral's internal state machine, including a
       BUSY flag left set by the stuck transfer. */
    I2C1->CR1 |= I2C_CR1_SWRST;  // Enter software reset
    wm_delay_hw_us(10);
    I2C1->CR1 &= ~I2C_CR1_SWRST; // Leave software reset (all registers are now at reset values)

    (void)I2C1_Init(); // Reprogram FREQ/CCR/TRISE and enable the peripheral

    if (!sda_high || !scl_high) // A line is still held low
    {
        i2c_status_error = i2c_bus_stuck; // Hardware fault or missing pull-ups
        return i2c_bus_stuck;
    }

    i2c_status_error = i2c_bus_recovered; // Bus is usable again
    return i2c_bus_recovered;
}



/* Terminates a failed transfer: STOP, then wait for the bus to become free.
   If it does not, the bus is recovered; i2c_status_error keeps the original
   cause of the abort instead of the recovery result. */
static void i2c_abort(void)
{
    I2C1->CR1 |= I2C_CR1_STOP; // CR1.STOP: generate STOP after the current byte

    timeout_t abort_timeout;
    timeout_start(&abort_timeout, I2C_BUS_FREE_MS); // Up to 10 ms for the bus to go idle

    while (I2C1->SR2 & I2C_SR2_BUSY) // Wait until SR2.BUSY = 0
    {
        if (timeout_has_expired(&abort_timeout)) // Bus still held
        {
            I2C_Status_t saved = i2c_status_error; // keep the original cause of the abort
            (void)i2c_bus_recover();               // Clock out the stuck slave and re-init I2C1
            i2c_status_error = saved;              // Restore the original cause
            return;
        }
    }

    i2c_clear_error_flags(); // Do not leave AF etc. for the next transfer
}


/* Prepares the bus for a new transfer: resets the shared status, makes sure
   I2C1 is enabled and the bus is idle (recovering it if needed), clears stale
   error flags. */
static I2C_Status_t i2c_begin_transfer(void)
{

    i2c_status_error = i2c_ok; // The status now describes this transfer only

    if (!(I2C1->CR1 & I2C_CR1_PE))
    {
        (void)I2C1_Init(); // Peripheral was left disabled by a previous recovery
    }

    if (I2C1->SR2 & I2C_SR2_BUSY) // Bus not idle (other transfer finishing, or slave holding SDA)
    {
        timeout_t busy_timeout;
        timeout_start(&busy_timeout, I2C_BUS_FREE_MS); // Up to 10 ms for it to become free

        while (I2C1->SR2 & I2C_SR2_BUSY)
        {
            if (timeout_has_expired(&busy_timeout)) // Still busy: treat as a stuck bus
            {
                if (i2c_bus_recover() != i2c_bus_recovered)
                {
                    i2c_status_error = i2c_bus_stuck; // Recovery failed
                    return i2c_busy;                  // Caller must not start the transfer
                }
                break; // Bus recovered: continue with the transfer
            }
        }
    }

    i2c_clear_error_flags(); // Clear AF/BERR/ARLO/OVR left by a previous transfer
    i2c_status_error = i2c_ok; // Discard i2c_bus_recovered set by a successful recovery

    return i2c_ok;
}


/* I2C1 setup for 100 kHz standard mode at PCLK1 = 8 MHz (see i2c_hw.h). */
I2C_Status_t I2C1_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; // Enable I2C1 clock



    I2C1->CR1 &= ~I2C_CR1_PE; // CCR/TRISE/FREQ can only be written with PE = 0

    I2C1->CR2 = 0;        // Reset CR2 (interrupts/DMA disabled)
    I2C1->CR2 = APB1_MHZ; // CR2.FREQ = 8: PCLK1 in MHz

    I2C1->CCR = 40;             // Standard mode: Thigh = Tlow = 40 * 125 ns = 5 us -> 100 kHz
    I2C1->TRISE = APB1_MHZ + 1; // TRISE = 1000 ns / 125 ns + 1 = 9 (standard-mode max rise time)

    I2C1->CR1 |= I2C_CR1_PE;  // Enable I2C1
    I2C1->CR1 |= I2C_CR1_ACK; // CR1.ACK: acknowledge received bytes (hardware clears ACK while PE = 0, so it is set after PE)

    return i2c_ok; // Initialization cannot fail
}

/* Master transmitter: START, address+W, len data bytes, STOP (RM0008 26.3.3). */
I2C_Status_t I2C_Write(uint8_t addr, uint8_t *data, uint8_t len)
{

    if (i2c_begin_transfer() != i2c_ok) // Bus could not be acquired
    {
        return i2c_busy;
    }

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for start bit
    I2C1->CR1 |= I2C_CR1_START;                  // CR1.START: generate START condition
    while (!(I2C1->SR1 & I2C_SR1_SB))            // Wait for SR1.SB (START sent, master mode entered)
    {
        if (timeout_has_expired(&i2c_timeout)) // START not generated in time
        {

            i2c_status_error = i2c_timeout_err_sb_write; // Record the cause
            i2c_abort();                                 // Release the bus
            return i2c_error;
        }
    }

    I2C1->DR = addr << 1; // Address + write bit (R/W = 0); writing DR after reading SR1 clears SB

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for address acknowledgment

    while (!(I2C1->SR1 & I2C_SR1_ADDR)) // Wait for SR1.ADDR (address acknowledged)
    {

        if (I2C1->SR1 & I2C_SR1_AF) // SR1.AF: address NACKed (device absent or busy)
        {
            i2c_status_error = i2c_nack_addr_write; // Record the cause
            i2c_abort();                            // STOP and release the bus
            return i2c_error;
        }
        if (timeout_has_expired(&i2c_timeout)) // No ACK and no NACK in time
        {
            i2c_status_error = i2c_timeout_err_addr_write; // Record the cause
            i2c_abort();
            return i2c_error;
        }
    }
    (void)I2C1->SR1; // ADDR is cleared by reading SR1, then SR2 - only in this order (RM0008 26.6.6)
    (void)I2C1->SR2; // Read SR2 to complete clearing ADDR

    for (uint8_t i = 0; i < len; i++) // Send data bytes
    {
        timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for TXE flag
        while (!(I2C1->SR1 & I2C_SR1_TXE))           // Wait for SR1.TXE (DR empty)
        {
            if (I2C1->SR1 & I2C_SR1_AF) // Previous byte NACKed by the slave
            {
                i2c_status_error = i2c_nack_txe; // Record the cause
                i2c_abort();                     // STOP, wait for BUSY, clear AF
                return i2c_error;
            }
            if (timeout_has_expired(&i2c_timeout)) // DR did not empty in time
            {
                i2c_status_error = i2c_timeout_err_txe; // Record the cause
                i2c_abort();
                return i2c_error;
            }
        }
        I2C1->DR = data[i]; // Queue the next byte
    }

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for BTF flag
    while (!(I2C1->SR1 & I2C_SR1_BTF))           // Wait for SR1.BTF: last byte fully shifted out
    {
        if (I2C1->SR1 & I2C_SR1_AF) // Last byte NACKed by the slave
        {
            i2c_status_error = i2c_nack_btf; // Record the cause
            i2c_abort();                     // STOP, wait for BUSY, clear AF
            return i2c_error;
        }
        if (timeout_has_expired(&i2c_timeout)) // Transfer did not finish in time
        {
            i2c_status_error = i2c_timeout_err_btf; // Record the cause
            i2c_abort();
            return i2c_error;
        }
    }

    I2C1->CR1 |= I2C_CR1_STOP; // CR1.STOP: generate STOP condition

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for bus idle
    while (I2C1->SR2 & I2C_SR2_BUSY)             // Wait until STOP is on the bus (BUSY = 0)
    {
        if (timeout_has_expired(&i2c_timeout)) // STOP did not complete
        {
            i2c_status_error = i2c_timeout_err_stop; // Record the cause
            /* The bus is held low. Recover it here, otherwise the next transfer
               (possibly to the other device) cannot get through. */
            (void)i2c_bus_recover();
            i2c_status_error = i2c_timeout_err_stop; // Recovery overwrote the status: restore the cause
            return i2c_error;
        }
    }

    return i2c_ok; // All bytes acknowledged and bus released
}

/* Master receiver: START, address+R, len bytes, NACK on the last byte, STOP.
   The closing sequence depends on len (RM0008 26.3.3, "Closing the communication"). */
I2C_Status_t I2C_Read(uint8_t addr, uint8_t *data, uint8_t len)
{
    if (len == 0)
    {
        return i2c_ok; // Nothing to read: do not generate a START without a transfer
    }

    if (i2c_begin_transfer() != i2c_ok) // Bus could not be acquired
    {
        return i2c_busy;
    }

    I2C1->CR1 &= ~I2C_CR1_POS;  // CR1.POS may be left over from an aborted 2-byte read
    I2C1->CR1 |= I2C_CR1_ACK;   // CR1.ACK: acknowledge received bytes
    I2C1->CR1 |= I2C_CR1_START; // CR1.START: generate START condition

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for start bit
    while (!(I2C1->SR1 & I2C_SR1_SB))            // Wait for SR1.SB
    {
        if (timeout_has_expired(&i2c_timeout)) // START not generated in time
        {
            i2c_status_error = i2c_timeout_err_sb_read; // Record the cause
            i2c_abort();                                // STOP, wait for BUSY (recover if needed)
            return i2c_error;
        }
    }
    I2C1->DR = (addr << 1) | 1; // Address + read bit (R/W = 1)

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for address acknowledgment
    while (!(I2C1->SR1 & I2C_SR1_ADDR))          // Wait for SR1.ADDR
    {
        if (I2C1->SR1 & I2C_SR1_AF) // SR1.AF: address NACKed
        {
            i2c_status_error = i2c_nack_addr_read; // Record the cause
            i2c_abort();                           // STOP, wait for BUSY, clear AF
            return i2c_error;
        }

        if (timeout_has_expired(&i2c_timeout)) // No ACK and no NACK in time
        {
            i2c_status_error = i2c_timeout_err_addr_read; // Record the cause
            i2c_abort();
            return i2c_error;
        }
    }

    if (len == 1u)
    {
        /* Single byte: ACK must be cleared before ADDR is cleared and STOP must be
           set right after, before the byte finishes. An interrupt between these
           writes could let the byte be ACKed and the slave keep transmitting, so
           the sequence runs with interrupts disabled (also recommended by the
           STM32F10xx errata for I2C master reception). PRIMASK is saved and
           restored so a caller's critical section is not broken. */
        uint32_t primask = __get_PRIMASK();
        __disable_irq();

        I2C1->CR1 &= ~I2C_CR1_ACK; // NACK the only byte
        (void)I2C1->SR1;           // Clear ADDR: read SR1 ...
        (void)I2C1->SR2;           // ... then SR2
        I2C1->CR1 |= I2C_CR1_STOP; // STOP must be armed before the byte finishes

        __set_PRIMASK(primask); // Restore the previous interrupt state

        timeout_start(&i2c_timeout, i2c_timeout_ms);
        while (!(I2C1->SR1 & I2C_SR1_RXNE)) // Wait for the byte in DR
        {
            if (timeout_has_expired(&i2c_timeout)) // Byte not received in time
            {
                i2c_status_error = i2c_timeout_err_rxne;
                i2c_abort();
                return i2c_error;
            }
        }
        data[0] = (uint8_t)I2C1->DR; // Read the byte (clears RXNE)
    }
    else if (len == 2u)
    {
        /* Two bytes: with POS = 1 and ACK = 0 the NACK applies to the SECOND byte.
           Both must be set before ADDR is cleared, hence the critical section. */
        uint32_t primask = __get_PRIMASK();
        __disable_irq();

        I2C1->CR1 &= ~I2C_CR1_ACK; // NACK ...
        I2C1->CR1 |= I2C_CR1_POS;  // ... but only for the *next* byte (the second one)
        (void)I2C1->SR1;           // Clear ADDR: read SR1 ...
        (void)I2C1->SR2;           // ... then SR2

        __set_PRIMASK(primask); // Restore the previous interrupt state

        timeout_start(&i2c_timeout, i2c_timeout_ms);
        while (!(I2C1->SR1 & I2C_SR1_BTF)) // Byte 1 in DR, byte 2 in the shift register
        {
            if (timeout_has_expired(&i2c_timeout)) // Bytes not received in time
            {
                i2c_status_error = i2c_timeout_err_btf_read;
                I2C1->CR1 &= ~I2C_CR1_POS; // Do not leave POS set for the next transfer
                i2c_abort();
                return i2c_error;
            }
        }

        /* STOP must be set before the bytes are read out of DR; the critical
           section keeps the STOP + two reads together. */
        primask = __get_PRIMASK();
        __disable_irq();

        I2C1->CR1 |= I2C_CR1_STOP;   // Generate STOP after the current byte
        data[0] = (uint8_t)I2C1->DR; // Byte 1 (from DR)
        data[1] = (uint8_t)I2C1->DR; // Byte 2 (moved from the shift register)

        __set_PRIMASK(primask); // Restore the previous interrupt state

        I2C1->CR1 &= ~I2C_CR1_POS; // Leave POS clear for the next transfer
    }
    else
    {
        (void)I2C1->SR1; // Clear ADDR: read SR1 ...
        (void)I2C1->SR2; // ... then SR2; ACK stays enabled

        uint8_t remaining = len; // Bytes still to be read
        uint8_t idx = 0;         // Next position in data[]

        /* All bytes up to N-3 are plain RXNE reads with ACK. */
        while (remaining > 3u)
        {
            timeout_start(&i2c_timeout, i2c_timeout_ms);
            while (!(I2C1->SR1 & I2C_SR1_RXNE)) // Wait for the next byte
            {
                if (timeout_has_expired(&i2c_timeout))
                {
                    i2c_status_error = i2c_timeout_err_rxne;
                    i2c_abort();
                    return i2c_error;
                }
            }
            data[idx++] = (uint8_t)I2C1->DR; // Read and ACK
            remaining--;
        }

        /* 3 bytes left: wait for BTF (DataN-2 in DR, DataN-1 in the shift
           register), only then clear ACK and read DataN-2. */
        timeout_start(&i2c_timeout, i2c_timeout_ms);
        while (!(I2C1->SR1 & I2C_SR1_BTF))
        {
            if (timeout_has_expired(&i2c_timeout))
            {
                i2c_status_error = i2c_timeout_err_btf_read;
                i2c_abort();
                return i2c_error;
            }
        }

        /* ACK = 0 and the DataN-2 read must happen together, before DataN-1
           finishes being acknowledged; interrupts are disabled for that. */
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        I2C1->CR1 &= ~I2C_CR1_ACK; // The last byte will be NACKed
        data[idx++] = (uint8_t)I2C1->DR; // DataN-2
        __set_PRIMASK(primask); // Restore the previous interrupt state
        remaining--;

        /* 2 bytes left: wait for BTF again, then STOP and read them both. */
        timeout_start(&i2c_timeout, i2c_timeout_ms);
        while (!(I2C1->SR1 & I2C_SR1_BTF)) // DataN-1 in DR, DataN in the shift register
        {
            if (timeout_has_expired(&i2c_timeout))
            {
                i2c_status_error = i2c_timeout_err_btf_read;
                i2c_abort();
                return i2c_error;
            }
        }

        primask = __get_PRIMASK();
        __disable_irq();
        I2C1->CR1 |= I2C_CR1_STOP;       // Generate STOP before reading the last two bytes
        data[idx++] = (uint8_t)I2C1->DR; // DataN-1
        data[idx++] = (uint8_t)I2C1->DR; // DataN
        __set_PRIMASK(primask); // Restore the previous interrupt state
    }

    timeout_start(&i2c_timeout, i2c_timeout_ms); // Start timeout for bus idle
    while (I2C1->SR2 & I2C_SR2_BUSY)             // Wait until STOP is on the bus
    {
        if (timeout_has_expired(&i2c_timeout)) // STOP did not complete
        {
            i2c_status_error = i2c_timeout_err_stop;
            (void)i2c_bus_recover();                 // Release the bus for the next transfer
            i2c_status_error = i2c_timeout_err_stop; // Recovery overwrote the status: restore the cause
            return i2c_error;
        }
    }

    I2C1->CR1 |= I2C_CR1_ACK; // Re-enable ACK for the next reception

    return i2c_ok; // All bytes received and bus released
}
