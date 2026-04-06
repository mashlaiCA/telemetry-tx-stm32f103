#include "time.h"
#include "drivers/timeout_hw/timeout_hw.h"
#include "drivers/tim/tim2_hw.h"

void timer_start(void)
{
    timer_init(); // Initialize TIM2 for system timekeeping
}

void timer_set(timeout_t *t, uint32_t timeout_ms)
{
    timeout_start(t, timeout_ms); // Start a timeout with the specified duration
}

uint8_t timer_wait(timeout_t *t)
{
    return timeout_has_expired(t); // Return whether the timeout has expired
}