# STM32 From Scratch on Linux

### Overview

A comprehensive guide to understanding STM32 bare-metal programming on Linux without relying on heavy IDEs.

### Goals - Demystify how microcontrollers boot and execute C/C++ programs

- Master low-level concepts: startup files, vector tables, and linker scripts
- Build foundational knowledge to write clean, optimized and efficient embedded code.

---

## 1. Hardware & Software Requirements
- **Hardware:** STM32F103C8T6 (Blue Pill), ST-Link V2, USB-to-UART module (optional).
- **Software & Toolchain:** Ubuntu/Linux, `arm-none-eabi-gcc`, `openocd` (or `stlink-tools`), `make`.

## 2. Project Structure

```text
stm32-from-scratch-linux/
├── .gitmodules                # Git Submodules configuration
├── Drivers/                   # Shared Hardware Drivers & Vendor Libraries
│   └── CMSIS/                 # Official ARM & ST CMSIS (Git Submodules)
│       ├── Core/              # ARM Cortex-M Core Peripheral Access Layer
│       └── Device/ST/STM32F1xx/ # ST Device Peripheral Access Layer
├── 1_toolchain-setup/         # Part 1: Setting up Arm Toolchain & OpenOCD
├── 2_memory-and-vector-table/ # Part 2: ARM Cortex-M Memory & Reset Sequence
├── 3_startup-code/            # Part 3: Writing C Startup Code & Vector Table
├── 4_linker-script/           # Part 4: Writing Linker Script from Scratch (.ld)
├── 5_makefile/                # Part 5: Build Automation with Makefile
├── 6_blinky/                  # Part 6: Complete Bare-Metal Blinky (Raw Registers)
├── 7_cmsis-refactoring/       # Part 7: Official ARM/ST CMSIS Integration
├── 8_clock-configuration/     # Part 8: Clock Tree & PLL Configuration (72 MHz)
├── 9_systick-timer/           # Part 9: SysTick Timer & Hardware 1ms Delay
├── 10_usart-printf/           # Part 10: Interactive UART CLI (Full-Duplex RX/TX)
├── 11_usart-dma/              # Part 11: High-Performance USART DMA (Zero-CPU)
├── docs/                      # Datasheets & Schematics
├── LICENSE                    # MIT License
└── readme.md                  # Project Root Documentation
```

## 3. Quick Start (How to Build & Flash)

If you already have the GNU Arm Toolchain and OpenOCD installed, you can build and flash the bare-metal blinky firmware right away:

```bash
# Navigate to the blinky project
cd 6_blinky

# Compile the firmware
make

# Flash firmware to STM32F103 via ST-Link & OpenOCD
make flash
```

## 4. Tutorial Series (Roadmap)

| Part | Topic | Status | Link |
| :--- | :--- | :---: | :--- |
| **01** | Setting up GNU Arm Toolchain & OpenOCD | ✅ Completed | [Read Guide](1_toolchain-setup/readme.md) |
| **02** | ARM Cortex-M Vector Table & Memory Layout | ✅ Completed | [Read Guide](2_memory-and-vector-table/readme.md) |
| **03** | Writing Startup Code from Scratch | ✅ Completed | [Read Guide](3_startup-code/readme.md) |
| **04** | Demystifying the Linker Script | ✅ Completed | [Read Guide](4_linker-script/readme.md) |
| **05** | Building the Project with Makefile | ✅ Completed | [Read Guide](5_makefile/readme.md) |
| **06** | Bare-metal GPIO Blinky | ✅ Completed | [Read Guide](6_blinky/readme.md) |
| **07** | Integrating Official CMSIS Headers | ✅ Completed | [Read Guide](7_cmsis-refactoring/readme.md) |
| **08** | Clock Tree & PLL Configuration (72 MHz) | ✅ Completed | [Read Guide](8_clock-configuration/readme.md) |
| **09** | SysTick Timer & Hardware 1ms Delay | ✅ Completed | [Read Guide](9_systick-timer/readme.md) |
| **10** | Interactive UART CLI & Command Parser | ✅ Completed | [Read Guide](10_usart-printf/readme.md) |
| **11** | High-Performance USART DMA (Zero-CPU) | ✅ Completed | [Read Guide](11_usart-dma/readme.md) |

> 🚀 **Next Repositories in this Series:**
> - **FreeRTOS Bare-Metal from Scratch**: Porting FreeRTOS kernel on STM32 using bare-metal toolchain and Makefile.
> - **STM32 Custom Bootloader & OTA**: Dual-bank firmware update, UART/YModem transfer, and vector table relocation.

## 5. References & Resources

* [STM32F103x8 Datasheet](docs/DS_stm32f103c8.pdf)
* [STM32F101/STM32F103 Reference Manual (RM0008)](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
* [ARM Cortex-M3 Technical Reference Manual](https://developer.arm.com/documentation/ddi0337/e/)
* [Blue Pill Original Schematic](docs/original-schematic-STM32F103C8T6-Blue_Pill.pdf)

## 6. License
This project is licensed under the [MIT License](LICENSE) - see the [LICENSE](LICENSE) file for details. 
You are free to use, study, modify, and distribute this material for both educational and commercial purposes.

## 7. Author & Contact
Created by Duong Quoc Hoang

[![GitHub](https://img.shields.io/badge/GitHub-Profile-181717?style=flat&logo=github)](https://github.com/QuocHoang-git)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?style=flat&logo=linkedin)](https://linkedin.com/in/duong-quoc-hoang)
[![Gmail](https://img.shields.io/badge/Email-Contact_Me-red?style=flat&logo=gmail)](mailto:duong.quoc.hoang.work@example.com)

> Feedback, issue reports, and pull requests are warmly welcomed! If you find this project helpful, feel free to give it a ⭐️!
