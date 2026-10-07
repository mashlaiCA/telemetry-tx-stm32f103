#include "iwdg_hw.h"
#include "stm32f103xb.h"

/* IWDG register access keys (RM0008, 19.4) */
#define IWDG_KEY_RELOAD   0x0000AAAAu // Reload the counter
#define IWDG_KEY_ENABLE   0x0000CCCCu // Start the watchdog
#define IWDG_KEY_UNLOCK   0x00005555u // Allow writes to PR/RLR

/* Period = RLR * (4 * 2^PR) / LSI.
   PR = 6 -> divider 256; RLR = 4095 -> 4095 * 256 / 40000 = 26.2 s. */
#define IWDG_PRESCALER_256   6u    // PR value for divider /256
#define IWDG_RELOAD_MAX      4095u // RLR is 12 bits: maximum reload value

/* Starts the watchdog (see iwdg_hw.h). */
void iwdg_start(void)
{
    /* LSI is enabled explicitly: if it were not running, the IWDG counter would
       not advance and the watchdog would never fire. */
    RCC->CSR |= RCC_CSR_LSION; // RCC_CSR.LSION: turn on the 40 kHz LSI oscillator

    /* The LSI ready wait is bounded: if the oscillator never starts, an
       unbounded wait would hang the device before the watchdog is running,
       i.e. in the one place where nothing could recover it. LSI starts in
       ~85 us; 100000 iterations leave a margin of hundreds of times. */
    for (uint32_t i = 0; !(RCC->CSR & RCC_CSR_LSIRDY) && i < 100000u; i++)
        ; // Wait for RCC_CSR.LSIRDY (bounded)

    IWDG->KR = IWDG_KEY_UNLOCK;   // Unlock PR and RLR
    IWDG->PR = IWDG_PRESCALER_256; // Prescaler /256
    IWDG->RLR = IWDG_RELOAD_MAX;   // Reload value 4095

    /* Wait until the new values are transferred to the working registers. */
    for (uint32_t i = 0; (IWDG->SR != 0u) && i < 100000u; i++)
        ; // IWDG_SR.PVU/RVU clear = update done (bounded)

    IWDG->KR = IWDG_KEY_RELOAD; // Load the counter before starting
    IWDG->KR = IWDG_KEY_ENABLE; // Start. Cannot be stopped anymore - only by reset
}

/* Reloads the counter. */
void iwdg_kick(void)
{
    IWDG->KR = IWDG_KEY_RELOAD; // Counter <- RLR
}

/* Stops the IWDG counter while the core is halted by a debugger. */
void iwdg_debug_freeze(void)
{
    /* Without this every stop at a breakpoint ends in a reset. */
    DBGMCU->CR |= DBGMCU_CR_DBG_IWDG_STOP; // DBGMCU_CR.DBG_IWDG_STOP
}

/* Reads the IWDG reset flag and clears all reset flags. */
uint8_t iwdg_reset_occurred(void)
{
    uint8_t was_iwdg = (RCC->CSR & RCC_CSR_IWDGRSTF) ? 1u : 0u; // RCC_CSR.IWDGRSTF: last reset by IWDG

    RCC->CSR |= RCC_CSR_RMVF; // Clear reset cause flags for next time

    return was_iwdg;
}
