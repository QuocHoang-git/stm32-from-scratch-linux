/**
 ******************************************************************************
 * @file      main.c
 * @author    Duong Quoc Hoang
 * @brief     Bare-metal Blinky using Official ARM & ST CMSIS Headers.
 ******************************************************************************
 */

#include "stm32f1xx.h"

/* Busy-wait delay function */
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}

int main(void) {
    /* Call CMSIS system initialization (sets up reset clock configuration) */
    SystemInit();

    /* 1. Enable Port C Clock via official CMSIS macro */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* 2. Configure PC13 as Output Push-Pull (2 MHz) */
    /* Clear both CNF13 and MODE13 (Bits 23:20) */
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    /* Set MODE13 to 2 MHz (0b10) and CNF13 to Push-Pull (0b00) */
    GPIOC->CRH |= GPIO_CRH_MODE13_1;

    /* 3. Toggle PC13 LED in an infinite loop */
    while (1) {
        GPIOC->ODR ^= GPIO_ODR_ODR13;
        delay(400000);
    }

    return 0;
}
