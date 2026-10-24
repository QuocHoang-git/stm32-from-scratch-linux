[⬅️ Back to Main README](../readme.md)

# Part 5: Building the Project with Makefile

## 1. Overview
In [Part 3](../3_startup-code/readme.md) and [Part 4](../4_linker-script/readme.md), we created:
* `startup_stm32f103c8.c` (C Startup Code)
* `stm32f103c8.ld` (Linker Script)

To turn our source code into a runnable firmware binary, we must perform several steps:
1. Compile each `.c` source into an object file (`.o`).
2. Link all `.o` files with the Linker Script to produce an Executable and Linkable Format file (`.elf`).
3. Extract raw machine code into a flat binary file (`.bin`) or Intel Hex (`.hex`).
4. Inspect the exact Flash and SRAM consumption using `size`.
5. Flash the binary to the microcontroller using `OpenOCD`.

Typing these commands manually in the terminal is tedious and error-prone. A **Makefile** automates the entire build and flash workflow with simple commands like `make` and `make flash`.

---

## 2. The Embedded Build Process (Step-by-Step)

When you run `arm-none-eabi-gcc` or `make`, the toolchain does not convert C code directly into machine code in a single leap. Instead, it passes through **four distinct stages**, followed by binary extraction for hardware flashing:

```text
 [ main.c ] ──(1. Preprocessor)──► [ main.i ]
                                        │
                               (2. Compiler)
                                        │
                                        ▼
                                   [ main.s ] (Thumb-2 Assembly)
                                        │
                               (3. Assembler)
                                        │
                                        ▼
                                   [ main.o ] (Relocatable Object File)
                                        │
 [ startup.o ] ─────────────────────────┤
 [ stm32f103c8.ld ] ──────────────► (4. Linker)
                                        │
                                        ▼
                                 [ firmware.elf ] (Executable ELF)
                                        │
                           (5. Binary Extraction)
                                        ├──► firmware.bin (Raw Flash Bytes)
                                        ├──► firmware.hex (Intel HEX)
                                        └──► Memory Size Report
```

---

### Stage 1: Preprocessing (`.c` ➔ `.i`)
* **Tool:** `arm-none-eabi-cpp` (or `gcc -E`)
* **Input:** `main.c`, header files (`.h`)
* **Output:** Preprocessed C file (`main.i`)
* **What happens:**
  1. **Macro Expansion:** All `#define` constants and macros are substituted with their literal values.
  2. **File Inclusion:** `#include <stdint.h>` is replaced by the literal contents of that header file.
  3. **Conditional Compilation:** `#ifdef`, `#ifndef`, `#if` blocks are evaluated, and false branches are stripped out.
  4. **Comment Stripping:** All comments (`//` and `/* */`) are removed.

> **Try it yourself:**
> ```bash
> arm-none-eabi-gcc -E main.c -o main.i
> ```
> If you open `main.i`, you will see pure C code with all `#define`s replaced and headers expanded, ready for the compiler.

---

### Stage 2: Compilation (`.i` ➔ `.s`)
* **Tool:** `arm-none-eabi-gcc` (specifically the internal compiler `cc1`)
* **Input:** Preprocessed source (`main.i`)
* **Output:** Assembly source file (`main.s`)
* **What happens:**
  1. **Syntax & Semantic Analysis:** Verifies that C syntax, types, and function signatures conform to the C standard.
  2. **Optimization:** Applies code optimizations based on flags like `-O0` (no optimization), `-Os` (size optimization), or `-O2` (speed optimization).
  3. **Code Generation:** Translates the C logic into **ARM Thumb-2 assembly instructions** specific to the Cortex-M3 core.

> **Try it yourself:**
> ```bash
> arm-none-eabi-gcc -S -mcpu=cortex-m3 -mthumb main.c -o main.s
> ```
> Inspect `main.s` to see assembly mnemonics like `ldr`, `str`, `bx`, `subs`, and `nop`.

---

### Stage 3: Assembly (`.s` ➔ `.o`)
* **Tool:** `arm-none-eabi-as` (or `gcc -c`)
* **Input:** Assembly file (`main.s`)
* **Output:** Relocatable Object file (`main.o`) in **ELF (Executable and Linkable Format)**
* **What happens:**
  1. Translates human-readable assembly instructions into binary **machine code opcodes** (1s and 0s).
  2. Places functions and data into distinct sections: `.text` (code), `.data` (initialized data), and `.bss` (uninitialized data).
  3. **Relocation Table Creation:** Because `main.c` references external symbols (like `Reset_Handler` or `_sidata`) whose addresses are not yet known, the assembler leaves placeholders and creates a "Relocation Table" for the Linker to resolve later.

> **Try it yourself:**
> ```bash
> arm-none-eabi-gcc -c -mcpu=cortex-m3 -mthumb main.c -o main.o
> # Inspect section headers of the object file:
> arm-none-eabi-readelf -S main.o
> ```

---

### Stage 4: Linking (`.o` + `.ld` ➔ `.elf`)
* **Tool:** `arm-none-eabi-ld` (invoked via `gcc`)
* **Input:** All object files (`main.o`, `startup_stm32f103c8.o`), libraries (`libc.a`, `libgcc.a`), and the Linker Script (`stm32f103c8.ld`).
* **Output:** Unified Executable ELF file (`firmware.elf`)
* **What happens:**
  1. **Section Merging:** Combines identical sections from all object files (e.g., merging `.text` from `main.o` and `startup_stm32f103c8.o` into one master `.text` section).
  2. **Symbol Resolution:** Matches every external function call or variable reference (`extern`) to its concrete definition across all files.
  3. **Relocation & Address Assignment:** Consults the Linker Script (`stm32f103c8.ld`) to assign **absolute physical memory addresses** (e.g., placing the vector table at Flash `0x08000000` and assigning RAM variables to `0x20000000`).
  4. **Dead Code Elimination (`--gc-sections`):** Strips any unreferenced functions or variables to minimize memory usage.

> **Inspect the final linked ELF:**
> ```bash
> arm-none-eabi-objdump -d build/firmware.elf    # Disassemble full binary with addresses
> arm-none-eabi-nm build/firmware.elf            # List all symbols and their absolute addresses
> ```

---

### Stage 5: Binary Extraction (`.elf` ➔ `.bin` / `.hex`)
* **Tool:** `arm-none-eabi-objcopy` & `arm-none-eabi-size`
* **Input:** `firmware.elf`
* **Output:** `firmware.bin`, `firmware.hex`
* **Why can't we flash `.elf` directly into raw Flash memory?**
  * An `.elf` file contains a lot of metadata: symbol tables, debug information (`DWARF`), section headers, and string tables that are useful for GDB debugging on PC, but **meaningless to the ARM Cortex-M hardware**.
  * Microcontroller Flash memory only needs the **raw machine instructions and constant data** starting from address `0x08000000`.
  * `arm-none-eabi-objcopy -O binary firmware.elf firmware.bin` extracts only the loadable sections (`.isr_vector`, `.text`, `.rodata`, `.data`), creating a pure byte-for-byte memory dump ready to be burned into Flash by ST-Link / OpenOCD.

---

## 3. Dissecting Essential Compiler & Linker Flags

Bare-metal embedded systems require specific GCC flags that differ from desktop C programming:

### Compiler Flags (`CFLAGS`)
* **`-mcpu=cortex-m3 -mthumb`:** Forces the compiler to generate ARM Cortex-M3 specific Thumb-2 instructions.
* **`-O0` or `-Os`:** Optimization level (`-O0` for debugging, `-Os` to optimize for binary size).
* **`-g3`:** Generates comprehensive debug information for GDB.
* **`-Wall -Wextra`:** Enables compiler warnings to catch subtle bugs early.
* **`-ffunction-sections -fdata-sections`:** Places each function and variable into its own separate ELF section (e.g. `.text.main` instead of one big `.text`). This enables the linker to prune dead code.
* **`-specs=nano.specs -specs=nosys.specs`:** Links against `newlib-nano` (a tiny C library optimized for microcontrollers) without requiring operating system syscalls (like `fork` or `read`).

### Linker Flags (`LDFLAGS`)
* **`-T stm32f103c8.ld`:** Specifies our custom linker script.
* **`-Wl,--gc-sections`:** Instructs the linker to perform **Dead Code Elimination** (Garbage Collection). Functions or variables never called by `main()` or the vector table are completely stripped from the final binary!
* **`-Wl,-Map=$(BUILD_DIR)/$(TARGET).map`:** Generates a detailed memory map file showing the exact byte address and size of every function and variable.

---

## 4. Key Makefile Targets

A well-structured Makefile provides intuitive targets:

| Command | Purpose |
| :--- | :--- |
| `make` or `make all` | Compiles and links the project, outputs `.elf`, `.bin`, `.hex`, and prints memory usage. |
| `make flash` | Uses OpenOCD to automatically burn `firmware.bin` into STM32 Flash and reset the board. |
| `make size` | Shows how many bytes of Flash and SRAM are used. |
| `make clean` | Deletes the `build/` directory and all generated artifacts. |

---

## 5. Complete Makefile
The full, modular Makefile is located in this directory:
📁 [`Makefile`](Makefile)

> **📌 Note on Project Execution:**  
> This directory serves as a standalone explanation of Makefile construction and flags. To build and test a live, working project that integrates this Makefile with `main.c`, `startup_stm32f103c8.c`, and `stm32f103c8.ld`, head over to [Part 6 (`6_blinky/`)](../6_blinky/readme.md).

### OpenOCD Auto-Flash Rule:
Notice how cleanly OpenOCD can program the board in headless mode:

```makefile
flash: all
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
	        -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"
```

---

## 6. What's Next?
Now we possess the complete build infrastructure:
1. **Toolchain & Debugger** ([Part 1](../1_toolchain-setup/readme.md))
2. **Memory Map & Reset Theory** ([Part 2](../2_memory-and-vector-table/readme.md))
3. **C Startup Code** ([Part 3](../3_startup-code/readme.md))
4. **Linker Script** ([Part 4](../4_linker-script/readme.md))
5. **Makefile Automation** ([Part 5](./readme.md))

In **Part 6 (Final Milestone)**, we will write our first bare-metal `main.c` program: **Bare-metal GPIO Blinky**, by directly manipulating hardware registers without using HAL or CMSIS!

---

[➡️ Next: Part 6 - Bare-metal GPIO Blinky](../6_blinky/readme.md)

