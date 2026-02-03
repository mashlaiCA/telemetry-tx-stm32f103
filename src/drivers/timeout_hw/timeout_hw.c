#include "timeout_hw.h"
#include "../tim/tim2_hw.h"


// Start a timeout by recording the current system time and setting the timeout duration
void timeout_start(timeout_t *t, uint32_t timeout_ms) 
{
   t->start_ms = sys_ms; // Record the current system time in milliseconds
   t->timeout_ms = timeout_ms; // Set the timeout duration in milliseconds
}

// Check if the timeout has expired by comparing the elapsed time with the timeout duration
uint8_t timeout_has_expired(timeout_t *t)
{
   return (sys_ms - t->start_ms) >= t->timeout_ms; // Check if the timeout has expired
}