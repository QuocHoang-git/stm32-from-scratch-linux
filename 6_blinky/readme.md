[⬅️ Back to Main README](../readme.md)

# Part 6: Bare-metal GPIO Blinky (No HAL, No CMSIS)

## 1. Overview
Welcome to the final milestone! In this tutorial, we bring everything together:
* Our C Startup Code ([Part 3](../3_startup-code/readme.md))
* Our Linker Script ([Part 4](../4_linker-script/readme.md))
* Our Makefile ([Part 5](../5_makefile/readme.md))

We will write our application code in `main.c` to blink the user LED on the STM32F103 Blue Pill board by writing directly to hardware peripheral registers—without importing STM32Cube HAL, Standard Peripheral Library, or even CMSIS header files.

---

## 2. Hardware Analysis: The Blue Pill LED

If we check the Blue Pill schematic (`docs/original-schematic-STM32F103C8T6-Blue_Pill.pdf`):
* The on-board green LED is connected to **Pin PC13** (Port C, Pin 13).
* The anode of the LED is connected to **3.3V** through a current-limiting resistor, and the cathode is connected to **PC13**.
* This means the LED is **Active-LOW**:
  * **PC13 = LOW (0V):** Current flows ➔ **LED ON**.
  * **PC13 = HIGH (3.3V):** No voltage difference ➔ **LED OFF**.

---

## 3. Register Mapping from Reference Manual (RM0008)

To control a GPIO pin on ARM microcontrollers, we must perform two steps:
1. **Enable the peripheral clock** for Port C via the Reset and Clock Control (RCC) block. (By default, peripherals are powered off to conserve energy).
2. **Configure the pin mode** (Input, Output, Speed) in the GPIO registers.

### 1. Enabling the Clock (RCC_APB2ENR)
* In RM0008 Section 7.3.7, the APB2 peripheral clock enable register (`RCC_APB2ENR`) is located at offset `0x18` from the RCC base address (`0x40021000`):
  * **Address:** `0x40021018`
  * **Bit 4 (`IOPCEN`):** IO Port C clock enable. Setting this bit to `1` powers on Port C.

```c
#define RCC_BASE            0x40021000UL
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x18UL))
#define RCC_APB2ENR_IOPCEN  (1UL << 4)

/* Enable clock */
RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;
```

---

### 2. Configuring PC13 (GPIOC_CRH)
Each GPIO port has two 32-bit configuration registers:
* **`CRL` (Port Configuration Register Low):** Configures Pins 0 through 7.
* **`CRH` (Port Configuration Register High):** Configures Pins 8 through 15.

Since we are using **Pin 13**, we configure `GPIOC_CRH` at offset `0x04` from the GPIOC base address (`0x40011000`):
* **Address:** `0x40011004`
* **Bits [23:20]** control Pin 13:
  * `MODE13[1:0]` (Bits 20, 21): Output mode, max speed 2 MHz = `10` binary.
  * `CNF13[1:0]`  (Bits 22, 23): General purpose output push-pull = `00` binary.
  * Combined nibble: `0b0010` (`0x2`).

```c
#define GPIOC_BASE          0x40011000UL
#define GPIOC_CRH           (*(volatile uint32_t *)(GPIOC_BASE + 0x04UL))

/* Clear bits 23:20, then set MODE13 to 0b0010 (Output 2 MHz, Push-Pull) */
GPIOC_CRH &= ~(0xFUL << 20);
GPIOC_CRH |=  (0x2UL << 20);
```

---

### 3. Toggling the Output (GPIOC_ODR)
The Output Data Register (`GPIOC_ODR`) at offset `0x0C` (`0x4001100C`) directly reflects the logical level of the pins. Toggling bit 13 toggles the LED:

```c
#define GPIOC_ODR           (*(volatile uint32_t *)(GPIOC_BASE + 0x0CUL))

/* Toggle bit 13 using XOR */
GPIOC_ODR ^= (1UL << 13);
```

---

## 4. Why use the `volatile` Keyword?

Notice how all register pointers are cast using `volatile uint32_t *`:
```c
(*(volatile uint32_t *)(0x40021018UL))
```

Without `volatile`, the GCC compiler's optimizer assumes the memory address doesn't change on its own or that consecutive writes to the same address are redundant, and may completely optimize away delay loops or register writes! 

`volatile` tells the compiler: *"Do not optimize or cache access to this memory location. Read and write directly to hardware every single time."*

---

## 5. How to Build & Flash

All files are provided in this directory:
* [`main.c`](main.c) - Bare-metal application code
* [`startup_stm32f103c8.c`](startup_stm32f103c8.c) - C Startup code
* [`stm32f103c8.ld`](stm32f103c8.ld) - Linker script
* [`Makefile`](Makefile) - Build & flash automation

### Step 1: Compile the Project
In your Linux terminal, navigate to this directory and run:

```bash
make
```

**Output:**
```text
  CC    startup_stm32f103c8.c
  CC    main.c
  LD    build/firmware.elf
  HEX   build/firmware.hex
  BIN   build/firmware.bin
================ Memory Consumption ================
   text	   data	    bss	    dec	    hex	filename
    472	      0	   1536	   2008	    7d8	build/firmware.elf
====================================================
```
> 💡 **Notice the size:** The entire binary takes **less than 500 bytes of Flash**! In contrast, a CubeIDE HAL project typically consumes 3 KB to 5 KB just to blink an LED.

### Step 2: Flash to the Board
Connect your ST-Link to your PC and the Blue Pill, then run:

```bash
make flash
```

OpenOCD will flash the firmware, verify the written bytes, reset the microcontroller, and the green LED on PC13 will begin blinking! 🎉

---

## 6. Conclusion
Congratulations! You have built a fully functional ARM Cortex-M embedded firmware from absolute scratch:
* Without heavy IDEs (Keil, CubeIDE).
* Without pre-compiled vendor libraries or HAL bloat.
* Completely automated on Linux using standard GNU tools (`gcc`, `ld`, `make`, `openocd`).

You now possess a true deep understanding of how microcontrollers boot, execute code, and communicate with hardware.

