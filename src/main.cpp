
#include "stm32f1xx.h"
#include "board/board_pins.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"
#include "radiolib_stm32_hal/radiolib_stm32_hal.h"
#include "protocols/spi/spi.h"
#include "devices/lora_sx1276/lora_sx1276.h"
#include "protocols/uart/uart.h"
#include "application/time/time.h"
#include "devices/lora_sx1276/lora_fsm.h"
#include "protocols/i2c/i2c.h"
#include "devices/Sensor_SHT35/sensor_sht35.h"
#include "devices/Sensor_SHT35/sht35_fsm.h"
#include "application/system_data/system_data.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture_fsm.h"

extern STM32F103RadioLibHal hal;

Module module(&hal, 0, 1, 2, RADIOLIB_NC);
SX1276 radio(&module);

timeout_t system_timeout; // Timeout structure for LoRa operations
uint8_t timeout_ms;       // Timeout duration in milliseconds
uint8_t started = 0;      // Flag to indicate if the system has started

uint16_t count = 1;

int main(void) // Main function
{

  pins_init();
  spi_start(); // Initialize SPI peripheral
  lora_init();

  timer_start();  // Start the system timer for timekeeping

  i2c_SDA_SCL(6); // SCL
  i2c_SDA_SCL(7); // SDA
  i2c_start();    // Initialize I2C peripheral

  
 __enable_irq(); // Enable global interrupts

  hal.spiBegin();
  

  timer_set(&system_timeout, 200);
  while (!timer_wait(&system_timeout))
    ;

  lora_fsm_init(&radio);
  analog_sensor_fsm_init();
  uart_init();

  analog_sensor_soil_moisture_init(3); // Initialize soil moisture sensor on PA2/ADC1 channel 3
  analog_sensor_soil_moisture_init(2); // Initialize soil moisture sensor on PA3/ADC1 channel 2

  while (1)
  {

    lora_fsm_run();
    SHT35_FSM_Run();
    analog_sensor_FSM_Run();
    

    if (!started)
    {
      timer_set(&system_timeout, 3000);
      started = 1;
    }

    if (timer_wait(&system_timeout))
    {
      started = 0; // Reset the started flag to allow the next timeout to start

            sensor_update(&system_data);
            build_payload(&system_data);

              uart_send_string(system_data.data_string);
              uart_print_int(g_begin_state);
              uart_print_int(hal.millis());
              uart_print_int(hal.micros());
              uart_print_int(spi_check());
/*
      uart_send_uint16_t2(temperatureSHT35(),
                          humiditySHT35(),
                          analog_sensor_soil_moisture_read(2),
                          analog_sensor_soil_moisture_read(3));
                          */
    }
  }
}
