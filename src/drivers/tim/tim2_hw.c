#include "stm32f103xb.h"
#include "tim2_hw.h"

volatile uint32_t sys_ms = 0; // Millisecond clock, advanced by TIM2_IRQHandler and sys_time_add_ms()
volatile uint64_t sys_us = 0; // Microsecond clock at the last tick (always a multiple of 1000)

/* TIM2 update interrupt: one tick per 1 ms. */
void TIM2_IRQHandler(void)
{
	if (TIM2->SR & TIM_SR_UIF) // SR.UIF: update event (counter reached ARR and wrapped)
	{
		TIM2->SR &= ~TIM_SR_UIF; // Clear UIF (rc_w0), otherwise the interrupt re-enters immediately
		sys_ms++;				 // One more millisecond
		sys_us += 1000;			 // Same tick in microseconds


	}
}

/* Configures TIM2 for a 1 MHz count and a 1 ms update interrupt (see tim2_hw.h). */
void timer_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock

	RCC->APB1RSTR |= RCC_APB1RSTR_TIM2RST;  // Reset TIM2 registers to their defaults ...
	RCC->APB1RSTR &= ~RCC_APB1RSTR_TIM2RST; // ... and release the reset
	TIM2->CR1 = 0; // Counter stopped, up-counting, no one-pulse mode
	TIM2->CNT = 0; // Start counting from 0
	TIM2->PSC = 8 - 1;    // 8 MHz / (7 + 1) = 1 MHz: one count per microsecond
	TIM2->ARR = 1000 - 1; // 1000 counts per update: one update per millisecond

	TIM2->EGR |= TIM_EGR_UG; // EGR.UG: load PSC/ARR into the active registers now (PSC is buffered)

	TIM2->SR &= ~TIM_SR_UIF;	// UG also sets UIF: clear it so no extra tick is counted
	TIM2->DIER |= TIM_DIER_UIE; // DIER.UIE: enable update interrupt
	NVIC_EnableIRQ(TIM2_IRQn);	// Enable TIM2 interrupt in NVIC

	TIM2->CR1 |= TIM_CR1_CEN; // CR1.CEN: start the counter
}

/* Busy-wait on sys_ms. The guard counter ends the wait if the tick stops
   (interrupts disabled), so the function can never hang. */
void delay_hw_ms(uint32_t ms)
{
	uint32_t start = millis_hw(); // Reference time
	uint32_t guard = 0;           // Loop iterations so far
	uint32_t guard_limit = (ms + 1u) * 2000u; // Upper bound on iterations (2000 per ms)

	while ((millis_hw() - start) < ms) // Unsigned difference handles sys_ms wrap-around
	{
		if (++guard >= guard_limit)
		{
			return; // Tick not advancing (or loop faster than expected): give up
		}
	}
}

/* Busy-wait on TIM2->CNT, independent of interrupts.
   CNT counts 0..999 (1 us per count) and wraps every 1 ms, so the elapsed
   time is computed modulo 1000; delays must be shorter than 1000 us. */
void wm_delay_hw_us(uint16_t us)
{
	uint16_t start = (uint16_t)TIM2->CNT; // Reference count
	uint16_t elapsed;                     // Microseconds since start

	uint32_t guard = 4u * ((uint32_t)us + 4u); // Upper bound on loop iterations, ends the wait if TIM2 is stopped

	do
	{
		uint16_t now = (uint16_t)TIM2->CNT; // Current count
		elapsed = (now >= start) ? (uint16_t)(now - start)
								 : (uint16_t)(now + 1000 - start); // Counter wrapped at ARR + 1 = 1000

		if (--guard == 0u)
		{
			return; // Counter not advancing: give up
		}
	} while (elapsed < us);
}

/* Returns sys_us + CNT read consistently.
   Interrupts are disabled so sys_us (64-bit, two loads) and CNT come from the
   same millisecond. If the counter has already wrapped but the update
   interrupt has not run yet (UIF pending), sys_us is still the old value:
   1000 us are added and CNT is re-read, because the first read may have been
   taken before the wrap (e.g. 999) and would then be too large. */
uint32_t micros_hw(void)
{
	uint32_t primask = __get_PRIMASK(); // Save the interrupt state of the caller
	__disable_irq();

	uint32_t cnt = TIM2->CNT;                             // Microseconds within the current millisecond
	uint32_t pending = (TIM2->SR & TIM_SR_UIF) ? 1000u : 0u; // Unhandled wrap: sys_us is 1 ms behind

	if (pending)
	{
		cnt = TIM2->CNT; // Re-read: now guaranteed to be after the wrap
	}

	uint32_t us = (uint32_t)sys_us + cnt + pending; // Truncated to 32 bits (wraps every ~71.6 min)

	__set_PRIMASK(primask); // Restore the caller's interrupt state

	return us;
}

/* Busy-wait on micros_hw(); the guard ends the wait if time stops advancing. */
void delay_hw_us(uint32_t us)
{
	uint32_t start = micros_hw(); // Reference time
	uint32_t guard = 0;           // Loop iterations so far
	uint32_t guard_limit = (us + 1u) * 20u; // Upper bound on iterations

	while ((micros_hw() - start) < us) // Unsigned difference handles wrap-around
	{
		if (++guard >= guard_limit)
		{
			return; // Time not advancing: give up
		}
	}
}

/* Millisecond clock getter (32-bit read is atomic on Cortex-M3). */
uint32_t millis_hw(void)
{
	return sys_ms;
}

/* Adds time during which TIM2 was not clocked (Stop mode).
   Interrupts are disabled because sys_us is 64-bit (two stores) and both
   clocks must change together relative to TIM2_IRQHandler. */
void sys_time_add_ms(uint32_t ms)
{
	uint32_t primask = __get_PRIMASK(); // Save the caller's interrupt state
	__disable_irq();

	sys_ms += ms;                   // Advance the millisecond clock
	sys_us += (uint64_t)ms * 1000u; // Advance the microsecond clock by the same amount

	__set_PRIMASK(primask); // Restore the caller's interrupt state
}


//=================test timer PB0
/* Test helper: 100 us high pulse on PB0 for checking wm_delay_hw_us() with a scope. */
void test_wm_100us(void)
{
    GPIOB->BSRR = (1u << 0);        // PB0 high
    wm_delay_hw_us(100);            // 100 us
    GPIOB->BSRR = (1u << (0 + 16)); // PB0 low (BSRR upper half = reset)
}
