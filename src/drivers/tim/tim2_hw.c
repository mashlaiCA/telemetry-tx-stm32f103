#include "stm32f103xb.h"
#include "tim2_hw.h"

volatile uint32_t sys_ms = 0; // System milliseconds counter

void timer_init(uint32_t time_ms)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock

	TIM2->PSC = 8000 - 1;	  // Set prescaler for 1 ms tick (assuming 8 MHz clock)
	TIM2->ARR = time_ms - 1;  // Set auto-reload value for desired time_ms
	TIM2->CNT = 0;			  // Reset counter
	TIM2->CR1 |= TIM_CR1_CEN; // Enable TIM2
}

void timer_tick(void)
{
	sys_ms = TIM2->CNT; // Update system milliseconds counter
}
