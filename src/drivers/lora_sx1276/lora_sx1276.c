#include "lora_sx1276.h"
#include <stdint.h>
#include "../gpio/gpio_hw.h"
#include "../timeout_hw/timeout_hw.h"
#include "../spi/spi_LL.h"
#include "stm32f1xx_ll_bus.h"
#include "stm32f1xx_ll_spi.h"
#include "stm32f1xx_ll_gpio.h"
#include "../timeout_hw/timeout_hw.h"

/*================= SX1276 Register Map =================*/
#define SX1276_SPI_READ_MASK 0x7F       // MSB = 0 for read operations
#define SX1276_SPI_WRITE_MASK 0x80      // MSB = 1 for write operations
#define REG_FIFO 0x00                   // FIFO read/write access
#define REG_OP_MODE 0x01                // Operating mode and LoRa/FSK selection
#define REG_FRF_MSB 0x06                // Frequency setting (MSB)
#define REG_FRF_MID 0x07                // Frequency setting (MID)
#define REG_FRF_LSB 0x08                // Frequency setting (LSB)
#define REG_PA_CONFIG 0x09              // Power amplifier configuration
#define REG_FIFO_ADDR_PTR 0x0D          // FIFO address pointer for read/write access
#define REG_FIFO_ADDR_TX_BASE_ADDR 0x0E // Base address for FIFO data transmission
#define REG_FIFO_ADDR_RX_BASE_ADDR 0x0F // Base address for FIFO data reception
#define REG_FIFO_RX_CURRENT_ADDR 0x10   // Current address of the last received packet in FIFO
#define REG_IRQ_FLAGS 0x12              // Interrupt flags
#define IRQ_RX_DONE 0x40                // Receive done IRQ flag
#define IRQ_PAYLOAD_CRC_ERROR 0x20      // Payload CRC error IRQ flag
#define REG_PAYLOAD_LENGTH 0x22         // Payload length for LoRa mode
#define REG_RX_NB_BYTES 0x13              // Number of bytes received in the last packet
#define REG_MODEM_CONFIG1 0x1D          // Modem configuration 1 (LoRa settings)
#define REG_MODEM_CONFIG2 0x1E          // Modem configuration 2 (LoRa settings)
#define REG_MODEM_CONFIG3 0x26          // Modem configuration 3 (LoRa settings)
#define REG_SYNC_WORD 0x39              // Sync word for LoRa mode

/* ================= MODES ================= */

#define LONG_RANGE_MODE 0x80    // LoRa mode bit in REG_OP_MODE
#define MODE_RX_CONTINUOUS 0x05 // Continuous receive mode
#define OPMODE_MASK 0x07        // Mask for mode bits in REG_OP_MODE
#define MODE_SLEEP 0x00         // Sleep mode
#define MODE_STDBY 0x01         // Standby mode

#define MODE_TX 0x03 // Transmit mode

#define IRQ_TX_DONE 0x08 // Transmit done IRQ flag

timeout_t lora_timeout;  // Timeout structure for LoRa operations
uint8_t lora_timeout_ms; // Timeout duration in milliseconds

static uint8_t lora_len;      // Variable to store received payload length
static uint8_t lora_buf[256]; // Buffer to store received payload data

void CS_LOW()
{
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_0);
}
void CS_HIGH()
{
    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_0);
}

static uint8_t sx_read(uint8_t reg)
{
    uint8_t value;
    CS_LOW();
    spi_transfer(reg | SX1276_SPI_READ_MASK);
    value = spi_transfer(0x00);
    CS_HIGH();
    return value;
}

static void sx_write(uint8_t reg, uint8_t value)
{
    CS_LOW();
    spi_transfer(reg | SX1276_SPI_WRITE_MASK);
    spi_transfer(value);
    CS_HIGH();
}

static void lora_enter_mode(void)
{
    sx_write(REG_OP_MODE, MODE_SLEEP);                   // Enter sleep mode to allow configuration
    sx_write(REG_OP_MODE, MODE_SLEEP | LONG_RANGE_MODE); // Enable LoRa mode
    sx_write(REG_OP_MODE, MODE_STDBY | LONG_RANGE_MODE); // Enter standby mode to prepare for transmission
}

static void lora_configure(void)
{

    sx_write(REG_FRF_MSB, 0xE4);
    sx_write(REG_FRF_MID, 0xC0);
    sx_write(REG_FRF_LSB, 0x00);

    sx_write(REG_FIFO_ADDR_TX_BASE_ADDR, 0x00); // Set FIFO base address for transmission
    sx_write(REG_FIFO_ADDR_RX_BASE_ADDR, 0x00); // Set FIFO base address for reception

    sx_write(REG_MODEM_CONFIG1, 0x72); // Set bandwidth to 125 kHz, coding rate to 4/5, and explicit header mode
    sx_write(REG_MODEM_CONFIG2, 0x94); // Set spreading factor to 7 and enable CRC
    sx_write(REG_MODEM_CONFIG3, 0x04); // Set low datarate optimization for SF7

    sx_write(REG_SYNC_WORD, 0x34); // Set sync word for LoRa mode
    sx_write(REG_IRQ_FLAGS, 0xFF); // Clear all IRQ flags
}

static void lora_reset(void)
{
    reset_pin_1();
}

void lora_init(void)
{
    lora_reset();
    timeout_start(&lora_timeout, 10);
    while (!timeout_has_expired(&lora_timeout))
        ;
    lora_enter_mode();
    lora_configure();
}

void lora_start_rx(void)
{
    sx_write(REG_IRQ_FLAGS, 0xFF);

    sx_write(REG_FIFO_ADDR_RX_BASE_ADDR, 0x00);
    sx_write(REG_FIFO_ADDR_PTR, 0x00);

    sx_write(REG_OP_MODE, LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

uint8_t lora_check_rx_done(void)
{
    uint8_t irq_flags = sx_read(REG_IRQ_FLAGS);

    if (!(irq_flags & IRQ_RX_DONE))
    {
        return 0; // No new data received
    }

    if (irq_flags & IRQ_PAYLOAD_CRC_ERROR)
    {
        sx_write(REG_IRQ_FLAGS, 0xFF);
        return 0;
    }

    lora_len = sx_read(REG_RX_NB_BYTES); // Get the number of bytes received

    sx_write(REG_FIFO_ADDR_PTR, sx_read(REG_FIFO_RX_CURRENT_ADDR));

    for (uint8_t i = 0; i < lora_len; i++)
    {
        lora_buf[i] = sx_read(REG_FIFO);
    }
    sx_write(REG_IRQ_FLAGS, 0xFF); // Clear IRQ flags after processing

    return lora_buf[0]; // Return the first byte of the received payload
}

void lora_send_byte(uint8_t *data, uint8_t len)
{
   sx_write(REG_OP_MODE, MODE_STDBY | LONG_RANGE_MODE); // Ensure we're in standby mode before transmitting

    sx_write(REG_FIFO_ADDR_PTR, 0x00); // Reset FIFO address pointer to the base address for transmission

    for (uint8_t i = 0; i < len; i++)
    {
        sx_write(REG_FIFO, data[i]); // Write data bytes to FIFO
    }

    sx_write(REG_PAYLOAD_LENGTH, len); // Set the payload length

    sx_write(REG_IRQ_FLAGS, 0xFF); // Clear all IRQ flags before transmission

    sx_write(REG_OP_MODE, MODE_TX | LONG_RANGE_MODE); // Enter transmit mode to send the data

    while(!(sx_read(REG_IRQ_FLAGS) & IRQ_TX_DONE));

    sx_write(REG_IRQ_FLAGS, IRQ_TX_DONE); // Clear the TX done flag after transmission
    
}

void sx1276_send_data(uint8_t data){

 lora_start_rx();

 if(lora_check_rx_done() == 1){
    lora_send_byte(&data, 1); // Send the byte if received data is 1
 }
}