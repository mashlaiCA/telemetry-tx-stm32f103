#include "stm32f103xb.h"

volatile uint32_t sys_ms = 0; // System milliseconds counter

/**
 * @brief Initialize TIM2 for system timekeeping.
 */
void timer_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock

	TIM2->PSC = 4000 - 1; // Set prescaler because 8MHz/4000 = 2000 Hz (0.5 ms per tick)
	TIM2->ARR = 1;		  // Set auto-reload value for 1 second intervals

	TIM2->CNT = 0;			 // Reset counter
	TIM2->SR &= ~TIM_SR_UIF; // Clear status register

	TIM2->CR1 |= TIM_CR1_CEN; // Enable TIM2
}

/**
 * @brief Update system time. Called in main loop.
 */
void timer_tick(void)
{
	if (TIM2->SR & TIM_SR_UIF) // If update interrupt flag is set
	{
		TIM2->SR &= ~TIM_SR_UIF; // Clear update interrupt flag
		sys_ms++;				 // Increment system milliseconds counter
	}
}

/**
 * @brief Check if specified delay in milliseconds has elapsed.
 * @param delay_ms The delay in milliseconds to check.
 */
uint8_t timer_sec(uint32_t delay_ms)
{
	static uint32_t last = 0; // Static variable to hold the last checked time
	if (((sys_ms - last)) >= delay_ms)
	{				   // Check if the delay has elapsed
		last = sys_ms; // Update last checked time
		return 1;	   // Delay has elapsed
	}
	return 0; // Delay has not yet elapsed
}