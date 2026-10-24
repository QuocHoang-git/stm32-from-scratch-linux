/**
 ******************************************************************************
 * @file      main.c
 * @author    Duong Quoc Hoang
 * @brief     Part 8: Clock Tree Configuration & PLL Setup for 72 MHz.
 *            Demonstrates boosting the STM32F103 CPU from 8 MHz to 72 MHz.
 ******************************************************************************
 */

#include "stm32f1xx.h"

/**
 * @brief  Configure the System Clock to 72 MHz using external crystal (HSE 8 MHz).
 * 
 * Pipeline:
 *   HSE (8 MHz) -> PLL (x9) -> SYSCLK (72 MHz)
 *   AHB  = SYSCLK / 1 = 72 MHz
 *   APB1 = HCLK / 2   = 36 MHz (Max allowed on APB1)
 *   APB2 = HCLK / 1   = 72 MHz
 */
void SystemClock_Config_72MHz(void) {
    /* --------------------------------------------------------------------- */
    /* 1. Turn on HSE (High-Speed External 8 MHz Crystal)                    */
    /* --------------------------------------------------------------------- */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {
        /* Wait until HSE is stable and ready */
    }

    /* --------------------------------------------------------------------- */
    /* 2. Configure Flash Latency & Prefetch Buffer                          */
    /*    - 0 to 24 MHz: 0 wait states                                       */
    /*    - 24 to 48 MHz: 1 wait state                                       */
    /*    - 48 to 72 MHz: 2 wait states (Required for 72 MHz operation!)     */
    /* --------------------------------------------------------------------- */
    FLASH->ACR |= FLASH_ACR_PRFTBE;          /* Enable Flash prefetch buffer */
    FLASH->ACR &= ~FLASH_ACR_LATENCY;        /* Clear latency bits           */
    FLASH->ACR |= FLASH_ACR_LATENCY_2;       /* Set 2 wait states            */

    /* --------------------------------------------------------------------- */
    /* 3. Configure Bus Prescalers & PLL Multiplier in RCC_CFGR              */
    /*    - AHB Prescaler  = DIV1 (72 MHz)                                   */
    /*    - APB1 Prescaler = DIV2 (36 MHz - APB1 max limit is 36 MHz)        */
    /*    - APB2 Prescaler = DIV1 (72 MHz)                                   */
    /*    - PLL Source     = HSE (8 MHz)                                     */
    /*    - PLL Multiplier = x9  (8 MHz * 9 = 72 MHz)                        */
    /* --------------------------------------------------------------------- */
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 | 
                   RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);

    RCC->CFGR |= (RCC_CFGR_HPRE_DIV1  |      /* AHB  = SYSCLK / 1 = 72 MHz   */
                  RCC_CFGR_PPRE1_DIV2 |      /* APB1 = HCLK / 2   = 36 MHz   */
                  RCC_CFGR_PPRE2_DIV1 |      /* APB2 = HCLK / 1   = 72 MHz   */
                  RCC_CFGR_PLLSRC     |      /* PLL entry clock source = HSE */
                  RCC_CFGR_PLLMULL9);        /* PLL multiplication factor = 9*/

    /* --------------------------------------------------------------------- */
    /* 4. Enable PLL and wait for PLLRDY                                     */
    /* --------------------------------------------------------------------- */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {
        /* Wait until PLL locks */
    }

    /* --------------------------------------------------------------------- */
    /* 5. Select PLL as System Clock Source (SW = PLL)                       */
    /* --------------------------------------------------------------------- */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
        /* Wait until PLL is used as system clock */
    }

    /* --------------------------------------------------------------------- */
    /* 6. Update CMSIS global variable SystemCoreClock                       */
    /* --------------------------------------------------------------------- */
    SystemCoreClockUpdate();
}

/* Software delay loop */
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}

int main(void) {
    /* 1. Boost CPU clock from 8 MHz (HSI) to 72 MHz (HSE + PLL) */
    SystemClock_Config_72MHz();

    /* 2. Enable GPIOC clock */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* 3. Configure PC13 as Output Push-Pull 2 MHz */
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;

    /* 4. Blink PC13 */
    /* Because CPU clock is now 72 MHz (9x faster than 8 MHz), 
       a delay loop count of 3,600,000 provides ~0.5s period. */
    while (1) {
        GPIOC->ODR ^= GPIO_ODR_ODR13;
        delay(3600000);
    }

    return 0;
}

