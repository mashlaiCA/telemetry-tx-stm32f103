#include "board/board_pins.h"
#include "devices/analog_sensor_soil_moisture/analog_sensor_soil_moisture.h"
#include "devices/led_light_bulb/led_light_bulb.h"
#include "application/uart/uart.h"
#include "application/time/scheduler.h"
#include "drivers/I2C/i2c.h"
#include "drivers/gpio/gpio.h"
#include "drivers/sht31/sht31.h"


uint16_t count = 100; // Counter for readings

uint8_t cmd[2] = {0x24, 0x00};
uint8_t rx[6];
float temperature;
float humidity;

int main(void)// Main function
{
    gpio_B_init();
    gpio_B_init_I2C_SDA_SCL(6);//SCL
    gpio_B_init_I2C_SDA_SCL(7);//SDA

    I2C_init();
    I2C_interrupts_init();
   

   pins_init(); // Initialize board pins

    uart_init(); // Initialize UART
    scheduler_init(6000); // Initialize scheduler
    //analog_sensor_soil_moisture_init(3); // Initialize soil moisture sensor on PA3/ADC1 channel 3
    //analog_sensor_soil_moisture_init(4); // Initialize soil moisture sensor on PA4/ADC1 channel 4
    //led_light_init(1); // Initialize LED light bulb on PA5
    //led_light_init(2); // Initialize LED light bulb on PA6
    //led_ON(1); 
    //led_ON(2); 

    while (1)// Main loop
    {
      scheduler_tick();// Update scheduler (timer)
      if(scheduler_delay_ms(3000)) {
        SHT31_StartMeasurement();
    }
 if(scheduler_delay_ms(3000)) {
        SHT31_ReadData(&temperature, &humidity);
      
        uart_send_uint16_t(count++);
        uart_send_string(": Temp=");
        uart_send_uint16_t((uint16_t)(temperature * 100)); // Send temperature multiplied by 100
        uart_send_string(" Hum=");
        uart_send_uint16_t((uint16_t)(humidity * 100)); // Send humidity multiplied by 100
        uart_send_string("\r\n");
      }
    /* if (scheduler_delay_ms(2000)) 
       {
            led_OFF(1);  
            uint16_t adc_value3 = analog_sensor_soil_moisture_read(3); 
            uart_send_uint16_t(count++); 
            uart_send_string(": ");
            uart_send_uint16_t(adc_value3);  
            led_ON(1);  
        }
        if(scheduler_delay_ms(1000)){
           led_OFF(2);
            uart_send_string(", ");
            uint16_t adc_value4 = analog_sensor_soil_moisture_read(4);
            uart_send_uint16_t(adc_value4); 
            uart_send_string("\r\n"); 
            led_ON(2);
        }

*/
    }  

  }