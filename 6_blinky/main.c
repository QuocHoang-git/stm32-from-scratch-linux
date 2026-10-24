/**
 ******************************************************************************
 * @file      main.c
 * @author    Duong Quoc Hoang
 * @brief     Bare-metal GPIO Blinky for STM32F103 (Blue Pill) on PC13.
 *            Written directly against hardware registers without CMSIS/HAL.
 ******************************************************************************
 */

#include <stdint.h>

/* ========================================================================= */
/* Register Definitions (Reference Manual RM0008)                            */
/* ========================================================================= */

/* Reset and Clock Control (RCC) */
#define RCC_BASE            0x40021000UL
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x18UL))
#define RCC_APB2ENR_IOPCEN  (1UL << 4)   /* Bit 4: Enable GPIOC clock */

/* General Purpose I/O Port C (GPIOC) */
#define GPIOC_BASE          0x40011000UL
#define GPIOC_CRH           (*(volatile uint32_t *)(GPIOC_BASE + 0x04UL))
#define GPIOC_ODR           (*(volatile uint32_t *)(GPIOC_BASE + 0x0CUL))
#define GPIOC_BSRR          (*(volatile uint32_t *)(GPIOC_BASE + 0x10UL))

/* Pin definitions */
#define LED_PIN             13UL

/* Simple busy-wait delay function */
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}

int main(void) {
    /* --------------------------------------------------------------------- */
    /* 1. Enable peripheral clock for GPIOC (RCC_APB2ENR Bit 4)              */
    /* --------------------------------------------------------------------- */
    RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* --------------------------------------------------------------------- */
    /* 2. Configure PC13 as Output Push-Pull (Max speed 2 MHz)               */
    /*    In GPIOC_CRH:                                                      */
    /*    - Pin 13 controls bits [23:20] (CNF13[1:0] and MODE13[1:0])        */
    /*    - MODE13 = 10 (Output 2 MHz)                                       */
    /*    - CNF13  = 00 (General purpose output push-pull)                   */
    /* --------------------------------------------------------------------- */
    GPIOC_CRH &= ~(0xFUL << 20);         /* Clear bits 23:20 */
    GPIOC_CRH |=  (0x2UL << 20);         /* MODE13 = 10 (2MHz), CNF13 = 00 */

    /* --------------------------------------------------------------------- */
    /* 3. Infinite loop: Toggle LED PC13                                     */
    /*    Note: On STM32 Blue Pill, the on-board LED is active-LOW:          */
    /*    - PC13 = 0 -> LED ON                                               */
    /*    - PC13 = 1 -> LED OFF                                              */
    /* --------------------------------------------------------------------- */
    while (1) {
        /* Toggle bit 13 in Output Data Register */
        GPIOC_ODR ^= (1UL << LED_PIN);

        /* Wait approximately 500ms (at default 8MHz internal HSI clock) */
        delay(400000);
    }

    return 0;
}

