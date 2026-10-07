/**
 * @file  radiolib_stm32_hal.h
 * @brief RadioLib hardware abstraction layer (HAL) for this STM32F103 board.
 * This file provides:
 * 1. STM32F103RadioLibHal - RadioLibHal implementation on top of the project drivers.
 * 2. The global instance hal.
 * RadioLib refers to pins by abstract numbers; this HAL maps them as
 * 0 = NSS (PB12), 1 = DIO0 (PB1), 2 = RST (PB5), matching Module(&hal, 0, 1, 2, ...) in main.cpp.
 * Pin modes are fixed by gpio_hw.c (pins_init()), so pinMode() is a no-op; the DIO0
 * interrupt is configured by EXTI1_init() and dispatched through dio0Callback.
 */

#ifndef RADIOLIB_STM32_HAL_H
#define RADIOLIB_STM32_HAL_H

#include <RadioLib.h>

/**
 * @brief RadioLib HAL implementation for the STM32F103 board.
 * Constructor values passed to RadioLibHal: GpioModeInput = 0, GpioModeOutput = 1,
 * GpioLevelLow = 0, GpioLevelHigh = 1, GpioInterruptRising = 2, GpioInterruptFalling = 3.
 * Provides digital write/read on the mapped pins, the DIO0 callback,
 * delays and time from the TIM2 time base, and SPI transfers over SPI1.
 */

class STM32F103RadioLibHal : public RadioLibHal {
public:

void (*dio0Callback)(void) = nullptr; // DIO0 callback set by attachInterrupt(1, ...); called from EXTI1_IRQHandler

STM32F103RadioLibHal():
    RadioLibHal(0, 1, 0, 1, 2, 3) {}; // input, output, low, high, rising, falling


void pinMode(uint32_t pin, uint32_t mode) override; // No-op: pins are configured by gpio_hw.c

void digitalWrite(uint32_t pin, uint32_t value) override; // Pin 2 -> RST (PB5), pin 0 -> NSS (PB12); others ignored

uint32_t digitalRead(uint32_t pin) override; // Pin 1 -> DIO0 (PB1); others read as 0

void attachInterrupt(uint32_t interruptNum, void (*interruptCb)(void), uint32_t mode) override; // interruptNum 1: store DIO0 callback (mode ignored)

void detachInterrupt(uint32_t interruptNum) override; // interruptNum 1: remove DIO0 callback

void delay(RadioLibTime_t ms) override; // Blocking delay in ms (delay_ms())

void delayMicroseconds(RadioLibTime_t us) override; // Blocking delay in us (delay_us())

RadioLibTime_t millis() override; // millis_time()

RadioLibTime_t micros() override; // micros_time()

long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t timeout) override; // Not implemented: always returns 0

void spiBegin() override; // Initializes SPI1 (spi_start())

void spiBeginTransaction() override; // No-op: SPI settings are fixed

void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override; // Byte-by-byte full-duplex transfer

void spiEndTransaction() override; // No-op

void spiEnd() override; // No-op: SPI1 stays enabled

};

extern STM32F103RadioLibHal hal; // Single HAL instance, defined in radiolib_stm32_hal.cpp

#endif

