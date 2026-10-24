[⬅️ Back to Main README](../readme.md)

# Part 4: Demystifying the Linker Script (.ld)

## 1. Overview
In [Part 3](../3_startup-code/readme.md), our C startup code referenced symbols like `_estack`, `_sidata`, `_sdata`, and `_sbss`, and instructed the compiler to place the vector table in a custom section called `.isr_vector`.

However, the compiler by itself does not know:
* Where Flash memory starts or how large it is.
* Where SRAM begins or where the stack pointer should point.
* How to separate code (read-only) from variables (read/write).

This is the job of the **Linker Script (`.ld`)**. The GNU Linker (`arm-none-eabi-ld`) uses this script to assemble individual object files (`.o`) into a unified Executable and Linkable Format (`.elf`) binary with concrete hardware memory addresses.

---

## 2. Fundamental Concept: VMA vs. LMA

Understanding the distinction between **VMA** and **LMA** is the key to mastering embedded linker scripts:

* **VMA (Virtual Memory Address / Execution Address):** The address where a section resides **while the program is running**.
* **LMA (Load Memory Address / Storage Address):** The address where a section is stored **inside non-volatile Flash storage when the device is powered off**.

### Why does `.data` need two addresses?
Consider a global variable: `int counter = 42;`
1. When the microcontroller is powered off, SRAM loses all data. Therefore, the initial value `42` must be permanently burned into **Flash memory (LMA)**.
2. When the CPU runs, it needs to modify `counter++`. Since Flash cannot be written to like RAM, the variable must live in **SRAM (VMA)** during execution.

```text
               +-----------------------------+
Flash (LMA)    | .isr_vector | .text | .rodata| .data (initial values) |
               +-----------------------------+         │
                                                       │ (Copied at boot
                                                       │  by Reset_Handler)
                                                       ▼
SRAM (VMA)                                   +-------------------+
                                             | .data | .bss | ...|
                                             +-------------------+
```

---

## 3. Dissecting the Linker Script

A complete GNU Linker Script consists of three major blocks:

### 1. Entry Point & Memory Definition
We define the entry function and the physical memory boundaries of our STM32F103C8T6 (64 KB Flash, 20 KB SRAM):

```ld
ENTRY(Reset_Handler)

/* End of SRAM = 0x20000000 + 20K = 0x20005000 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}
```

* `rx`: Read and Execute permissions (Code and Constants).
* `rwx`: Read, Write, and Execute permissions (Variables, Heap, Stack).

---

### 2. The SECTIONS Command
The `SECTIONS` block controls how input sections from `.o` files are mapped into output sections and placed into `FLASH` or `RAM`.

#### Section A: `.isr_vector` (Vector Table)
Must be placed at the very beginning of Flash memory (`0x08000000`):

```ld
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))    /* Keep this section even if unreferenced */
    . = ALIGN(4);
  } > FLASH
```
> **Note on `KEEP()`:** When using dead-code elimination (`--gc-sections`), the linker discards unused functions. `KEEP()` tells the linker **never** to delete the vector table.

#### Section B: `.text` and `.rodata` (Code and Constants)
Both reside in `FLASH`:

```ld
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* Code */
    *(.text*)          /* Sub-sections (when using -ffunction-sections) */
    . = ALIGN(4);
    _etext = .;        /* Global symbol marking end of code */
  } > FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* Read-only data (const char*, string literals) */
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH
```

#### Section C: `.data` (Initialized Data)
Located in `RAM` (VMA), but loaded from `FLASH` (LMA):

```ld
  /* LMA is stored in Flash right after .rodata */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* Start of .data in RAM (VMA) */
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;        /* End of .data in RAM (VMA) */
  } > RAM AT > FLASH
```
* `> RAM AT > FLASH`: Tells the linker that execution happens in `RAM`, but storage is in `FLASH`.
* `LOADADDR(.data)`: Returns the address in Flash where `.data` initialization bytes are stored.

#### Section D: `.bss` (Uninitialized Data)
Located entirely in `RAM`. It takes zero space in Flash storage:

```ld
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;         /* Start of .bss in RAM */
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;         /* End of .bss in RAM */
  } > RAM
```

---

## 4. Complete Linker Script
The complete, ready-to-use linker script is available in this folder:
📁 [`stm32f103c8.ld`](stm32f103c8.ld)

> **💡 Best Practice (Why avoid discarding `libgcc.a`?):**  
> Some minimal linker scripts place `/DISCARD/ : { libgcc.a (*) }` to shrink output size. However, ARM Cortex-M processors rely on helper functions inside `libgcc.a` for operations like 64-bit integer division (`__aeabi_ldivmod`) or software floating-point. Instead of discarding them manually, we rely on the linker's `--gc-sections` flag, which safely removes only unused functions while preserving essential compiler routines.

---

## 5. What's Next?
We now have:
1. The **Startup Code** (`startup_stm32f103c8.c`).
2. The **Linker Script** (`stm32f103c8.ld`).

Now we need a tool to tie them together, compile our application code (`main.c`), and produce `.elf` and `.bin` binaries.

In **Part 5**, we will write a production-ready **Makefile** from scratch!

---

[➡️ Next: Part 5 - Building the Project with Makefile](../5_makefile/readme.md)

