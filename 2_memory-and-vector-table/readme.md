[⬅️ Back to Main README](../readme.md)

# Part 2: ARM Cortex-M Vector Table & Memory Layout

## 1. Overview
Before writing our first line of bare-metal code, we must understand how the hardware behaves the moment power is applied. 

Unlike desktop operating systems where an OS loader initiates application execution, microcontrollers have no operating system. Instead, the ARM Cortex-M3 processor follows a strict, hardware-wired sequence to initialize its stack and jump to our code.

In this chapter, we will explore:
1. The 32-bit Memory Map of STM32F103 (Flash, SRAM, Peripherals).
2. The Boot Aliasing mechanism (BOOT pins).
3. The ARM Cortex-M hardware reset sequence.
4. The Vector Table architecture and its role in the system.

---

## 2. STM32F103 Memory Map

The ARM Cortex-M3 is a 32-bit processor with a 4 GB linear address space (`0x00000000` to `0xFFFFFFFF`). STMicroelectronics divides this space into predefined regions:

```text
+-----------------------+ 0xFFFFFFFF
| Cortex-M3 Internal    | (NVIC, SysTick, MPU registers: 0xE0000000 - 0xE00FFFFF)
+-----------------------+ 0xE0000000
| External Device       |
+-----------------------+ 0xA0000000
| External RAM          |
+-----------------------+ 0x60000000
| Peripherals           | (GPIO, USART, SPI, Timers: 0x40000000 - 0x5FFFFFFF)
+-----------------------+ 0x40000000
| SRAM (20 KB)          | (0x20000000 - 0x20004FFF) -> Stack, Heap, Variables
+-----------------------+ 0x20000000
| Flash Memory (64 KB)  | (0x08000000 - 0x0800FFFF) -> Code, Constants, Vector Table
+-----------------------+ 0x08000000
| Boot Memory Alias     | (0x00000000 - 0x0007FFFF) -> Mirrored to Flash / ROM / SRAM
+-----------------------+ 0x00000000
```

### Key Memory Regions for STM32F103C8T6:
* **Flash Memory (`0x08000000` - `0x0800FFFF`):** 64 KB of non-volatile memory where compiled code, read-only constants, and the Vector Table reside.
* **SRAM (`0x20000000` - `0x20004FFF`):** 20 KB of volatile memory. Used for global variables, heap, and call stack. (SRAM ends at `0x20005000`).
* **Peripherals (`0x40000000` - `0x5FFFFFFF`):** Memory-mapped hardware registers (e.g., RCC, GPIOA, GPIOC). Controlling a pin means writing values to these addresses.

---

## 3. Boot Modes & Memory Aliasing

At power-on or reset, the Cortex-M3 core **always fetches instructions starting at address `0x00000000`**. 

However, Flash memory physically starts at `0x08000000`. How does the CPU read our code from `0x00000000`?

The STM32 implements **Memory Aliasing** controlled by the `BOOT0` and `BOOT1` pins:

| BOOT0 | BOOT1 | Boot Mode | Aliased Memory at `0x00000000` |
| :---: | :---: | :--- | :--- |
| **0** | **X** | **Main Flash Memory** (Standard) | `0x08000000` is mapped to `0x00000000`. CPU executes user code. |
| **1** | **0** | **System Memory** | `0x1FFFF000` (Factory ST ROM bootloader for UART flashing). |
| **1** | **1** | **Embedded SRAM** | `0x20000000` is mapped to `0x00000000` (used for RAM debugging). |

> **Takeaway:** When `BOOT0 = 0`, reading from `0x00000000` is identical to reading from `0x08000000`.

---

## 4. The Hardware Reset Sequence

When the MCU leaves reset (power applied, NRST released), the ARM Cortex-M3 core performs the following steps automatically **in hardware**:

```text
                [ Hardware Reset / Power-On ]
                              │
                              ▼
        Read 4 bytes from address 0x00000000 (Flash 0x08000000)
                              │
                              ▼
             Load value into MSP (Main Stack Pointer)
                              │
                              ▼
        Read 4 bytes from address 0x00000004 (Flash 0x08000004)
                              │
                              ▼
          Load address into PC (Program Counter)
                              │
                              ▼
               Jump and execute Reset_Handler()
```

1. **Step 1:** The processor fetches the 32-bit word at `0x00000000` and assigns it to the **Main Stack Pointer (`MSP`)**. This sets up the stack in SRAM before executing any instructions.
2. **Step 2:** The processor fetches the 32-bit word at `0x00000004` and assigns it to the **Program Counter (`PC`)**. This is the address of `Reset_Handler`.
3. **Step 3:** The CPU jumps to `Reset_Handler()`, which initializes memory (`.data` and `.bss`) and ultimately calls `main()`.

### ⚠️ The Thumb Bit Rule (Bit 0)
ARM Cortex-M processors only support the **Thumb-2** instruction set; they do **not** support ARM 32-bit state.
* To signify Thumb state, **Bit 0 of any function pointer in the vector table must be set to 1** (an odd address).
* For example, if `Reset_Handler` is located at `0x08000100`, the vector table entry must contain `0x08000101`.
* *Don't worry:* The GNU C/C++ compiler and linker will automatically set Bit 0 for you, but understanding this prevents mysterious `UsageFault` exceptions during low-level debugging!

---

## 5. The Cortex-M3 Vector Table

The **Vector Table** is an array of 32-bit addresses stored at the very beginning of Flash memory. It tells the CPU where to jump whenever an exception or interrupt happens.

### System Exceptions (Defined by ARM Architecture):

| Address / Offset | Vector Name | Description |
| :--- | :--- | :--- |
| `0x00000000` | `Initial SP` | Top address of the stack (typically end of SRAM: `0x20005000`) |
| `0x00000004` | `Reset` | Entry point of program execution (`Reset_Handler`) |
| `0x00000008` | `NMI` | Non-Maskable Interrupt |
| `0x0000000C` | `HardFault` | Generic error handler when an unrecoverable fault occurs |
| `0x00000010` | `MemManage` | Memory protection unit violation |
| `0x00000014` | `BusFault` | Memory bus error (e.g. invalid memory access) |
| `0x00000018` | `UsageFault` | Undefined instruction, unaligned access, or invalid state |
| `0x0000001C` - `0x0000002B` | *Reserved* | Reserved by ARM |
| `0x0000002C` | `SVCall` | Supervisor Call (used by RTOS for system calls) |
| `0x00000030` | `DebugMonitor`| Debug Monitor exception |
| `0x00000034` | *Reserved* | Reserved by ARM |
| `0x00000038` | `PendSV` | Pendable Service Request (used by RTOS for context switching)|
| `0x0000003C` | `SysTick` | System Tick timer interrupt (used for timing / OS scheduling) |

### Peripheral Interrupts (Defined by STMicroelectronics):
Starting from offset `0x00000040`, the vector table lists external interrupt handlers specific to the STM32F103:
* `0x00000058`: `EXTI0_IRQHandler` (External interrupt line 0)
* `0x000000B4`: `USART1_IRQHandler`
* `0x000000D4`: `TIM2_IRQHandler`
* *(Full list is documented in RM0008 Reference Manual, Section 10: Interrupts and Events).*

---

## 6. What's Next?

Now that we understand:
1. Where code lives (`0x08000000`).
2. Where the stack begins (`0x20005000`).
3. How the processor boots using the Vector Table.

In **Part 3**, we will write the actual **Startup Code** (`startup_stm32f103c8.c`) to construct this Vector Table and implement the `Reset_Handler`!

---

[➡️ Next: Part 3 - Writing the Startup Code from Scratch](../3_startup-code/readme.md)

