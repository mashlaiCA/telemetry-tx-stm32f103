#include "spi_LL.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_spi.h"
#include "../timeout_hw/timeout_hw.h"

volatile spi_error_t spi_error = spi_error_none; // Last SPI failure

static timeout_t spi_timeout; // Timeout of the current wait step

/* SPI1 master, mode 0, 8-bit, MSB first, software NSS, 125 kHz (see spi_LL.h). */
void spi_init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1); // Enable SPI1 clock

    LL_SPI_Disable(SPI1); // Disable SPI1 before configuration

    LL_SPI_SetMode(SPI1, LL_SPI_MODE_MASTER); // Master mode (MSTR = 1, SSI = 1)

    LL_SPI_SetTransferDirection(SPI1, LL_SPI_FULL_DUPLEX); // Set SPI1 to full duplex mode

    LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_8BIT); // Set SPI1 data width to 8 bits

    LL_SPI_SetClockPolarity(SPI1, LL_SPI_POLARITY_LOW); // CPOL = 0: SCK idles low

    LL_SPI_SetClockPhase(SPI1, LL_SPI_PHASE_1EDGE); // CPHA = 0: data sampled on the first (rising) edge - SPI mode 0

    LL_SPI_SetNSSMode(SPI1, LL_SPI_NSS_SOFT); // SSM = 1: NSS handled in software (PB12 GPIO)

    LL_SPI_SetBaudRatePrescaler(SPI1, LL_SPI_BAUDRATEPRESCALER_DIV64); // PCLK2 8 MHz / 64 = 125 kHz SCK

    CLEAR_BIT(SPI1->CR1, SPI_CR1_LSBFIRST); // Set SPI1 to transmit MSB first

    LL_SPI_Enable(SPI1); // Enable SPI1 after configuration

    (void)SPI1->DR; // Read DR: discard a stale received byte (clears RXNE)
    (void)SPI1->SR; // Read SR: together with the DR read, clears a stale OVR flag

    if(!(SPI1->CR1 & SPI_CR1_SPE)) {
        spi_error = spi_error_init; // Peripheral did not enable
    }
}

/* Reports the first configuration/state problem found (0 = OK). */
uint8_t spi_check_hw(void){
    if(!(SPI1->CR1 & SPI_CR1_SPE)) return 1; // SPI not enabled
    if(!(SPI1->CR1 & SPI_CR1_MSTR)) return 2; // Not in master mode
    if(!(SPI1->CR1 & SPI_CR1_SSM)) return 3; // NSS not in software mode
    if(!(SPI1->CR1 & SPI_CR1_SSI)) return 4; // NSS internal signal not set
    if(!LL_SPI_IsActiveFlag_TXE(SPI1)) return 5; // Transmit buffer not empty
    if(LL_SPI_IsActiveFlag_BSY(SPI1)) return 6; // SPI is busy

    return 0; // All checks passed
}

/* One full-duplex byte exchange with 100 ms limits on each wait. */
uint8_t spi_transfer(uint8_t data)
{
    // TXE: wait until the transmit buffer can take a byte
    timeout_start(&spi_timeout, 100);
    while (!LL_SPI_IsActiveFlag_TXE(SPI1)) {
        if (timeout_has_expired(&spi_timeout)) {
            spi_error = spi_error_tx;
            return 0; // Byte not sent
        }
    }

    LL_SPI_TransmitData8(SPI1, data); // Start the transfer

    // BSY: wait until the byte has been clocked out
    timeout_start(&spi_timeout, 100);
    while (LL_SPI_IsActiveFlag_BSY(SPI1)) {
        if (timeout_has_expired(&spi_timeout)) {
            spi_error = spi_error_bsy;
            return 0; // Transfer did not finish
        }
    }

    // RXNE: the byte clocked in during the transfer is ready
    timeout_start(&spi_timeout, 100);
    while (!LL_SPI_IsActiveFlag_RXNE(SPI1)) {
        if (timeout_has_expired(&spi_timeout)) {
            spi_error = spi_error_rx;
            return 0; // Nothing received
        }
    }

    return LL_SPI_ReceiveData8(SPI1); // Reading DR clears RXNE
}