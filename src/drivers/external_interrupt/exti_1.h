/**
 * @file  exti_1.h
 * @brief EXTI line 1 (PB1 = SX1276 DIO0) interrupt for RadioLib.
 * This file provides:
 * 1. EXTI1_init() - PB1 rising-edge interrupt setup.
 * 2. Diagnostic counters updated by EXTI1_IRQHandler.
 * The handler forwards each DIO0 edge to the callback registered in the
 * RadioLib HAL (hal.dio0Callback).
 */

#ifndef EXTI_1_H
#define EXTI_1_H

#include "radiolib_stm32_hal/radiolib_stm32_hal.h"

extern STM32F103RadioLibHal hal; // RadioLib HAL instance; holds dio0Callback

extern volatile uint32_t exti1_interrupt_count;          // Number of handled DIO0 interrupts
extern volatile uint32_t exti1_spurious_interrupt_count; // EXTI1_IRQHandler entries with PR1 not set
extern volatile uint8_t exti1_last_state;                // Declared for diagnostics; currently never written
/**
 * @brief Configures EXTI line 1 on PB1 (SX1276 DIO0), rising edge.
 * This function performs the following steps:
 * 1. Enables the AFIO clock and maps EXTI1 to port B (AFIO_EXTICR1.EXTI1 = 0001).
 * 2. Unmasks line 1, enables the rising-edge and disables the falling-edge trigger.
 * 3. Clears any pending request (EXTI->PR and NVIC) and enables EXTI1_IRQn.
 * PB1 itself is configured as input by lora_ctrl_gpio_init().
 */
void EXTI1_init(void);

#endif