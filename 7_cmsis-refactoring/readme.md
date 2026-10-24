[⬅️ Back to Main README](../readme.md)

# Part 7: Integrating Official ARM & ST CMSIS Headers

## 1. Overview
In [Part 6](../6_blinky/readme.md), we blinked the on-board LED by hardcoding raw register addresses:

```c
#define GPIOC_CRH (*(volatile uint32_t *)(0x40011004UL))
#define GPIOC_ODR (*(volatile uint32_t *)(0x4001100CUL))
```

In real-world production firmware, nobody manually defines thousands of registers. Instead, the industry relies on **CMSIS** (**Cortex Microcontroller Software Interface Standard**), officially defined by **ARM Ltd.** and implemented by **STMicroelectronics**.

In this tutorial, we integrate the **official CMSIS library** directly into our bare-metal Linux project—without installing STM32CubeIDE, CubeMX, or vendor bloatware.

---

## 2. The Two Layers of CMSIS

A complete STM32 CMSIS integration consists of two distinct packages:

```text
Drivers/CMSIS/
├── Core/Include/                  <-- Provided by ARM Ltd.
│   ├── core_cm3.h                 # Cortex-M3 processor core registers (NVIC, SysTick, SCB)
│   ├── cmsis_compiler.h           # Compiler abstraction (GCC, Clang, IAR, Keil)
│   ├── cmsis_gcc.h                # GCC-specific inline intrinsics (__NOP, __WFI, __disable_irq)
│   └── m-profile/                 # Architecture profile helpers
│
└── Device/ST/STM32F1xx/           <-- Provided by STMicroelectronics
    ├── Include/
    │   ├── stm32f1xx.h            # Master device selector
    │   ├── stm32f103xb.h          # Peripheral registers & bitmasks for STM32F103 Medium-Density
    │   └── system_stm32f1xx.h     # System clock initialization interface
    └── Source/
        └── system_stm32f1xx.c     # Standard SystemInit() function
```

1. **CMSIS-Core (ARM):** Standardizes CPU core registers across all ARM Cortex silicon vendors. Functions for configuring NVIC, SysTick, and core assembly instructions (`__NOP()`, `__WFI()`) are identical whether you use STM32, NXP, or Microchip.
2. **CMSIS-Device (ST):** Provides memory-mapped C structures and bitmask definitions for all on-chip peripherals specific to STM32F1 (RCC, GPIO, USART, SPI, I2C, Timers).

---

## 3. Where and How to Fetch Official CMSIS

Both packages are open-source and officially hosted on GitHub by STMicroelectronics and ARM:

| Component | Official GitHub Repository | What We Need |
| :--- | :--- | :--- |
| **CMSIS-Core** | [STMicroelectronics/cmsis_core](https://github.com/STMicroelectronics/cmsis_core) | `CMSIS/Core/Include/` |
| **CMSIS-Device-F1** | [STMicroelectronics/cmsis_device_f1](https://github.com/STMicroelectronics/cmsis_device_f1) | `Include/` and `Source/Templates/system_stm32f1xx.c` |

### Method 1: Fetch and Extract (Standalone Script)
You can pull only the required header files into your project using the following bash commands:

```bash
# 1. Create directory structure
mkdir -p Drivers/CMSIS/Core/Include
mkdir -p Drivers/CMSIS/Device/ST/STM32F1xx/Include
mkdir -p Drivers/CMSIS/Device/ST/STM32F1xx/Source

# 2. Fetch CMSIS-Core (ARM) headers
git clone --depth 1 https://github.com/STMicroelectronics/cmsis_core.git /tmp/cmsis_core
cp -r /tmp/cmsis_core/CMSIS/Core/Include/* Drivers/CMSIS/Core/Include/
rm -rf /tmp/cmsis_core

# 3. Fetch STM32F1 Device headers and system initialization source
git clone --depth 1 https://github.com/STMicroelectronics/cmsis_device_f1.git /tmp/cmsis_device_f1
cp /tmp/cmsis_device_f1/Include/* Drivers/CMSIS/Device/ST/STM32F1xx/Include/
cp /tmp/cmsis_device_f1/Source/Templates/system_stm32f1xx.c Drivers/CMSIS/Device/ST/STM32F1xx/Source/
rm -rf /tmp/cmsis_device_f1
```

*(Note: In this repository, these files are already pre-fetched and bundled inside `Drivers/CMSIS/` so you can build immediately).*

---

### Method 2: Using Git Submodules (Professional Practice)
If you manage your project with Git, you can link the upstream repositories directly as submodules so they can be versioned and updated independently:

```bash
git submodule add https://github.com/STMicroelectronics/cmsis_core.git Drivers/CMSIS/Core
git submodule add https://github.com/STMicroelectronics/cmsis_device_f1.git Drivers/CMSIS/Device/ST/STM32F1xx
```

---

## 4. How to Configure the Makefile for Shared CMSIS

Because CMSIS is located at the root of the repository (`../Drivers/CMSIS`), any lesson can easily reference it in its `Makefile`:

### 1. Target Chip Definition (`-DSTM32F103xB`)
`stm32f1xx.h` is a universal dispatcher header for the entire STM32F1 family. To select the register definitions for the STM32F103C8 (Medium-density device with 64KB/128KB Flash), we define `STM32F103xB`:

```makefile
C_DEFS = -DSTM32F103xB
```

### 2. Include Search Paths (`-I`)
We tell GCC where to locate the shared CMSIS headers:

```makefile
CMSIS_DIR = ../Drivers/CMSIS

C_INCLUDES = \
    -I$(CMSIS_DIR)/Core/CMSIS/Core/Include \
    -I$(CMSIS_DIR)/Device/ST/STM32F1xx/Include
```

### 3. Compile `system_stm32f1xx.c`
We add ST's official system initialization source file:

```makefile
C_SOURCES = \
    startup_stm32f103c8.c \
    $(CMSIS_DIR)/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c \
    main.c
```

---

## 5. Writing Clean Firmware with Official CMSIS

Once configured, our application code [`main.c`](main.c) becomes clean, portable, and readable:

```c
#include "stm32f1xx.h"

int main(void) {
    /* 1. Initialize chip clock configuration via CMSIS */
    SystemInit();

    /* 2. Enable GPIOC Clock via official CMSIS register macro */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* 3. Configure PC13 as Output Push-Pull (2 MHz) */
    GPIOC->CRH &= ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13);
    GPIOC->CRH |=  GPIO_CRH_MODE13_1;

    /* 4. Toggle PC13 using official bitmask */
    while (1) {
        GPIOC->ODR ^= GPIO_ODR_ODR13;
        delay(400000);
    }
}
```

### Notice the advantages over raw registers:
* **No magic numbers:** Instead of `0x40021018 |= (1 << 4)`, we write `RCC->APB2ENR |= RCC_APB2ENR_IOPCEN`.
* **Standard code completion & type checking:** IDEs and compilers understand `GPIO_TypeDef` and immediately catch typos like `GPIOC->CR` (which doesn't exist; it's `CRL` or `CRH`).
* **Zero runtime overhead:** CMSIS maps directly to physical memory addresses via pointer casting; it does not add any extra function calls or execution delays.

---

## 6. How to Build & Flash

All files, including the official CMSIS library headers, are ready in this directory:

```bash
# Build firmware with official CMSIS
make

# Flash to Blue Pill via OpenOCD
make flash
```

**Memory Usage:**
```text
================ Memory Consumption ================
   text	   data	    bss	    dec	    hex	filename
    484	      0	   1536	   2020	    7e4	build/firmware.elf
====================================================
```
> Even with the full official CMSIS library and `SystemInit()` included, our entire firmware binary remains **under 500 bytes of Flash**!

---

## 7. What's Next?
Now that we have the official CMSIS library integrated, we have access to all system registers and standard clock macros.

In **Part 8**, we will use CMSIS to configure the **STM32 Clock Tree & PLL**, taking advantage of the on-board 8 MHz crystal (HSE) to run our microcontroller at its maximum clock speed of **72 MHz**!

---

[➡️ Next: Part 8 - Clock Tree & PLL Configuration (72 MHz)](../8_clock-configuration/readme.md)
