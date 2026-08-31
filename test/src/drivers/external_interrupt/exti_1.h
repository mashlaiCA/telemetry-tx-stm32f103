
#ifndef EXTI_1_H
#define EXTI_1_H

#include "radiolib_stm32_hal/radiolib_stm32_hal.h"

extern STM32F103RadioLibHal hal;

extern volatile uint32_t exti1_interrupt_count;
extern volatile uint32_t exti1_spurious_interrupt_count;
extern volatile uint8_t exti1_last_state;
void EXTI1_init(void);

#endif