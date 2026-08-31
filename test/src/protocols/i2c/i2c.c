#include "i2c.h"
#include "drivers/I2C/i2c_hw.h"
#include "drivers/gpio/gpio_hw.h"

void i2c_start(void)
{
    I2C1_Init(); // Initialize I2C1 peripheral
}

void i2c_SDA_SCL(uint8_t pin)
{
    gpio_B_init_I2C_SDA_SCL(pin); // Configure GPIOB pins for I2C SDA and SCL
}