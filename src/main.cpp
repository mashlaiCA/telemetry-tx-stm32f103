
#include "stm32f1xx.h"
#include "board/board_pins.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"
#include "application/protocols/uart/uart.h"
#include "application/time/time.h"
#include "application/protocols/i2c/i2c.h"
#include "devices/Sensor_SHT35/sensor_sht35.h"
#include "devices/Sensor_SHT35/sht35_fsm.h"


timeout_t system_timeout;  // Timeout structure for LoRa operations
uint8_t timeout_ms; // Timeout duration in milliseconds
uint8_t started = 0; // Flag to indicate if the system has started

uint16_t count = 1;
  

int main(void) // Main function
{

  
  pins_init(); 
  i2c_SDA_SCL(6); // SCL
  i2c_SDA_SCL(7); // SDA
  i2c_start(); // Initialize I2C peripheral

  timer_start();  // Start the system timer for timekeeping
  __enable_irq(); // Enable global interrupts

 
  timer_set(&system_timeout, 200);
  while (!timer_wait(&system_timeout))
  ;
  uart_init();          
  
  analog_sensor_soil_moisture_init(3); // Initialize soil moisture sensor on PA3/ADC1 channel 3
  analog_sensor_soil_moisture_init(2); // !!!??Initialize soil moisture sensor on PA2/ADC1 channel 2
  while (1) 
  {
    SHT35_FSM_Run();

    if(!started){
    timer_set(&system_timeout, 500);
    started = 1;
    }

    if(timer_wait(&system_timeout))
    {
      started = 0; // Reset the started flag to allow the next timeout to start
      
        uart_send_uint16_t2(analog_sensor_soil_moisture_read(3),
                        analog_sensor_soil_moisture_read(2),
                        temperatureSHT35(),
                        humiditySHT35());
    } 
}
}
  

  