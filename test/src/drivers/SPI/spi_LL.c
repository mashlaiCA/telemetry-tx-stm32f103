#include "spi_LL.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_spi.h"
#include "../timeout_hw/timeout_hw.h"

volatile spi_error_t spi_error = spi_error_none;

static timeout_t spi_timeout;

void spi_init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1); // Enable SPI1 clock

    LL_SPI_Disable(SPI1); // Disable SPI1 before configuration

    LL_SPI_SetMode(SPI1, LL_SPI_MODE_MASTER); // Set SPI1 to master mode

    LL_SPI_SetTransferDirection(SPI1, LL_SPI_FULL_DUPLEX); // Set SPI1 to full duplex mode

    LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_8BIT); // Set SPI1 data width to 8 bits

    LL_SPI_SetClockPolarity(SPI1, LL_SPI_POLARITY_LOW); // Set SPI1 clock polarity to low

    LL_SPI_SetClockPhase(SPI1, LL_SPI_PHASE_1EDGE); // Set SPI1 clock phase to second(1!!!!) edge

    LL_SPI_SetNSSMode(SPI1, LL_SPI_NSS_SOFT); // Set SPI1 NSS mode to software

    LL_SPI_SetBaudRatePrescaler(SPI1, LL_SPI_BAUDRATEPRESCALER_DIV64); // !!!!! 16 Set SPI1 baud rate prescaler to 16 (for 8 MHz APB2 clock, this gives 0.5 MHz SPI clock)

    CLEAR_BIT(SPI1->CR1, SPI_CR1_LSBFIRST); // Set SPI1 to transmit MSB first

    LL_SPI_Enable(SPI1); // Enable SPI1 after configuration

    (void)SPI1->DR; // Clear any pending flags by reading the status register
    (void)SPI1->SR; // Clear any pending data by reading the data register

    if(!(SPI1->CR1 & SPI_CR1_SPE)) {
        spi_error = spi_error_init;
    }
}

uint8_t spi_check_hw(void){
    if(!(SPI1->CR1 & SPI_CR1_SPE)) return 1; // SPI not enabled
    if(!(SPI1->CR1 & SPI_CR1_MSTR)) return 2; // Not in master mode
    if(!(SPI1->CR1 & SPI_CR1_SSM)) return 3; // NSS not in software mode
    if(!(SPI1->CR1 & SPI_CR1_SSI)) return 4; // NSS internal signal not set
    if(!LL_SPI_IsActiveFlag_TXE(SPI1)) return 5; // Transmit buffer not empty
    if(LL_SPI_IsActiveFlag_BSY(SPI1)) return 6; // SPI is busy

    return 0;
}

uint8_t spi_transfer(uint8_t data)
{
    // TXE
    timeout_start(&spi_timeout, 100);
    while (!LL_SPI_IsActiveFlag_TXE(SPI1)) {
        if (timeout_has_expired(&spi_timeout)) {
            spi_error = spi_error_tx;
            return 0;
        }
    }

    LL_SPI_TransmitData8(SPI1, data);

    // BSY
    timeout_start(&spi_timeout, 100);
    while (LL_SPI_IsActiveFlag_BSY(SPI1)) {
        if (timeout_has_expired(&spi_timeout)) {
            spi_error = spi_error_bsy;
            return 0;
        }
    }

    // RXNE
    timeout_start(&spi_timeout, 100);
    while (!LL_SPI_IsActiveFlag_RXNE(SPI1)) {
        if (timeout_has_expired(&spi_timeout)) {
            spi_error = spi_error_rx;
            return 0;
        }
    }

    return LL_SPI_ReceiveData8(SPI1);
}