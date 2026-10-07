#include "time.h"
#include "drivers/timeout_hw/timeout_hw.h"
#include "drivers/tim/tim2_hw.h"

/* Starts the 1 ms system tick. */
void timer_start(void)
{
    timer_init(); // TIM2: 1 MHz counter, update interrupt every 1 ms
}

/* Arms a non-blocking timeout. */
void timer_set(timeout_t *t, uint32_t timeout_ms)
{
    timeout_start(t, timeout_ms); // Remember current sys_ms and the duration
}

/* Polls a timeout (non-blocking). */
uint8_t timer_wait(timeout_t *t)
{
    return timeout_has_expired(t); // 1 = expired, 0 = still running
}

/* Blocking millisecond delay. */
void delay_ms(uint32_t ms)
{
   delay_hw_ms(ms);
}

/* Blocking microsecond delay on TIM2->CNT (usable with interrupts disabled, < 1000 us). */
void wm_delay_us(uint16_t us){
   wm_delay_hw_us(us);
}

/* Blocking microsecond delay on micros_hw(). */
void delay_us(uint32_t us){
   delay_hw_us(us);
}

/* Current time in milliseconds. */
uint32_t millis_time(){
   return millis_hw();
}

/* Current time in microseconds (32-bit). */
uint32_t micros_time(){
   return micros_hw();
}