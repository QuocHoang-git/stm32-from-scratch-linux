[⬅️ Back to Main README](../readme.md)

# Part 8: Clock Tree Configuration & PLL Setup (72 MHz)

## 1. Overview
When the STM32F103 powers up or resets, it boots by default using the **High-Speed Internal (HSI)** RC oscillator running at **8 MHz**. 

While 8 MHz is sufficient for a simple LED blinky, the microcontroller is capable of running at up to **72 MHz**—nine times faster. Furthermore, the internal HSI oscillator is sensitive to temperature and voltage fluctuations, whereas the on-board metal can crystal (**HSE - High-Speed External oscillator**) provides a rock-solid 8.000 MHz reference clock.

In this tutorial, we will configure the **STM32F103 Clock Tree**, configure the **Phase-Locked Loop (PLL)**, and run our CPU at its full rated speed of **72 MHz**.

---

## 2. The STM32F103 Clock Tree Architecture

The clock system is managed by the **Reset and Clock Control (RCC)** block:

```text
 [ HSE Crystal: 8 MHz ]
           │
           ▼
    [ PLL Source ] ──► [ PLL Multiplier: x9 ] ──► [ PLLCLK: 72 MHz ]
                                                          │
                                                          ▼ (SW: Switch to PLL)
                                                  [ SYSCLK: 72 MHz ]
                                                          │
                    ┌─────────────────────────────────────┴────────────────────┐
                    ▼ (AHB Prescaler: /1)                                      │
             [ HCLK: 72 MHz ] (CPU Core, Memory, DMA)                         │
                    │                                                          │
         ┌──────────┴──────────────────────────┐                               │
         ▼ (APB1 Prescaler: /2)                ▼ (APB2 Prescaler: /1)          │
  [ PCLK1: 36 MHz (MAX!) ]             [ PCLK2: 72 MHz ]                       │
  (TIM2-4, USART2, I2C, SPI2)          (GPIOA-E, USART1, ADC, SPI1)            ▼
                                                                        [ USB: 48 MHz ]
```

### Key Bus Limits (Reference Manual RM0008):
* **SYSCLK (System Clock):** Maximum **72 MHz**.
* **HCLK (AHB Bus):** Supplies the Cortex-M3 core, SRAM, Flash, and DMA. Max **72 MHz**.
* **PCLK1 (APB1 Bus - Low Speed):** **Max 36 MHz**. Exceeding 36 MHz on APB1 will violate hardware specifications! Therefore, the APB1 prescaler **must** be set to divide-by-2 (`/2`).
* **PCLK2 (APB2 Bus - High Speed):** Supplies GPIO ports, USART1, and ADC. Max **72 MHz**.

---

## 3. The Critical Role of Flash Access Latency (Wait States)

Why can't we immediately switch the CPU to 72 MHz?

Because **embedded Flash memory is slow compared to the CPU core**. At room temperature, the STM32 Flash interface can only be read at speeds up to 24 MHz with zero wait states.

If the CPU attempts to fetch instructions at 72 MHz with 0 wait states, the Flash memory will return corrupted data, immediately triggering a **HardFault exception** and crashing the chip!

The **Flash Access Control Register (`FLASH_ACR`)** allows us to insert wait states:

| CPU Clock (HCLK) | Required Flash Latency | CMSIS Macro |
| :---: | :---: | :--- |
| **0 < HCLK ≤ 24 MHz** | 0 Wait States | `FLASH_ACR_LATENCY_0` |
| **24 < HCLK ≤ 48 MHz** | 1 Wait State | `FLASH_ACR_LATENCY_1` |
| **48 < HCLK ≤ 72 MHz** | **2 Wait States** | `FLASH_ACR_LATENCY_2` |

> ⚠️ **Golden Rule:** **Always configure Flash Latency BEFORE increasing the CPU clock speed!**

---

## 4. The 6-Step Clock Switching Sequence

Here is the robust, step-by-step sequence implemented in [`main.c`](main.c):

```c
void SystemClock_Config_72MHz(void) {
    /* 1. Turn on HSE (8 MHz Crystal) and wait until stable */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    /* 2. Enable Flash Prefetch Buffer and set 2 Wait States for 72 MHz */
    FLASH->ACR |= FLASH_ACR_PRFTBE;
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_2;

    /* 3. Configure Bus Prescalers & PLL Multiplier:
     *    AHB  = /1 (72 MHz)
     *    APB1 = /2 (36 MHz - Max limit!)
     *    APB2 = /1 (72 MHz)
     *    PLL  = HSE * 9 = 8 MHz * 9 = 72 MHz
     */
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 | 
                   RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL);

    RCC->CFGR |= (RCC_CFGR_HPRE_DIV1  |
                  RCC_CFGR_PPRE1_DIV2 |
                  RCC_CFGR_PPRE2_DIV1 |
                  RCC_CFGR_PLLSRC     |
                  RCC_CFGR_PLLMULL9);

    /* 4. Enable PLL and wait until locked */
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    /* 5. Switch System Clock source to PLL */
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    /* 6. Update CMSIS core clock variable (SystemCoreClock = 72000000) */
    SystemCoreClockUpdate();
}
```

---

## 5. Visual Proof: 8 MHz vs. 72 MHz

How can we prove that our chip is indeed running 9 times faster without an oscilloscope?

* In [Part 7](../7_cmsis-refactoring/readme.md) (at 8 MHz HSI), our delay loop was `delay(400000)`, giving a toggle period of roughly **~0.3 seconds**.
* At 72 MHz, the CPU executes 9 instructions in the time it previously took to execute 1. If we kept `delay(400000)`, the LED would flash so fast it appears almost continuous.
* To achieve an equivalent human-visible ~0.5 second delay at 72 MHz, we increase the loop count by 9x:
  ```c
  delay(3600000); /* 400,000 * 9 = 3,600,000 cycles */
  ```

---

## 6. How to Build & Flash

All files are ready in this directory:
* [`main.c`](main.c) - 72 MHz PLL clock setup and blinky loop
* [`Makefile`](Makefile) - Links to shared `Drivers/CMSIS` submodules

```bash
# Build firmware
make

# Flash via OpenOCD & ST-Link
make flash
```

**Memory Usage:**
```text
================ Memory Consumption ================
   text	   data	    bss	    dec	    hex	filename
    896	      4	   1536	   2436	    984	build/firmware.elf
====================================================
```

---

## 7. What's Next?
Our STM32 is now running at its maximum speed of 72 MHz! However, our delay function is still an imprecise CPU-burning loop (`while(count--)`).

In **Part 9**, we will use the ARM Cortex-M3 internal **SysTick Timer** to create an exact, hardware-timed **1 millisecond delay (`delay_ms()`)** using hardware interrupts!

---

[➡️ Next: Part 9 - SysTick Timer & Precise 1ms Delay](../9_systick-timer/readme.md)

