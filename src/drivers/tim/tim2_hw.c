#include "stm32f103xb.h"
#include "tim2_hw.h"

volatile uint32_t sys_ms = 0; // System milliseconds counter
volatile uint64_t sys_us = 0; 

void TIM2_IRQHandler(void)
{
	if (TIM2->SR & TIM_SR_UIF) // Check for update interrupt
	{
		TIM2->SR &= ~TIM_SR_UIF; // Clear interrupt flag
		sys_ms++;				 // Increment system milliseconds counter
		sys_us += 1000; 
	}
}

void timer_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock

	RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST;
    TIM2->CR1 = 0; // Reset control register
	TIM2->CNT = 0; // Reset counter
	TIM2->PSC = 8 - 1;	  
	TIM2->ARR = 1000 - 1;  // Set auto-reload value for desired time_ms

	TIM2->EGR |= TIM_EGR_UG; // Generate an update event to load the prescaler value
    
	TIM2->SR &= ~TIM_SR_UIF; // Clear update interrupt flag
	TIM2->DIER |= TIM_DIER_UIE; // Enable update interrupt
	NVIC_EnableIRQ(TIM2_IRQn); // Enable TIM2 interrupt in NVIC

	TIM2->CR1 |= TIM_CR1_CEN; // Enable TIM2
}

void delay_hw_ms(uint32_t ms){
	uint32_t start = millis_hw();
	while ((millis_hw() - start) < ms)
		;
}

void delay_hw_us(uint32_t us){
	uint32_t start = micros_hw();
	while ((micros_hw() - start) < us)
		;
}

uint32_t micros_hw(void) {
	return sys_us + TIM2->CNT;
}	

uint32_t millis_hw(void){
	return sys_ms; 
}
