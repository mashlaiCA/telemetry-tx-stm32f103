
#include "stm32f1xx.h"
#include "board/board_pins.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"

#include "application/protocols/uart/uart.h"
#include "drivers/timeout_hw/timeout_hw.h"
#include "drivers/tim/tim2_hw.h"
#include "drivers/I2C/i2c_hw.h"
#include "drivers/gpio/gpio_hw.h"
#include "devices/Sensor_SHT35/sensor_sht35.h"
#include "drivers/sht35/sht35.h"

#include "drivers/spi/spi_LL.h"
#include "devices/Sensor_SHT35/sht35_fsm.h"


timeout_t system_timeout;  // Timeout structure for LoRa operations
uint8_t timeout_ms; // Timeout duration in milliseconds
uint8_t started = 0; // Flag to indicate if the system has started

uint16_t count = 1;
  

int main(void) // Main function
{

  gpio_B_init();
  pins_init(); 
  gpio_B_init_I2C_SDA_SCL(6); // SCL
  gpio_B_init_I2C_SDA_SCL(7); // SDA
  I2C1_Init(); // Initialize I2C1
  
  timer_init(); // Initialize TIM2 for system timekeeping with a 1 ms tick rate
  __enable_irq(); // Enable global interrupts

 // Initialize board pins
  //spi_init();
  //lora_init(); // Initialize LoRa SX1276 module

  timeout_start(&system_timeout, 200);
  while (!timeout_has_expired(&system_timeout))
  ;
 
    uart_init();          // Initialize UART
  
  
  analog_sensor_soil_moisture_init(3); // Initialize soil moisture sensor on PA3/ADC1 channel 3
  analog_sensor_soil_moisture_init(2); // !!!??Initialize soil moisture sensor on PA2/ADC1 channel 2
  while (1) 
  {
   
    SHT35_FSM_Run();

    if(!started){
    timeout_start(&system_timeout, 5000);
    started = 1;
    }

    if(timeout_has_expired(&system_timeout))
    {
      started = 0; // Reset the started flag to allow the next timeout to start
      
        uart_send_uint16_t2(analog_sensor_soil_moisture_read(3),
                        analog_sensor_soil_moisture_read(4),
                        temperatureSHT35(),
                        humiditySHT35());  
                        
   
    } 
}
}
  

  