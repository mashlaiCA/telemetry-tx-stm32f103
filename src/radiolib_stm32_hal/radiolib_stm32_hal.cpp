#include "radiolib_stm32_hal.h"

#include "application/time/time.h"
#include "devices/lora_sx1276/lora_sx1276.h"
#include "protocols/spi/spi.h"
#include "../drivers/external_interrupt/exti_1.h"

STM32F103RadioLibHal hal;

void STM32F103RadioLibHal::pinMode(uint32_t pin, uint32_t mode) { 
    (void)pin;
    (void)mode;
}

void STM32F103RadioLibHal::digitalWrite(uint32_t pin, uint32_t value) {
    if(pin == 2) value ? lora_rst_high() : lora_rst_low();
    else if(pin == 0) value ? lora_nss_high() : lora_nss_low();
}

uint32_t STM32F103RadioLibHal::digitalRead(uint32_t pin){ 
    if(pin == 1) {
        return lora_dio0_read();
    }
    return 0;
}

void STM32F103RadioLibHal::attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode){

    if(interruptNum == 1) {
        this->dio0Callback = interruptCb;
        //EXTI1_init();
    }

}

void STM32F103RadioLibHal::detachInterrupt(uint32_t interruptNum){
    
    if(interruptNum == 1) {
        this->dio0Callback = nullptr; 
    }
    
}

void STM32F103RadioLibHal::delayMicroseconds(RadioLibTime_t us){ 
    delay_us(us);
}

void STM32F103RadioLibHal::delay(RadioLibTime_t ms) { 
    delay_ms(ms);
}

RadioLibTime_t STM32F103RadioLibHal::millis() {
    return millis_time();
}

RadioLibTime_t STM32F103RadioLibHal::micros() {
    return micros_time();
}

long STM32F103RadioLibHal::pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) {
    (void)pin;
    (void)state;
    (void)timeout;
    return 0;
}

void STM32F103RadioLibHal::spiBegin() {
  spi_start();
}

void STM32F103RadioLibHal::spiBeginTransaction(){}


void STM32F103RadioLibHal::spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
    for(size_t i = 0; i < len; i++) {
        uint8_t r = spi_send(out ? out[i] : 0x00);
        if(in) {
            in[i] = r;
        }
    }
}

void STM32F103RadioLibHal::spiEndTransaction(){}

void STM32F103RadioLibHal::spiEnd(){}

