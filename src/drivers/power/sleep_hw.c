#include "sleep_hw.h"
#include "stm32f103xb.h"

// Enter Stop mode and block (WFI) until a wakeup interrupt occurs
void sleep_enter_stop(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; // Enable PWR peripheral clock

    PWR->CR &= ~PWR_CR_PDDS; // Clear PDDS: enter Stop mode (not Standby) on deepsleep
    PWR->CR |= PWR_CR_LPDS;  // Regulator in low-power mode during Stop
    PWR->CR |= PWR_CR_CWUF;  // Clear the wakeup flag

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; // Set SLEEPDEEP so WFI enters Stop mode

    __WFI(); // Wait for interrupt, enters Stop mode here

    // HSI 8MHz with no PLL: HSI is already the clock source on wake,
    // so no SystemClock_Config() call is needed here.
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; // Clear SLEEPDEEP after wake
}

// Keep SWD alive during Stop mode (development only)
void sleep_debug_enable(void)
{
    DBGMCU->CR |= DBGMCU_CR_DBG_STOP; // Keep debug clocks running in Stop mode
}
