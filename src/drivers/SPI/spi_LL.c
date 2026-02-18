 #include "spi_LL.h"
 #include "stm32f1xx_ll_bus.h"
 #include "stm32f1xx_ll_spi.h"
 #include "../gpio/gpio_hw.h"

 void spi_init()
 {
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1); // Enable SPI1 clock

    LL_SPI_Disable(SPI1); // Disable SPI1 before configuration

    LL_SPI_SetMode(SPI1, LL_SPI_MODE_MASTER); // Set SPI1 to master mode

    LL_SPI_SetTransferDirection(SPI1, LL_SPI_FULL_DUPLEX); // Set SPI1 to full duplex mode

    LL_SPI_SetDataWidth(SPI1, LL_SPI_DATAWIDTH_8BIT); // Set SPI1 data width to 8 bits

    LL_SPI_SetClockPolarity(SPI1, LL_SPI_POLARITY_LOW); // Set SPI1 clock polarity to low

    LL_SPI_SetClockPhase(SPI1, LL_SPI_PHASE_1EDGE); // Set SPI1 clock phase to first edge

    LL_SPI_SetNSSMode(SPI1, LL_SPI_NSS_SOFT); // Set SPI1 NSS mode to software

    LL_SPI_SetBaudRatePrescaler(SPI1, LL_SPI_BAUDRATEPRESCALER_DIV16); // Set SPI1 baud rate prescaler to 16 (for 8 MHz APB2 clock, this gives 0.5 MHz SPI clock)

    CLEAR_BIT(SPI1->CR1, SPI_CR1_LSBFIRST); // Set SPI1 to transmit MSB first

    LL_SPI_Enable(SPI1); // Enable SPI1 after configuration
 }

 uint8_t spi_transfer(uint8_t data)
 {
     while (!LL_SPI_IsActiveFlag_TXE(SPI1)); // Wait until transmit buffer is empty
     LL_SPI_TransmitData8(SPI1, data); // Send data byte

     while (!LL_SPI_IsActiveFlag_RXNE(SPI1)); // Wait until receive buffer is not empty
     return LL_SPI_ReceiveData8(SPI1); // Return received data byte
 }