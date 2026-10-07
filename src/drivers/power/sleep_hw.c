#include "sleep_hw.h"
#include "stm32f103xb.h"
#include "../tim/tim2_hw.h" // sys_time_add_ms(): compensates for TIM2 being stopped in Stop mode

/* Enters Stop mode (RM0008, "Low-power modes" - Stop mode) and blocks in WFI until a wakeup interrupt. */
void sleep_enter_stop(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable PWR peripheral clock

    PWR->CR &= ~PWR_CR_PDDS; // PWR_CR.PDDS = 0: deep sleep is Stop mode (not Standby)
    PWR->CR |= PWR_CR_LPDS;  // PWR_CR.LPDS = 1: regulator in low-power mode during Stop
    PWR->CR |= PWR_CR_CWUF;  // PWR_CR.CWUF: clear the wakeup flag

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // SCR.SLEEPDEEP: WFI enters Stop instead of Sleep

    /* Interrupts are masked with PRIMASK before WFI. Without this, an EXTI0
       edge arriving between here and __WFI() would run the handler first
       (clearing the pending flag), and WFI would then wait for an edge that
       never comes, because the DS3231 keeps INT low until A1F is cleared.
       With PRIMASK = 1 the request stays pending: the Cortex-M3 still wakes
       from WFI on a pending interrupt that is enabled in the NVIC, and the
       handler runs right after PRIMASK is restored below.
       This protects only the window from __disable_irq() to __WFI(); edges
       before it are covered by the caller's PA0 level check. */
    uint32_t primask = __get_PRIMASK(); // Save the caller's interrupt state
    __disable_irq();

    __DSB(); // all writes to PWR/SCB registers must reach the peripherals before sleep
    __ISB(); // Flush the pipeline so WFI executes with the new SCR value

    __WFI(); // Wait for interrupt, enters Stop mode here

    // HSI 8 MHz with no PLL: the hardware selects HSI as the system clock on
    // Stop exit, so no SystemClock_Config() call is needed here.
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; // Clear SLEEPDEEP: a later WFI must be a normal Sleep

    __set_PRIMASK(primask); // Restore PRIMASK: the EXTI0 handler that woke us runs now
}

/* Stop mode plus software clock compensation (see sleep_hw.h). */
void sleep_enter_stop_for(uint32_t expected_ms)
{
    sleep_enter_stop(); // Returns after wakeup

    /* TIM2 is stopped in Stop mode: sys_ms/sys_us are not incremented and
       TIM2->CNT resumes from its frozen value, so the sleep interval is missing
       from the software clock. Timers based on timeout_t behave as if no time
       had passed, which suits the FSMs, but millis_time() would lag real time.
       The armed alarm duration is added instead. Its accuracy (about +-1 s,
       the DS3231 alarm resolution) is sufficient because the DS3231, not
       sys_ms, is the authoritative date/time source. */
    sys_time_add_ms(expected_ms); // Add the expected sleep duration
}

// Keep SWD alive during Stop mode (development only)
void sleep_debug_enable(void)
{
    DBGMCU->CR |= DBGMCU_CR_DBG_STOP; // DBGMCU_CR.DBG_STOP: keep debug clocks running in Stop mode
}
