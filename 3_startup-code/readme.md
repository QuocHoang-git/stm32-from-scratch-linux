[⬅️ Back to Main README](../readme.md)

# Part 3: Writing the Startup Code from Scratch (in C)

## 1. Overview
In [Part 2](../2_memory-and-vector-table/readme.md), we learned that when the STM32 powers up, the hardware loads the Stack Pointer from `0x00000000` and immediately jumps to the address at `0x00000004` (`Reset_Handler`).

Before `main()` can be safely executed, the microcontroller must:
1. **Initialize the `.data` section:** Copy initialized global and static variables from Flash (where they persist when powered off) into SRAM (where they can be modified during runtime).
2. **Zero-initialize the `.bss` section:** Clear uninitialized global and static variables in SRAM to zero (as mandated by the C standard).
3. **Call `main()`**.

In this tutorial, we will write our own **Startup Code entirely in C** (`startup_stm32f103c8.c`) without relying on proprietary vendor code or assembly files.

---

## 2. Why Write Startup Code in C?
Traditionally, startup files (like ST's `startup_stm32f103xb.s`) were written in ARM assembly. However, writing the startup file in C provides several advantages:
* **Readability:** Much easier to understand and maintain than assembly instructions.
* **Portability:** Easier to adapt across different Cortex-M microcontrollers.
* **Type Safety:** Uses standard C function pointers and types.

GCC provides compiler attributes that make this possible:
* `__attribute__((section(".isr_vector")))`: Instructs the linker to place the vector table into a custom output section named `.isr_vector` (which we will locate at Flash address `0x08000000` in Part 4).
* `__attribute__((weak, alias("Default_Handler")))`: Marks an interrupt handler as "weak". If the user doesn't write a custom handler in their application, the CPU falls back to `Default_Handler`. If the user defines `void SysTick_Handler(void)` in their code, the linker automatically binds it instead!

---

## 3. Linker Symbols Explained
To copy `.data` and zero out `.bss`, our C startup code needs to know where these sections start and end in memory. 

These boundaries are defined in the Linker Script (which we will cover in Part 4) and exported as external symbols:

| Linker Symbol | Meaning |
| :--- | :--- |
| `_estack` | End of SRAM / Initial Stack Pointer (`0x20005000`) |
| `_sidata` | Start of `.data` initial values in Flash (LMA) |
| `_sdata` | Start address of `.data` section in SRAM (VMA) |
| `_edata` | End address of `.data` section in SRAM (VMA) |
| `_sbss` | Start address of `.bss` section in SRAM (VMA) |
| `_ebss` | End address of `.bss` section in SRAM (VMA) |

In C, we declare them as external variables:
```c
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
```

---

## 4. Step-by-Step Implementation

### Step 1: The Reset Handler
Here is what `Reset_Handler()` does before calling `main()`:

```c
void Reset_Handler(void) {
    // 1. Copy .data from FLASH (LMA) to SRAM (VMA)
    uint32_t *pSrc = &_sidata;
    uint32_t *pDest = &_sdata;
    while (pDest < &_edata) {
        *pDest++ = *pSrc++;
    }

    // 2. Zero-initialize the .bss section in SRAM
    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0;
    }

    // 3. Call main()
    main();

    // 4. Trap in infinite loop if main() ever exits
    while (1);
}
```

### Step 2: The Default Handler
If an unexpected interrupt or fault occurs, we catch it in an infinite loop for debugging:

```c
void Default_Handler(void) {
    while (1);
}
```

### Step 3: Weak Aliasing of Interrupt Handlers
Using GCC function attributes, we define all ISRs as weak aliases pointing to `Default_Handler`:

```c
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVCall_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));
// ... and peripheral interrupts (EXTI, TIM, USART, etc.)
```

### Step 4: The Vector Table Array
Finally, we construct the table as an array of 32-bit values placed into the `.isr_vector` section:

```c
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    (uint32_t)&_estack,         // 0: Initial Stack Pointer
    (uint32_t)Reset_Handler,     // 1: Reset Handler
    (uint32_t)NMI_Handler,       // 2: NMI
    (uint32_t)HardFault_Handler, // 3: HardFault
    (uint32_t)MemManage_Handler, // 4: MemManage
    (uint32_t)BusFault_Handler,  // 5: BusFault
    (uint32_t)UsageFault_Handler,// 6: UsageFault
    0, 0, 0, 0,                  // 7-10: Reserved
    (uint32_t)SVCall_Handler,    // 11: SVCall
    0,                           // 12: DebugMonitor
    0,                           // 13: Reserved
    (uint32_t)PendSV_Handler,    // 14: PendSV
    (uint32_t)SysTick_Handler,   // 15: SysTick
    // Peripheral IRQs follow...
};
```

---

## 5. Complete Source Code
You can find the full, production-ready startup file right inside this directory:
📁 [`startup_stm32f103c8.c`](startup_stm32f103c8.c)

> **💡 Pro Tip (C vs. C++ Initialization):**  
> In pure C bare-metal development, calling `main()` directly after `.data` and `.bss` initialization is optimal and clean. However, if you plan to write C++ code in the future, you must invoke `__libc_init_array()` right before `main()` so that static and global C++ object constructors are properly executed.

---

## 6. What's Next?
We have written the startup code and marked the vector table with `section(".isr_vector")`, but how does the compiler know that `.isr_vector` must be placed at address `0x08000000`, or what `_estack` and `_sidata` actually evaluate to?

That is the exact job of the **Linker Script (`.ld`)**, which we will dissect and write from scratch in **Part 4**!

---

[➡️ Next: Part 4 - Demystifying the Linker Script](../4_linker-script/readme.md)

