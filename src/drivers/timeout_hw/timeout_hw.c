#include "timeout_hw.h"
#include "../tim/tim2_hw.h"

/* Arms a timeout: remembers the current time and the duration. */
void timeout_start(timeout_t *t, uint32_t timeout_ms)
{
   t->start_ms = sys_ms;       // Current time in milliseconds
   t->timeout_ms = timeout_ms; // Duration in milliseconds
}

/* Expired when the elapsed time reaches the duration. */
uint8_t timeout_has_expired(timeout_t *t)
{
   return ((uint32_t)(sys_ms - t->start_ms) >= t->timeout_ms); // Unsigned difference is correct across sys_ms wrap-around
}