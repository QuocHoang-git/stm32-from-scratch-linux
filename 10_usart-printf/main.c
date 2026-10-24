/**
 ******************************************************************************
 * @file      main.c
 * @author    Duong Quoc Hoang
 * @brief     Part 10: Interactive UART CLI (Full-Duplex RX/TX with Interrupts).
 *            Supports commands: 'uptime', 'led auto <ms>', 'led manual',
 *            'led on', 'led off', and 'help'.
 ******************************************************************************
 */

#include "stm32f1xx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ========================================================================= */
/* System Tick & Timekeeping                                                 */
/* ========================================================================= */
static volatile uint32_t g_ms_ticks = 0;

void SysTick_Handler(void) {
    g_ms_ticks++;
}

uint32_t get_ticks(void) {
    return g_ms_ticks;
}

/* ========================================================================= */
/* Command Buffer & RX Interrupt                                             */
/* ========================================================================= */
#define CMD_BUFFER_SIZE  64

static char rx_line_buffer[CMD_BUFFER_SIZE];
static volatile uint8_t rx_index = 0;
static volatile uint8_t rx_line_ready = 0;

/**
 * @brief Send a single character over USART1.
 */
void USART1_SendChar(char c) {
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = (uint16_t)c;
}

/**
 * @brief  USART1 Global Interrupt Handler.
 *         Triggered when a new byte arrives (RXNE flag = 1).
 */
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {
        char c = (char)(USART1->DR & 0xFF);

        /* Handle Enter key (Carriage Return / Line Feed) */
        if (c == '\r' || c == '\n') {
            if (!rx_line_ready && rx_index > 0) {
                rx_line_buffer[rx_index] = '\0';
                rx_line_ready = 1;
            }
            USART1_SendChar('\r');
            USART1_SendChar('\n');
        }
        /* Handle Backspace (ASCII 0x08 or 0x7F) */
        else if (c == 0x08 || c == 0x7F) {
            if (rx_index > 0) {
                rx_index--;
                /* Erase character visually on terminal: back, space, back */
                USART1_SendChar('\b');
                USART1_SendChar(' ');
                USART1_SendChar('\b');
            }
        }
        /* Printable characters */
        else if (rx_index < (CMD_BUFFER_SIZE - 1) && !rx_line_ready) {
            rx_line_buffer[rx_index++] = c;
            USART1_SendChar(c); /* Echo back to terminal */
        }
    }
}

/**
 * @brief Retarget standard C library _write() syscall to USART1.
 */
int _write(int file, char *ptr, int len) {
    (void)file;
    for (int i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            USART1_SendChar('\r');
        }
        USART1_SendChar(ptr[i]);
    }
    return len;
}

/**
 * @brief Configure System Clock to 72 MHz.
 */
void SystemClock_Config_72MHz(void) {
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    FLASH->ACR |= FLASH_ACR_PRFTBE;
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 | 
                   RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);

    RCC->CFGR |= (RCC_CFGR_HPRE_DIV1  |
                  RCC_CFGR_PPRE1_DIV2 |
                  RCC_CFGR_PPRE2_DIV1 |
                  RCC_CFGR_PLLSRC     |
                  RCC_CFGR_PLLMULL9);

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    SystemCoreClockUpdate();
}

/**
 * @brief Configure USART1 at 115200 Baud with RX Interrupt enabled.
 *        PA9  = TX (Alternate Function Push-Pull 50 MHz)
 *        PA10 = RX (Input Floating)
 */
void USART1_Init(uint32_t baudrate) {
    /* 1. Enable Clocks for GPIOA and USART1 (APB2) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* 2. Configure PA9 (TX) as Alternate Function Push-Pull 50 MHz (0xB) */
    GPIOA->CRH &= ~(0xFUL << 4);
    GPIOA->CRH |=  (0xBUL << 4);

    /* 3. Configure PA10 (RX) as Input Floating (0x4) */
    GPIOA->CRH &= ~(0xFUL << 8);
    GPIOA->CRH |=  (0x4UL << 8);

    /* 4. Configure Baud Rate */
    USART1->BRR = (SystemCoreClock + (baudrate / 2)) / baudrate;

    /* 5. Enable TX, RX, and RX Not Empty Interrupt (RXNEIE) */
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    /* 6. Enable USART1 IRQ in ARM Core NVIC */
    NVIC_EnableIRQ(USART1_IRQn);
}

/* ========================================================================= */
/* Command Processing & Execution                                            */
/* ========================================================================= */
static uint8_t  g_led_auto_mode  = 1;     /* 1 = Auto blinking, 0 = Manual */
static uint32_t g_led_interval_ms = 500;  /* Default auto interval: 500ms */

void Process_Command(char *cmd) {
    /* Trim trailing carriage return / whitespace */
    size_t len = strlen(cmd);
    while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '\t')) {
        cmd[--len] = '\0';
    }

    if (len == 0) {
        return;
    }

    /* 1. Command: 'uptime' */
    if (strcmp(cmd, "uptime") == 0) {
        uint32_t sec = g_ms_ticks / 1000;
        uint32_t ms  = g_ms_ticks % 1000;
        printf("Uptime: %lu.%03lu seconds (%lu total ms)\r\n", sec, ms, g_ms_ticks);
    }
    /* 2. Command: 'led auto <ms>' */
    else if (strncmp(cmd, "led auto ", 9) == 0) {
        int val = atoi(cmd + 9);
        if (val > 0) {
            g_led_interval_ms = (uint32_t)val;
            g_led_auto_mode = 1;
            printf("LED mode: AUTO (toggling every %lu ms)\r\n", g_led_interval_ms);
        } else {
            printf("Error: Interval must be greater than 0 ms (e.g. 'led auto 1000')\r\n");
        }
    }
    /* 3. Command: 'led manual' */
    else if (strcmp(cmd, "led manual") == 0) {
        g_led_auto_mode = 0;
        printf("LED mode: MANUAL (auto blink disabled. Use 'led on' or 'led off')\r\n");
    }
    /* 4. Command: 'led on' (Active-LOW: 0 = ON) */
    else if (strcmp(cmd, "led on") == 0) {
        g_led_auto_mode = 0;
        GPIOC->ODR &= ~GPIO_ODR_ODR13; /* Pull PC13 LOW -> LED ON */
        printf("LED turned ON (Manual mode)\r\n");
    }
    /* 5. Command: 'led off' (Active-LOW: 1 = OFF) */
    else if (strcmp(cmd, "led off") == 0) {
        g_led_auto_mode = 0;
        GPIOC->ODR |= GPIO_ODR_ODR13;  /* Pull PC13 HIGH -> LED OFF */
        printf("LED turned OFF (Manual mode)\r\n");
    }
    /* 6. Command: 'help' */
    else if (strcmp(cmd, "help") == 0) {
        printf("\r\nAvailable Commands:\r\n");
        printf("  uptime          - Print system uptime in seconds and milliseconds\r\n");
        printf("  led auto <ms>   - Enable automatic blinking with specified period (e.g. 'led auto 1000')\r\n");
        printf("  led manual      - Switch to manual LED control (pauses auto blink)\r\n");
        printf("  led on          - Turn on PC13 LED (switches to manual)\r\n");
        printf("  led off         - Turn off PC13 LED (switches to manual)\r\n");
        printf("  help            - Display this help message\r\n\r\n");
    }
    /* Unknown command */
    else {
        printf("Unknown command: '%s'. Type 'help' for available commands.\r\n", cmd);
    }
}

/* ========================================================================= */
/* Main Application                                                          */
/* ========================================================================= */
int main(void) {
    /* 1. Disable stdout buffering in newlib-nano so printf without '\n' flushes immediately */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* 1. Boost CPU clock to 72 MHz */
    SystemClock_Config_72MHz();

    /* 2. Configure SysTick for 1 ms hardware ticks */
    SysTick_Config(SystemCoreClock / 1000);

    /* 3. Initialize USART1 at 115200 bps with RX Interrupt */
    USART1_Init(115200);

    /* 4. Configure PC13 LED */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;

    /* 5. Print interactive boot banner */
    printf("\r\n==============================================\r\n");
    printf("   STM32 Bare-Metal Interactive UART Shell    \r\n");
    printf("==============================================\r\n");
    printf("Core Clock : %lu MHz\r\n", SystemCoreClock / 1000000UL);
    printf("Baud Rate  : 115200 bps (8N1)\r\n");
    printf("Type 'help' to view available commands.\r\n\r\n");
    printf("stm32> ");
    fflush(stdout);

    uint32_t last_led_toggle = 0;

    /* 6. Non-blocking Main Loop (Concurrent CLI + Blinky) */
    while (1) {
        /* Task A: Non-blocking LED Auto-Blink */
        if (g_led_auto_mode) {
            if ((g_ms_ticks - last_led_toggle) >= g_led_interval_ms) {
                last_led_toggle = g_ms_ticks;
                GPIOC->ODR ^= GPIO_ODR_ODR13;
            }
        }

        /* Task B: Handle incoming command line when Enter is pressed */
        if (rx_line_ready) {
            Process_Command(rx_line_buffer);

            /* Reset buffer for next command */
            rx_index = 0;
            rx_line_ready = 0;

            /* Print prompt */
            printf("stm32> ");
            fflush(stdout);
        }

        /* Sleep CPU until next interrupt (SysTick 1ms or UART RX) */
        __WFI();
    }

    return 0;
}
