#ifndef RADIOLIB_STM32_HAL_H
#define RADIOLIB_STM32_HAL_H

#include <RadioLib.h>

/**
 * STM32F103RadioLibHal constructor
 * Initializes the base RadioLibHal class with platform-specific values for GPIO modes, levels, and interrupt modes.
 * - GpioModeInput: 0 (represents input mode for GPIO pins)
 * - GpioModeOutput: 1 (represents output mode for GPIO pins)
 * - GpioLevelLow: 0 (represents low logic level for GPIO pins)     
 * - GpioLevelHigh: 1 (represents high logic level for GPIO pins)
 * - GpioInterruptRising: 2 (represents rising edge interrupt mode for GPIO pins)
 * - GpioInterruptFalling: 3 (represents falling edge interrupt mode for GPIO pins)
 * These values are used by the RadioLib library to configure GPIO pins and interrupts correctly on the STM32F103 platform.
 * This class performs:
 * - GPIO pin mode configuration is empty as the pin configuration is handled in the lora_sx1276.c file, which initializes the necessary pins for the LoRa module.
 * - Digital write and read operations
 * - External interrupt handling
 * - Timing functions (delay, millis, micros)
 * - SPI communication functions (begin, transfer, end)
 * The actual pin numbers and their corresponding functions (e.g., reset, NSS, DIO0) 
 * are defined in the implementation of the methods, allowing the RadioLib library to interact with the hardware correctly.
 */

class STM32F103RadioLibHal : public RadioLibHal {
public:

void (*dio0Callback)(void) = nullptr;

STM32F103RadioLibHal():
    RadioLibHal(0, 1, 0, 1, 2, 3) {};


void pinMode(uint32_t pin, uint32_t mode) override;

void digitalWrite(uint32_t pin, uint32_t value) override;

uint32_t digitalRead(uint32_t pin) override;

void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override;

void detachInterrupt(uint32_t interruptNum) override;

void delay(RadioLibTime_t ms) override;

void delayMicroseconds(RadioLibTime_t us) override;

RadioLibTime_t millis() override;

RadioLibTime_t micros() override;

long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override;

void spiBegin() override;

void spiBeginTransaction() override;

void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override;

void spiEndTransaction() override;

void spiEnd() override;

};

extern STM32F103RadioLibHal hal;

#endif 

