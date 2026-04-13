#include "stm32f1xx.h"
#include "exti_1.h"
#include "drivers/gpio/gpio_hw.h"



void EXTI1_init(void) 
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN; // Enable AFIO clock

    AFIO->EXTICR[0] &= ~(0xF << 4);
    AFIO->EXTICR[0] |= (0x0 << 4); // Map EXTI1 to PA1
   
    EXTI->IMR |= EXTI_IMR_MR1;
    EXTI->RTSR |= EXTI_RTSR_TR1; // Trigger on rising edge

    NVIC_EnableIRQ(EXTI1_IRQn); // Enable EXTI1 interrupt in NVIC
}

 void EXTI1_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR1) { // Check if EXTI1 triggered the interrupt
        EXTI->PR |= EXTI_PR_PR1; // Clear the interrupt flag
        
        if(hal.dio0Callback) {
            hal.dio0Callback(); // Call the registered callback function
        }
    }
}