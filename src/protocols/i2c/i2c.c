#include "i2c.h"
#include "drivers/I2C/i2c_hw.h"
#include "drivers/gpio/gpio_hw.h"

/* Initializes I2C1 through the driver. */
void i2c_start(void)
{
    I2C1_Init(); // FREQ/CCR/TRISE setup and enable; the status (always i2c_ok) is ignored
}

/* Configures one I2C pin on GPIOB. */
void i2c_SDA_SCL(uint8_t pin)
{
    gpio_B_init_I2C_SDA_SCL(pin); // PBx -> alternate function open-drain, 50 MHz
}
/* Bus recovery for the application, so it does not call the driver directly. */
uint8_t i2c_recover(void)
{
    return (i2c_bus_recover() == i2c_bus_recovered) ? 1u : 0u; // 1 = bus released, 0 = still stuck
}
