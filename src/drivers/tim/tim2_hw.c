#include "stm32f103xb.h"
#include "tim2_hw.h"

volatile uint32_t sys_ms = 0; // System milliseconds counter

void TIM2_IRQHandler(void)
{
	if (TIM2->SR & TIM_SR_UIF) // Check for update interrupt
	{
		TIM2->SR &= ~TIM_SR_UIF; // Clear interrupt flag
		sys_ms++;				 // Increment system milliseconds counter
	}
}

void timer_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock
	TIM2->CR1 = 0;						// Configure TIM2 control register
	TIM2->CNT = 0;						// Reset TIM2 counter
	TIM2->PSC = 8 - 1;					// Set prescaler for 1 ms tick (assuming 8 MHz clock)
	TIM2->ARR = 1000 - 1;				// Set auto-reload value for desired time_ms

	TIM2->EGR |= TIM_EGR_UG; // Generate an update event to load the prescaler value
	TIM2->SR &= ~TIM_SR_UIF; // Clear update interrupt flag

	TIM2->DIER |= TIM_DIER_UIE; // Enable update interrupt
	NVIC_EnableIRQ(TIM2_IRQn);	// Enable TIM2 interrupt in NVIC
	TIM2->CR1 |= TIM_CR1_CEN;	// Enable TIM2
}
