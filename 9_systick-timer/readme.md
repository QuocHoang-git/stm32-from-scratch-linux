[⬅️ Back to Main README](../readme.md)

# Part 9: SysTick Timer & Hardware-Accurate 1ms Delay

## 1. Overview
In previous tutorials, we used an empty software loop to create delays:

```c
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm__("nop");
    }
}
```

While software loops work for basic testing, they suffer from fatal flaws in production firmware:
1. **Uncalibrated:** The actual delay varies wildly depending on compiler optimization flags (`-O0`, `-Os`, `-O3`) and CPU clock speed (8 MHz vs. 72 MHz).
2. **Wasteful Power Consumption:** The CPU burns 100% power spinning in a useless loop.
3. **Fragile:** Any higher-priority interrupt pauses the loop, making the delay unexpectedly longer.

In this tutorial, we will use the internal **ARM Cortex-M SysTick Timer** to create a rock-solid, hardware-accurate **1 millisecond timebase (`delay_ms()`)** that puts the CPU to sleep when idle.

---

## 2. What is the SysTick Timer?

Unlike peripheral timers (TIM1, TIM2, TIM3) which are designed by STMicroelectronics and vary between chip families, **SysTick is part of the ARM Cortex-M core itself**.

It is present on **every ARM Cortex-M microcontroller** (Cortex-M0, M3, M4, M7, M33) regardless of the silicon vendor (ST, NXP, Microchip, TI).

### Characteristics:
* **24-bit Down-Counter:** Counts down from a reload value to `0`, then automatically reloads and repeats.
* **Interrupt on Zero:** Fires a dedicated exception vector (`SysTick_Handler`) every time it hits zero.
* **Standard Operating System Tick:** Used by Real-Time Operating Systems (like FreeRTOS or Zephyr) as the system heartbeat for task scheduling.

---

## 3. The SysTick Registers

SysTick is mapped inside the ARM Private Peripheral Bus (**PPB**) space (`0xE000E010`):

| Register | Name | Description |
| :--- | :--- | :--- |
| `SysTick->CTRL` | Control & Status | Enables counter, enables interrupt (`TICKINT`), selects clock source (`CLKSOURCE`). |
| `SysTick->LOAD` | Reload Value | 24-bit value to reload when counter reaches `0`. |
| `SysTick->VAL`  | Current Value | Reading returns current count; writing any value resets it to `0`. |
| `SysTick->CALIB`| Calibration | Factory calibration value for 10ms reference. |

### Calculating the 1ms Reload Value:
At **72 MHz**, the processor clock ticks 72,000,000 times per second.
To trigger an interrupt every **1 millisecond** (1/1000th of a second):

$$\text{Ticks per ms} = \frac{72,000,000 \text{ cycles/sec}}{1000 \text{ ms/sec}} = 72,000 \text{ cycles}$$

Because the counter counts from $N-1$ down to $0$, the reload value is:
$$\text{LOAD} = 72,000 - 1 = 71,999$$

---

## 4. The CMSIS `SysTick_Config()` Function

ARM CMSIS Core provides a standard inline function in `core_cm3.h` that configures everything in a single line:

```c
SysTick_Config(SystemCoreClock / 1000);
```

What `SysTick_Config` does under the hood:
1. Validates that the reload value fits within 24 bits ($< 2^{24} = 16,777,216$).
2. Sets `SysTick->LOAD = (72000000 / 1000) - 1`.
3. Clears `SysTick->VAL = 0`.
4. Sets the SysTick interrupt priority in the NVIC.
5. Enables `SysTick->CTRL` with processor clock (`CLKSOURCE = 1`), interrupt enabled (`TICKINT = 1`), and counter enabled (`ENABLE = 1`).

---

## 5. Overriding the Weak `SysTick_Handler`

Recall from [Part 3](../3_startup-code/readme.md) that our C startup file defined:

```c
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));
```

Because it was declared as **`weak`**, we can simply implement our own version in [`main.c`](main.c). The linker will automatically bind our custom function into the vector table:

```c
static volatile uint32_t g_ms_ticks = 0;

void SysTick_Handler(void) {
    g_ms_ticks++; /* Increments every 1 ms */
}
```

---

## 6. Energy Efficient Delay with `__WFI()`

Instead of spinning in an empty `while` loop while waiting for milliseconds to elapse, we put the CPU to sleep using the ARM assembly instruction **`WFI` (Wait For Interrupt)**:

```c
void delay_ms(uint32_t ms) {
    uint32_t start = g_ms_ticks;
    while ((g_ms_ticks - start) < ms) {
        __WFI(); /* Enter Sleep Mode until the next 1ms SysTick interrupt */
    }
}
```

* When `__WFI()` executes, the Cortex-M3 core shuts down its execution pipeline and clock tree, dramatically reducing current consumption.
* Exactly 1 millisecond later, the SysTick interrupt triggers, waking up the core to evaluate the loop condition.
* This is standard practice in low-power battery-operated devices!

---

## 7. How to Build & Flash

All files are ready in this directory:
* [`main.c`](main.c) - 72 MHz clock setup, 1ms SysTick interrupt, and 1 Hz Blinky loop.
* [`Makefile`](Makefile) - Automated build and flash rule.

```bash
# Build firmware
make

# Flash to Blue Pill via OpenOCD
make flash
```

**Memory Usage:**
```text
================ Memory Consumption ================
   text	   data	    bss	    dec	    hex	filename
   1128	      4	   1540	   2672	    a70	build/firmware.elf
====================================================
```

Once flashed, the on-board LED on **PC13** will toggle with atomic precision: **exactly 500 ms ON, 500 ms OFF (1.000 Hz)**, regardless of compiler optimization settings!

---

## 8. What's Next?
Now that we have:
* A maximum 72 MHz clock tree ([Part 8](../8_clock-configuration/readme.md))
* A hardware-timed 1 ms timebase with low-power sleep ([Part 9](./readme.md))

The biggest missing piece for any embedded developer is **Serial Output / Logging**. 

In **Part 10**, we will build a bare-metal **USART1 Serial Driver** and retarget standard C library **`printf()`** so you can stream debug text directly to your Linux terminal over a USB-to-UART adapter!

---

[➡️ Next: Part 10 - USART Serial Driver & Retargeting printf()](../10_usart-printf/readme.md)

