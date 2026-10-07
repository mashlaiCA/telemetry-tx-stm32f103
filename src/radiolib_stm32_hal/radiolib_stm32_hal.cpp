#include "radiolib_stm32_hal.h"

#include "application/time/time.h"
#include "devices/lora_sx1276/lora_sx1276.h"
#include "protocols/spi/spi.h"
#include "../drivers/external_interrupt/exti_1.h"

STM32F103RadioLibHal hal; // Global HAL instance used by Module in main.cpp and by EXTI1_IRQHandler

/* Pin modes are fixed at startup by gpio_hw.c; nothing to do here. */
void STM32F103RadioLibHal::pinMode(uint32_t pin, uint32_t mode) {
    (void)pin;
    (void)mode;
}

/* Abstract pin 2 = RST, 0 = NSS; writes to other pins are ignored. */
void STM32F103RadioLibHal::digitalWrite(uint32_t pin, uint32_t value) {
    if(pin == 2) value ? lora_rst_high() : lora_rst_low();      // RST (PB5); each call includes a 10 ms delay
    else if(pin == 0) value ? lora_nss_high() : lora_nss_low(); // NSS (PB12)
}

/* Abstract pin 1 = DIO0; other pins read as 0. */
uint32_t STM32F103RadioLibHal::digitalRead(uint32_t pin){
    if(pin == 1) {
        return lora_dio0_read(); // DIO0 (PB1)
    }
    return 0;
}

/* Stores the DIO0 callback; EXTI1 itself is configured once by EXTI1_init()
   (rising edge), so mode is not used. */
void STM32F103RadioLibHal::attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode){

    if(interruptNum == 1) {
        this->dio0Callback = interruptCb; // Called from EXTI1_IRQHandler
        //EXTI1_init();
    }

}

/* Removes the DIO0 callback. */
void STM32F103RadioLibHal::detachInterrupt(uint32_t interruptNum){

    if(interruptNum == 1) {
        this->dio0Callback = nullptr; // EXTI1_IRQHandler will skip the call
    }

}

/* Blocking microsecond delay. */
void STM32F103RadioLibHal::delayMicroseconds(RadioLibTime_t us){
    delay_us(us);
}

/* Blocking millisecond delay. */
void STM32F103RadioLibHal::delay(RadioLibTime_t ms) {
    delay_ms(ms);
}

/* Milliseconds since start (TIM2 time base). */
RadioLibTime_t STM32F103RadioLibHal::millis() {
    return millis_time();
}

/* Microseconds since start (TIM2 time base, 32-bit). */
RadioLibTime_t STM32F103RadioLibHal::micros() {
    return micros_time();
}

/* Not implemented (not needed for LoRa TX). */
long STM32F103RadioLibHal::pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) {
    (void)pin;
    (void)state;
    (void)timeout;
    return 0;
}

/* Initializes SPI1. */
void STM32F103RadioLibHal::spiBegin() {
  spi_start();
}

/* SPI settings are fixed in spi_init(); nothing to do per transaction. */
void STM32F103RadioLibHal::spiBeginTransaction(){}


/* Full-duplex transfer of len bytes. out == NULL sends 0x00 bytes;
   in == NULL discards the received bytes. NSS is driven by RadioLib via digitalWrite(0, ...). */
void STM32F103RadioLibHal::spiTransfer(uint8_t* out, size_t len, uint8_t* in) {
    for(size_t i = 0; i < len; i++) {
        uint8_t r = spi_send(out ? out[i] : 0x00); // Send one byte, receive one byte
        if(in) {
            in[i] = r; // Store the received byte
        }
    }
}

/* Nothing to do at the end of a transaction. */
void STM32F103RadioLibHal::spiEndTransaction(){}

/* SPI1 is never disabled. */
void STM32F103RadioLibHal::spiEnd(){}

