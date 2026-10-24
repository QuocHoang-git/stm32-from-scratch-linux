[⬅️ Back to Main README](../readme.md)

# Part 11: High-Performance USART DMA (Zero-CPU Overhead)

## 1. Overview
In [Part 10](../10_usart-printf/readme.md), we built an interactive serial shell using UART interrupts (`RXNEIE`). While interrupts are far superior to polling, they still require the Cortex-M3 CPU to wake up and execute an interrupt service routine (ISR) **for every single byte** transmitted or received.

At high baud rates (e.g. 115,200 or 921,600 bps) or during large data transfers, processing thousands of interrupts per second degrades CPU performance and disrupts real-time tasks.

In this tutorial, we unlock one of the most powerful hardware accelerators in the STM32F103: **Direct Memory Access (DMA)**:
* **TX with DMA1 Channel 4:** `printf()` writes directly to a memory buffer; DMA transfers hundreds of bytes into `USART1->DR` completely in the background with **0% CPU load**.
* **RX with DMA1 Channel 5 + IDLE Line Detection:** The golden industry standard for receiving variable-length packets without interrupting the CPU for individual bytes.

---

## 2. What is Direct Memory Access (DMA)?

The DMA controller is a dedicated hardware engine connected directly to the internal multi-layer bus matrix. It can read from memory and write to peripherals (or vice-versa) **in parallel with the CPU**:

```text
               ┌───────────────────────────────┐
               │    ARM Cortex-M3 Core (CPU)   │
               └───────────────┬───────────────┘
                               │ (AHB Bus)
        ┌──────────────────────┴──────────────────────┐
        │                                             │
        ▼                                             ▼
┌───────────────┐                             ┌───────────────┐
│     SRAM      │◄─────── [ DMA1 Controller ] ──────►│   USART1->DR  │
│ (tx/rx_buffer)│       (Channels 4 & 5)      │ (TX / RX Data)│
└───────────────┘                             └───────────────┘
                                                      │
                                                      ▼
                                                 Serial Cable
```

### Hardware Mapping on STM32F103 (RM0008, Table 78):
The DMA channels are hardwired to specific peripherals by silicon design:
* **`DMA1_Channel4`** ➔ **`USART1_TX`**
* **`DMA1_Channel5`** ➔ **`USART1_RX`**

---

## 3. Transmitting with DMA (Memory ➔ Peripheral)

To transmit data with DMA1 Channel 4:
1. **Source Address (`CMAR`):** Address of our RAM buffer (`tx_dma_buffer`).
2. **Destination Address (`CPAR`):** Address of the USART1 data register (`&(USART1->DR)`).
3. **Data Count (`CNDTR`):** Number of bytes to transfer.
4. **Configuration (`CCR`):**
   * `DIR = 1`: Read from memory, write to peripheral.
   * `MINC = 1`: Increment memory address after each byte.
   * `PINC = 0`: Do NOT increment peripheral address (always write to `USART1->DR`).
   * `TCIE = 1`: Trigger an interrupt only when the **entire transfer is complete**.
5. **USART DMA Request:** Set bit `DMAT` in `USART1->CR3`.

```c
void USART1_DMA_Transmit(const char *data, uint16_t length) {
    while (g_dma_tx_busy); /* Wait if previous DMA transfer is running */
    g_dma_tx_busy = 1;

    memcpy(tx_dma_buffer, data, length);

    DMA1_Channel4->CCR &= ~DMA_CCR_EN;  /* Disable to reconfigure */
    DMA1_Channel4->CMAR  = (uint32_t)tx_dma_buffer;
    DMA1_Channel4->CNDTR = length;
    DMA1->IFCR = DMA_IFCR_CTCIF4;       /* Clear complete flag */
    DMA1_Channel4->CCR |= DMA_CCR_EN;   /* Start hardware transfer! */
}
```
> **The CPU returns immediately!** While the serial cable sends out data at 115,200 baud, the CPU is already free to run other tasks.

---

## 4. Receiving Variable-Length Data: Circular DMA + IDLE Detection

### The Classic Real-World Pitfall (Human Typing vs. Packet Bursts):
At 115,200 baud, one byte takes only **87 microseconds** to transmit. An IDLE condition is triggered whenever the line remains quiet for **just one frame duration (~87 µs)**!

* **Machine Packets (Modbus / GPS / Scripts):** Bytes are transmitted consecutively with zero pause. An IDLE line occurs only at the very end of the packet.
* **Human Typing from Keyboard:** Humans pause for 100 to 500 ms between keystrokes. Therefore, **every single keystroke triggers an IDLE line**!

If firmware blindly assumes every IDLE interrupt is a complete command, typing `h`, `e`, `l`, `p` would trigger four separate commands: `'h'`, `'e'`, `'l'`, and `'p'`!

### The Robust Industrial Solution:
We configure DMA1 Channel 5 in **Circular Mode (`DMA_CCR_CIRC = 1`)**:
1. DMA silently streams all incoming bytes into a circular ring buffer in RAM.
2. When an IDLE event occurs, the CPU processes the newly arrived chunk from `g_rx_last_pos` to the current DMA position.
3. It accumulates characters, echoes them back to the terminal, handles Backspace, ignores `\n` following `\r` (CRLF filtering), and **submits the command only when the Enter key is pressed**!

### Real-World Embedded Pitfall: Standard I/O Buffering (`_IONBF`)
In standard C runtimes (like `newlib-nano`), `stdout` is **line-buffered** by default. Any `printf()` without a trailing newline `\n` (such as `printf("dma-stm32> ")`) will be held in C library RAM buffers and delayed until a later `\n` is printed.
To fix this in interactive CLI applications, we explicitly disable stdout buffering:
```c
setvbuf(stdout, NULL, _IONBF, 0); /* Disable stdout buffering */
```
and call `fflush(stdout);` after printing command prompts.

```c
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_IDLE) {
        /* Clear IDLE flag by reading SR followed by DR */
        volatile uint32_t temp = USART1->SR;
        temp = USART1->DR;
        (void)temp;

        uint16_t current_pos = RX_DMA_BUF_SIZE - (uint16_t)(DMA1_Channel5->CNDTR);

        /* Process newly arrived characters */
        while (g_rx_last_pos != current_pos) {
            char c = rx_dma_circular_buf[g_rx_last_pos];
            g_rx_last_pos = (g_rx_last_pos + 1) % RX_DMA_BUF_SIZE;

            /* Enter key: Submit command line */
            if (c == '\r' || c == '\n') {
                if (!g_cmd_ready && g_cmd_len > 0) {
                    g_cmd_line[g_cmd_len] = '\0';
                    g_cmd_ready = 1;
                }
                USART1_SendChar('\r');
                USART1_SendChar('\n');
            }
            /* Backspace key */
            else if (c == 0x08 || c == 0x7F) {
                if (g_cmd_len > 0 && !g_cmd_ready) {
                    g_cmd_len--;
                    USART1_SendChar('\b');
                    USART1_SendChar(' ');
                    USART1_SendChar('\b');
                }
            }
            /* Printable ASCII */
            else if (c >= 32 && c <= 126) {
                if (g_cmd_len < (CMD_LINE_BUF_SIZE - 1) && !g_cmd_ready) {
                    g_cmd_line[g_cmd_len++] = c;
                    USART1_SendChar(c); /* Echo back */
                }
            }
        }
    }
}
```

This guarantees 100% reliable operation for **both human typing and automated burst packets**!

---

## 5. Architectural Comparison

| Method | CPU Load | Interrupt Frequency | Packet Handling | Best Use Case |
| :--- | :---: | :---: | :---: | :--- |
| **Polling** | 100% (Blocks CPU) | None | Rigid / Prone to overrun | Quick startup testing |
| **Interrupts (Part 10)** | Low (~5-10%) | **1 per byte** | Character-by-character | Low-speed shells, keyboards |
| **DMA + IDLE (Part 11)** | **~0% (Zero Overhead)**| **1 per packet** | **Automatic burst capture** | **High-speed telemetry, Modbus, GPS, IoT protocols** |

---

## 6. How to Build & Test

All files are ready in this directory:
* [`main.c`](main.c) - DMA TX (`_write` retarget) and DMA RX IDLE shell
* [`Makefile`](Makefile) - Automated compilation

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
  11472	    112	   1944	  13528	   34d8	build/firmware.elf
====================================================
```

### Connect with `picocom`:
```bash
picocom -b 115200 /dev/ttyUSB0
```

### Test Interactive Shell:
```text
===================================================
   STM32F103 High-Performance USART DMA Controller  
===================================================
Core Clock : 72 MHz
TX Engine  : DMA1 Channel 4 (Memory -> USART1_DR)
RX Engine  : DMA1 Channel 5 + IDLE Line Detection
Type 'help' to test DMA command execution.

dma-stm32> uptime
DMA-Uptime: 6.420 seconds (6420 ms)

dma-stm32> led auto 200
DMA-Shell: LED set to AUTO mode (200 ms interval)

dma-stm32> led off
DMA-Shell: LED turned OFF
```

---

## 7. What's Next?
Congratulations! You have completed the **STM32 Bare-Metal on Linux Foundation Series** by mastering hardware DMA and zero-overhead asynchronous I/O!

From here, this rock-solid Board Support Package (BSP) serves as the foundation for advanced projects:
* **Embedded RTOS**: Porting FreeRTOS / RT-Thread on top of this bare-metal foundation.
* **Custom Bootloader & OTA**: Implementing in-application programming (IAP) and firmware flashing over UART/DMA.

---

[⬅️ Back to Main Series Overview](../readme.md)

