/**
 ******************************************************************************
 * @file      main.c
 * @author    Duong Quoc Hoang
 * @brief     Part 11: High-Performance USART DMA (Zero-CPU Overhead).
 *            Demonstrates DMA1 Channel 4 (TX) & Circular DMA1 Channel 5 (RX)
 *            with IDLE Line Detection & Interactive Line Editing.
 ******************************************************************************
 */

#include "stm32f1xx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ========================================================================= */
/* System Timing & SysTick                                                   */
/* ========================================================================= */
static volatile uint32_t g_ms_ticks = 0;

void SysTick_Handler(void) {
    g_ms_ticks++;
}

uint32_t get_ticks(void) {
    return g_ms_ticks;
}

void delay_ms(uint32_t ms) {
    uint32_t start = g_ms_ticks;
    while ((g_ms_ticks - start) < ms) {
        __WFI();
    }
}

/* ========================================================================= */
/* DMA Buffers & CLI Line State                                              */
/* ========================================================================= */
#define TX_DMA_BUF_SIZE   512
#define RX_DMA_BUF_SIZE   128
#define CMD_LINE_BUF_SIZE 64

/* DMA RAM buffers */
static char tx_dma_buffer[TX_DMA_BUF_SIZE];
static char rx_dma_circular_buf[RX_DMA_BUF_SIZE];

/* Transmit busy flag */
static volatile uint8_t g_dma_tx_busy = 0;

/* Circular RX tracker */
static uint16_t g_rx_last_pos = 0;

/* Command line accumulator */
static char     g_cmd_line[CMD_LINE_BUF_SIZE];
static uint16_t g_cmd_len = 0;
static volatile uint8_t g_cmd_ready = 0;

/* ========================================================================= */
/* USART1 & DMA Hardware Transmit / SendChar                                 */
/* ========================================================================= */

/**
 * @brief Send a single character directly via UART (with DMA lock check).
 */
void USART1_SendChar(char c) {
    /* Wait if a DMA transmission is currently active */
    while (g_dma_tx_busy);
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = (uint16_t)c;
}

/**
 * @brief  Transmit a buffer over USART1 using DMA1 Channel 4.
 *         Returns immediately while DMA hardware transmits in background.
 */
void USART1_DMA_Transmit(const char *data, uint16_t length) {
    while (g_dma_tx_busy);

    if (length == 0) return;
    if (length > TX_DMA_BUF_SIZE) {
        length = TX_DMA_BUF_SIZE;
    }

    /* Wait until USART finishes transmitting any previous character (e.g. key echo) */
    while (!(USART1->SR & USART_SR_TC));

    g_dma_tx_busy = 1;

    /* Copy payload into dedicated TX DMA memory buffer */
    memcpy(tx_dma_buffer, data, length);

    /* Disable DMA1 Channel 4 to configure registers */
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;

    /* Set memory address and transfer count */
    DMA1_Channel4->CMAR  = (uint32_t)tx_dma_buffer;
    DMA1_Channel4->CNDTR = length;

    /* Clear any pending Transfer Complete flags for Channel 4 */
    DMA1->IFCR = DMA_IFCR_CTCIF4;

    /* Re-enable DMA1 Channel 4 to begin transmission */
    DMA1_Channel4->CCR |= DMA_CCR_EN;
}

/**
 * @brief  DMA1 Channel 4 Interrupt Handler (Triggered upon Transfer Complete).
 */
void DMA1_Channel4_IRQHandler(void) {
    if (DMA1->ISR & DMA_ISR_TCIF4) {
        DMA1->IFCR = DMA_IFCR_CTCIF4;
        DMA1_Channel4->CCR &= ~DMA_CCR_EN;
        g_dma_tx_busy = 0;
    }
}

/**
 * @brief Retarget standard C library _write() syscall via DMA transmission.
 */
int _write(int file, char *ptr, int len) {
    (void)file;
    USART1_DMA_Transmit(ptr, (uint16_t)len);
    return len;
}

/* ========================================================================= */
/* USART1 Interrupt Handler (Circular DMA RX + IDLE Line Detection)          */
/* ========================================================================= */

/**
 * @brief  USART1 Global Interrupt Handler.
 *         Fires when the line goes IDLE (user pauses or sends a burst).
 *         Extracts newly arrived characters from the circular DMA buffer,
 *         handles echo, backspace, and triggers on Enter.
 */
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_IDLE) {
        /* Clear IDLE flag: sequence is to read SR followed by DR */
        volatile uint32_t temp = USART1->SR;
        temp = USART1->DR;
        (void)temp;

        /* Calculate current position of DMA in the circular buffer */
        uint16_t current_pos = RX_DMA_BUF_SIZE - (uint16_t)(DMA1_Channel5->CNDTR);

        /* Process all newly received characters since last IDLE */
        static char last_char = 0;
        while (g_rx_last_pos != current_pos) {
            char c = rx_dma_circular_buf[g_rx_last_pos];
            g_rx_last_pos = (g_rx_last_pos + 1) % RX_DMA_BUF_SIZE;

            /* Ignore '\n' if it directly follows '\r' (CRLF sequence from terminal) */
            if (c == '\n' && last_char == '\r') {
                last_char = c;
                continue;
            }
            last_char = c;

            /* 1. Enter key (\r or standalone \n): Submit current command line */
            if (c == '\r' || c == '\n') {
                g_cmd_line[g_cmd_len] = '\0';
                g_cmd_ready = 1;
            }
            /* 2. Backspace (0x08 or 0x7F): Erase previous character */
            else if (c == 0x08 || c == 0x7F) {
                if (g_cmd_len > 0 && !g_cmd_ready) {
                    g_cmd_len--;
                    USART1_SendChar('\b');
                    USART1_SendChar(' ');
                    USART1_SendChar('\b');
                }
            }
            /* 3. Normal printable ASCII character */
            else if (c >= 32 && c <= 126) {
                if (g_cmd_len < (CMD_LINE_BUF_SIZE - 1) && !g_cmd_ready) {
                    g_cmd_line[g_cmd_len++] = c;
                    USART1_SendChar(c); /* Echo back to terminal */
                }
            }
        }
    }
}

/* ========================================================================= */
/* System Initializations                                                    */
/* ========================================================================= */

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
 * @brief Configure USART1 and DMA1 (Channel 4 = TX, Channel 5 = RX Circular).
 */
void USART1_DMA_Init(uint32_t baudrate) {
    /* 1. Enable Clocks for GPIOA, USART1 (APB2), and DMA1 (AHB) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
    RCC->AHBENR  |= RCC_AHBENR_DMA1EN;

    /* 2. Configure PA9 (TX: AF-PP 50MHz) and PA10 (RX: Input Floating) */
    GPIOA->CRH &= ~(0xFFUL << 4);
    GPIOA->CRH |=  (0x0BUL << 4) | (0x04UL << 8);

    /* 3. Configure Baud Rate */
    USART1->BRR = (SystemCoreClock + (baudrate / 2)) / baudrate;

    /* --------------------------------------------------------------------- */
    /* 4. Configure DMA1 Channel 4 for USART1_TX                             */
    /* --------------------------------------------------------------------- */
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel4->CPAR = (uint32_t)&(USART1->DR);
    DMA1_Channel4->CCR  = DMA_CCR_DIR         |  /* Memory to Peripheral       */
                          DMA_CCR_MINC        |  /* Increment memory address   */
                          DMA_CCR_TCIE        |  /* Transfer complete interrupt*/
                          DMA_CCR_PL_1;          /* Priority: High             */

    /* Set NVIC priorities: DMA TX completion priority higher than USART RX */
    NVIC_SetPriority(DMA1_Channel4_IRQn, 1);
    NVIC_SetPriority(USART1_IRQn, 2);

    NVIC_EnableIRQ(DMA1_Channel4_IRQn);

    /* --------------------------------------------------------------------- */
    /* 5. Configure DMA1 Channel 5 for USART1_RX in Circular Mode            */
    /* --------------------------------------------------------------------- */
    DMA1_Channel5->CCR &= ~DMA_CCR_EN;
    DMA1_Channel5->CPAR  = (uint32_t)&(USART1->DR);
    DMA1_Channel5->CMAR  = (uint32_t)rx_dma_circular_buf;
    DMA1_Channel5->CNDTR = RX_DMA_BUF_SIZE;
    DMA1_Channel5->CCR   = DMA_CCR_MINC       |  /* Increment memory address   */
                           DMA_CCR_CIRC       |  /* Circular buffer mode       */
                           DMA_CCR_PL_1;         /* Priority: High             */

    DMA1_Channel5->CCR |= DMA_CCR_EN;  /* Enable RX DMA channel continuously */

    /* --------------------------------------------------------------------- */
    /* 6. Enable USART1 DMA Transmitter (DMAT) and Receiver (DMAR)           */
    /*    Enable IDLE Line Interrupt (IDLEIE)                                */
    /* --------------------------------------------------------------------- */
    USART1->CR3 |= USART_CR3_DMAT | USART_CR3_DMAR;
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_IDLEIE | USART_CR1_UE;

    NVIC_EnableIRQ(USART1_IRQn);
}

/* ========================================================================= */
/* Command Processing                                                        */
/* ========================================================================= */
static uint8_t  g_led_auto_mode   = 1;
static uint32_t g_led_interval_ms = 500;

void Process_Command(char *cmd) {
    if (cmd == NULL || strlen(cmd) == 0) return;

    if (strcmp(cmd, "uptime") == 0) {
        printf("DMA-Uptime: %lu.%03lu seconds (%lu ms)\r\n", 
               g_ms_ticks / 1000, g_ms_ticks % 1000, g_ms_ticks);
    }
    else if (strncmp(cmd, "led auto ", 9) == 0) {
        int val = atoi(cmd + 9);
        if (val > 0) {
            g_led_interval_ms = (uint32_t)val;
            g_led_auto_mode = 1;
            printf("DMA-Shell: LED set to AUTO mode (%lu ms interval)\r\n", g_led_interval_ms);
        } else {
            printf("Error: Interval must be > 0 ms (e.g. 'led auto 1000')\r\n");
        }
    }
    else if (strcmp(cmd, "led manual") == 0) {
        g_led_auto_mode = 0;
        printf("DMA-Shell: LED switched to MANUAL mode\r\n");
    }
    else if (strcmp(cmd, "led on") == 0) {
        g_led_auto_mode = 0;
        GPIOC->ODR &= ~GPIO_ODR_ODR13;
        printf("DMA-Shell: LED turned ON\r\n");
    }
    else if (strcmp(cmd, "led off") == 0) {
        g_led_auto_mode = 0;
        GPIOC->ODR |= GPIO_ODR_ODR13;
        printf("DMA-Shell: LED turned OFF\r\n");
    }
    else if (strcmp(cmd, "help") == 0) {
        printf("=== STM32 DMA High-Speed Shell ===\r\n"
               "  uptime        - Print uptime (Calculated via SysTick)\r\n"
               "  led auto <ms> - Auto-blink LED every <ms> (Zero CPU load)\r\n"
               "  led manual    - Switch to manual mode\r\n"
               "  led on        - Turn LED ON\r\n"
               "  led off       - Turn LED OFF\r\n"
               "  help          - Display this command list\r\n");
    }
    else {
        printf("Unknown command: '%s'. Type 'help' for info.\r\n", cmd);
    }
}

/* ========================================================================= */
/* Main Application                                                          */
/* ========================================================================= */
int main(void) {
    /* 1. Disable stdout buffering in newlib-nano so printf without '\n' flushes immediately */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* 2. Boost CPU clock to 72 MHz */
    SystemClock_Config_72MHz();

    /* 3. Configure SysTick for 1 ms hardware ticks */
    SysTick_Config(SystemCoreClock / 1000);

    /* 4. Initialize USART1 + DMA1 (Ch4 TX, Ch5 RX Circular with IDLE interrupt) */
    USART1_DMA_Init(115200);

    /* 5. Configure PC13 LED */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;

    /* 6. Print banner using DMA printf! */
    printf("\r\n===================================================\r\n");
    printf("   STM32F103 High-Performance USART DMA Controller  \r\n");
    printf("===================================================\r\n");
    printf("Core Clock : %lu MHz\r\n", SystemCoreClock / 1000000UL);
    printf("TX Engine  : DMA1 Channel 4 (Memory -> USART1_DR)\r\n");
    printf("RX Engine  : DMA1 Channel 5 (Circular Buffer + IDLE)\r\n");
    printf("Type 'help' to test DMA command execution.\r\n\r\n");
    printf("dma-stm32> ");
    fflush(stdout);

    uint32_t last_led_toggle = 0;

    /* 7. Non-blocking Main Loop (Concurrent DMA Shell + Blinky) */
    while (1) {
        /* Task A: Non-blocking LED Auto-Blink */
        if (g_led_auto_mode) {
            if ((g_ms_ticks - last_led_toggle) >= g_led_interval_ms) {
                last_led_toggle = g_ms_ticks;
                GPIOC->ODR ^= GPIO_ODR_ODR13;
            }
        }

        /* Task B: Handle entire command line when Enter is pressed */
        if (g_cmd_ready) {
            printf("\r\n");
            if (g_cmd_len > 0) {
                Process_Command(g_cmd_line);
            }

            /* Reset line buffer for next command */
            g_cmd_len = 0;
            g_cmd_ready = 0;

            printf("dma-stm32> ");
            fflush(stdout);
        }

        /* Sleep CPU until next hardware event */
        __WFI();
    }

    return 0;
}
