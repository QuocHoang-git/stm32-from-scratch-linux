/**
 ******************************************************************************
 * @file      main.c
 * @author    Duong Quoc Hoang
 * @brief     Part 9: SysTick Timer & Hardware-Accurate 1ms Delay.
 *            Replaces software spin-loops with ARM Cortex-M hardware ticks.
 ******************************************************************************
 */

#include "stm32f1xx.h"

/* Global millisecond tick counter */
static volatile uint32_t g_ms_ticks = 0;

/**
 * @brief  SysTick Exception Handler.
 *         Overrides the weak alias defined in startup_stm32f103c8.c.
 *         Called every 1 ms automatically by hardware.
 */
void SysTick_Handler(void) {
    g_ms_ticks++;
}

/**
 * @brief  Returns the number of milliseconds since system boot.
 */
uint32_t get_ticks(void) {
    return g_ms_ticks;
}

/**
 * @brief  Hardware-accurate delay in milliseconds.
 * @param  ms: Duration in milliseconds to wait.
 */
void delay_ms(uint32_t ms) {
    uint32_t start = g_ms_ticks;
    while ((g_ms_ticks - start) < ms) {
        /* Put CPU into low-power sleep until the next interrupt arrives */
        __WFI();
    }
}

/**
 * @brief  Configure System Clock to 72 MHz (HSE 8 MHz * 9).
 */
void SystemClock_Config_72MHz(void) {
    /* 1. Turn on HSE and wait */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* 2. Flash 2 wait states & prefetch */
    FLASH->ACR |= FLASH_ACR_PRFTBE;
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    /* 3. Bus prescalers & PLL */
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 | 
                   RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);

    RCC->CFGR |= (RCC_CFGR_HPRE_DIV1  |
                  RCC_CFGR_PPRE1_DIV2 |
                  RCC_CFGR_PPRE2_DIV1 |
                  RCC_CFGR_PLLSRC     |
                  RCC_CFGR_PLLMULL9);

    /* 4. Turn on PLL and wait */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* 5. Switch to PLL */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    /* 6. Update CMSIS SystemCoreClock variable */
    SystemCoreClockUpdate();
}

/**
 * @brief  Initialize SysTick Timer to generate an interrupt every 1 ms.
 */
void SysTick_Init_1ms(void) {
    /* SystemCoreClock is 72,000,000. 
       Reload value = (72,000,000 / 1000) = 72,000 clock cycles per 1 ms. */
    SysTick_Config(SystemCoreClock / 1000);
}

int main(void) {
    /* 1. Boost CPU clock to 72 MHz */
    SystemClock_Config_72MHz();

    /* 2. Configure SysTick for 1 ms hardware ticks */
    SysTick_Init_1ms();

    /* 3. Enable Port C Clock */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* 4. Configure PC13 as Output Push-Pull 2 MHz */
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;

    /* 5. Blink LED PC13: Exactly 500 ms ON, 500 ms OFF (1 Hz) */
    while (1) {
        GPIOC->ODR ^= GPIO_ODR_ODR13;
        delay_ms(500);
    }

    return 0;
}

