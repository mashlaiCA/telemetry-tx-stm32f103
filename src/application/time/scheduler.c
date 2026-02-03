#include "scheduler.h"
#include "drivers/tim/tim2_hw.h"

void scheduler_init(uint32_t time_scheduler_ms)
{
	timer_init(time_scheduler_ms); // Initialize TIM2 for system timekeeping
}

void scheduler_tick(void)
{
	timer_tick(); // Update system time (sys_ms)
}

uint8_t scheduler_delay_ms(uint32_t time_ms)
{

	if (sys_ms + 1 == time_ms) // Check if the specified time interval has elapsed
	{
		return 1; // Return true if elapsed
	}
	return 0; // Return false if not elapsed
}