
#include "stm32f1xx.h"
#include "board/board_pins.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"

#include "application/protocols/uart/uart.h"
#include "application/time/scheduler.h"
#include "drivers/I2C/i2c_hw.h"
#include "drivers/gpio/gpio_hw.h"
#include "devices/Sensor_SHT35/sensor_sht35.h"
#include "drivers/sht35/sht35.h"
#include "drivers/lora_sx1276/lora_sx1276.h"
#include "drivers/spi/spi_LL.h"



uint16_t count = 1;

// --- I2C and SHT35 functions below ---
//#define SHT35_ADDR   0x44    

int main(void) // Main function
{

  gpio_B_init();
  gpio_B_init_I2C_SDA_SCL(6); // SCL
  gpio_B_init_I2C_SDA_SCL(7); // SDA
  I2C1_Init(); // Initialize I2C1
  //SHT35_Init();!!!!!
  spi_init();
  lora_init(); // Initialize LoRa SX1276 module

  pins_init(); // Initialize board pins

  uart_init();          // Initialize UART
  scheduler_init(5000); // Initialize scheduler

  analog_sensor_soil_moisture_init(3); // Initialize soil moisture sensor on PA3/ADC1 channel 3
  analog_sensor_soil_moisture_init(2); // !!!??Initialize soil moisture sensor on PA2/ADC1 channel 2


  
  while (1) // Main loop
  {

    scheduler_tick();

    Sensor_FSM_Run();

    if(scheduler_delay_ms(5000))
    {
        uart_send_uint16_t2(analog_sensor_soil_moisture_read(3),
                        analog_sensor_soil_moisture_read(4),
                        temperatureSHT35(),
                        humiditySHT35());  
                        
     sx1276_send_data(1);
    } 
}
}
  

  