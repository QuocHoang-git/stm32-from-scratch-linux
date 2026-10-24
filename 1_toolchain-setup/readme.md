[⬅️ Back to Main README](../readme.md)

# Setting up GNU Arm Toolchain & OpenOCD

## 1. Overview
In this guide, we will setup a complete bare-metal development environment on Linux from scratch. By the end of this tutorial, you will have a working cross-compilation toolchain and be able to communicate with the STM32F103 board using ST-Link and OpenOCD.

---

## 2. Understanding the Tools
Before installing, let's understand the core components:

* **`arm-none-eabi-gcc` (Cross-Compiler):**
    * `arm`: Targets the ARM architecture.
    * `none`: No operating system (bare-metal).
    * `eabi`: Embedded Application Binary Interface.
* **`openocd` (Open On-Chip Debugger):** An open-source software that bridges your PC and the microcontroller via the SWD/JTAG interface (ST-Link).
* **`arm-none-eabi-gdb`:** GNU Debugger for inspecting registers, setting breakpoints, and stepping through code.
* **`make`:** Automates the build process.

---
## 3. Installation
Run the following commands in your Linux terminal to install the necessary packages:

```bash
sudo apt update
sudo apt install -y gcc-arm-none-eabi \
                    gdb-arm-none-eabi \
                    libnewlib-arm-none-eabi \
                    build-essential \
                    openocd \
                    stlink-tools
```

---
## 4. Configuring USB Permissions (Udev Rules)
By default, Linux requires root (`sudo`) privileges to access USB devices. To allow regular users to access the ST-Link programmer, we must configure `udev` rules.
1. Download or copy the official ST-Link udev rules:
```bash
sudo cp /usr/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
```
*(Or use the rules provided by `stlink-tools`)*:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```
2. Add your user to the `plugdev` and `dialout` groups:
```bash
sudo usermod -aG plugdev,dialout $USER
```
> **Note:** Log out and log back in (or restart) for group changes to take effect.
---
## 5. Verification & Sanity Checks
### Step 1: Verify the Toolchain
Check that the compiler and build tools are accessible in your `$PATH`:
```bash
arm-none-eabi-gcc --version
make --version
openocd --version
```

### Step 2: Test ST-Link Connection with Target
Connect your **ST-Link V2** to the **STM32F103 (Blue Pill)** via SWD pins:
* `3.3V` ➔ `3.3V`
* `GND`  ➔ `GND`
* `SWDIO` ➔ `SWDIO`
* `SWCLK` ➔ `SWCLK`
Plug the ST-Link into your Linux PC and run:
```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg
```
**Expected Successful Output:**
```text
Info : STLINK V2 JTAG/SWD v29 API v2 SWIM v7 VID 0x0483 PID 0x3748
Info : using stlink api v2
Info : Target voltage: 3.242352
Info : [stm32f1x.cpu] Cortex-M3 r1p1 processor detected
Info : [stm32f1x.cpu] target has 6 breakpoints, 4 watchpoints
Info : starting gdb server for stm32f1x.cpu on 3333
```
Press `Ctrl + C` to exit.
---
## 6. Troubleshooting Common Pitfalls
* **`LIBUSB_ERROR_ACCESS` / Permission Denied:**
  Make sure you configured the `udev` rules properly and added your user to the `plugdev` group.
* **`UNEXPECTED idcode: 0x2ba01477`:**
  Many Blue Pill boards use clone/compatible chips (like CKS32F103 or CS32F103). If OpenOCD fails due to an unexpected CPUTAPID, override it by adding `-c "set CPUTAPID 0x2ba01477"` to the OpenOCD command.
* **Target not found / Cannot connect:**
  Check your 4-wire SWD connections. If the chip is locked or low-power sleep is enabled, press and hold the `RESET` button on the board while starting OpenOCD, then release it.
---
[➡️ Next: Part 2 - Understanding ARM Cortex-M Vector Table & Memory Layout](../2_memory-and-vector-table/readme.md)
