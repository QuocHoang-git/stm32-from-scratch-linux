/**
 ******************************************************************************
 * @file      startup_stm32f103c8.c
 * @author    Duong Quoc Hoang
 * @brief     Minimal bare-metal startup code for STM32F103C8 (ARM Cortex-M3).
 *            Written entirely in C without assembly dependencies.
 ******************************************************************************
 */

#include <stdint.h>

/* Symbols exported by the Linker Script */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* Prototype of application entry point */
extern int main(void);

/* Prototype of Reset Handler */
void Reset_Handler(void);

/* Default fallback handler for unhandled exceptions */
void Default_Handler(void) {
    while (1) {
        /* Hang here for debugging if an unhandled interrupt occurs */
    }
}

/* Cortex-M3 Core System Exceptions (Weak Aliases) */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVCall_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* STM32F103 Peripheral Interrupts (Weak Aliases) */
void WWDG_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler(void)                __attribute__((weak, alias("Default_Handler")));
void TAMPER_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler(void)                __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void)                __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void)              __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel1_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel2_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel3_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel4_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel5_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel6_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel7_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void ADC1_2_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void USB_HP_CAN1_TX_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void USB_LP_CAN1_RX0_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));
void CAN1_RX1_IRQHandler(void)           __attribute__((weak, alias("Default_Handler")));
void CAN1_SCE_IRQHandler(void)           __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_IRQHandler(void)           __attribute__((weak, alias("Default_Handler")));
void TIM1_UP_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_COM_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler(void)            __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void)               __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void USART3_IRQHandler(void)             __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void RTC_Alarm_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void USBWakeUp_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));

/* Vector Table placed into ".isr_vector" section */
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    (uint32_t)&_estack,                     /* 0x000: Initial Stack Pointer */
    (uint32_t)Reset_Handler,                /* 0x004: Reset Handler */
    (uint32_t)NMI_Handler,                  /* 0x008: NMI Handler */
    (uint32_t)HardFault_Handler,            /* 0x00C: Hard Fault Handler */
    (uint32_t)MemManage_Handler,            /* 0x010: MPU Fault Handler */
    (uint32_t)BusFault_Handler,             /* 0x014: Bus Fault Handler */
    (uint32_t)UsageFault_Handler,           /* 0x018: Usage Fault Handler */
    0, 0, 0, 0,                             /* 0x01C - 0x028: Reserved */
    (uint32_t)SVCall_Handler,               /* 0x02C: SVCall Handler */
    (uint32_t)DebugMon_Handler,             /* 0x030: Debug Monitor Handler */
    0,                                      /* 0x034: Reserved */
    (uint32_t)PendSV_Handler,               /* 0x038: PendSV Handler */
    (uint32_t)SysTick_Handler,              /* 0x03C: SysTick Handler */
    
    /* External Interrupts (STM32F103 specific) */
    (uint32_t)WWDG_IRQHandler,              /* Window Watchdog */
    (uint32_t)PVD_IRQHandler,               /* PVD through EXTI Line detect */
    (uint32_t)TAMPER_IRQHandler,            /* Tamper */
    (uint32_t)RTC_IRQHandler,               /* RTC */
    (uint32_t)FLASH_IRQHandler,             /* Flash */
    (uint32_t)RCC_IRQHandler,               /* RCC */
    (uint32_t)EXTI0_IRQHandler,             /* EXTI Line 0 */
    (uint32_t)EXTI1_IRQHandler,             /* EXTI Line 1 */
    (uint32_t)EXTI2_IRQHandler,             /* EXTI Line 2 */
    (uint32_t)EXTI3_IRQHandler,             /* EXTI Line 3 */
    (uint32_t)EXTI4_IRQHandler,             /* EXTI Line 4 */
    (uint32_t)DMA1_Channel1_IRQHandler,     /* DMA1 Channel 1 */
    (uint32_t)DMA1_Channel2_IRQHandler,     /* DMA1 Channel 2 */
    (uint32_t)DMA1_Channel3_IRQHandler,     /* DMA1 Channel 3 */
    (uint32_t)DMA1_Channel4_IRQHandler,     /* DMA1 Channel 4 */
    (uint32_t)DMA1_Channel5_IRQHandler,     /* DMA1 Channel 5 */
    (uint32_t)DMA1_Channel6_IRQHandler,     /* DMA1 Channel 6 */
    (uint32_t)DMA1_Channel7_IRQHandler,     /* DMA1 Channel 7 */
    (uint32_t)ADC1_2_IRQHandler,            /* ADC1 and ADC2 */
    (uint32_t)USB_HP_CAN1_TX_IRQHandler,    /* USB High Priority or CAN1 TX */
    (uint32_t)USB_LP_CAN1_RX0_IRQHandler,   /* USB Low Priority or CAN1 RX0 */
    (uint32_t)CAN1_RX1_IRQHandler,          /* CAN1 RX1 */
    (uint32_t)CAN1_SCE_IRQHandler,          /* CAN1 SCE */
    (uint32_t)EXTI9_5_IRQHandler,           /* EXTI Line 9..5 */
    (uint32_t)TIM1_BRK_IRQHandler,          /* TIM1 Break */
    (uint32_t)TIM1_UP_IRQHandler,           /* TIM1 Update */
    (uint32_t)TIM1_TRG_COM_IRQHandler,      /* TIM1 Trigger and Commutation */
    (uint32_t)TIM1_CC_IRQHandler,           /* TIM1 Capture Compare */
    (uint32_t)TIM2_IRQHandler,              /* TIM2 */
    (uint32_t)TIM3_IRQHandler,              /* TIM3 */
    (uint32_t)TIM4_IRQHandler,              /* TIM4 */
    (uint32_t)I2C1_EV_IRQHandler,           /* I2C1 Event */
    (uint32_t)I2C1_ER_IRQHandler,           /* I2C1 Error */
    (uint32_t)I2C2_EV_IRQHandler,           /* I2C2 Event */
    (uint32_t)I2C2_ER_IRQHandler,           /* I2C2 Error */
    (uint32_t)SPI1_IRQHandler,              /* SPI1 */
    (uint32_t)SPI2_IRQHandler,              /* SPI2 */
    (uint32_t)USART1_IRQHandler,            /* USART1 */
    (uint32_t)USART2_IRQHandler,            /* USART2 */
    (uint32_t)USART3_IRQHandler,            /* USART3 */
    (uint32_t)EXTI15_10_IRQHandler,         /* EXTI Line 15..10 */
    (uint32_t)RTC_Alarm_IRQHandler,         /* RTC Alarm through EXTI Line */
    (uint32_t)USBWakeUp_IRQHandler          /* USB Wakeup from suspend */
};

/**
 * @brief  Entry point executed upon hardware reset.
 */
void Reset_Handler(void) {
    /* 1. Copy initialized data (.data section) from FLASH (LMA) to SRAM (VMA) */
    uint32_t *pSrc = &_sidata;
    uint32_t *pDest = &_sdata;
    while (pDest < &_edata) {
        *pDest++ = *pSrc++;
    }

    /* 2. Zero-fill the uninitialized data (.bss section) in SRAM */
    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0;
    }

    /* 3. Call the application entry point */
    main();

    /* 4. If main() exits, trap in an infinite loop */
    while (1) {
    }
}

