[⬅️ Back to Main README](../readme.md)

# Part 10: Interactive UART CLI (Full-Duplex RX/TX with Interrupts)

## 1. Overview
In standard embedded systems, communicating with a device requires two-way interaction: sending commands from a PC and receiving responses from the microcontroller.

In this tutorial, we build a **Full-Duplex Interactive Command Line Interface (CLI) / Shell** over USART1 running at **115,200 Baud**:
* **Hardware Interrupt-driven Reception (`RXNEIE`):** Never misses a keystroke while the CPU is busy doing other tasks.
* **Interactive Terminal Line Editing:** Local character echo, visual backspace deletion, and line buffering.
* **Non-Blocking Multitasking:** Blinks an LED concurrently while running an interactive prompt without an RTOS!
* **Supported Commands:**
  * `uptime`: Prints system uptime in seconds and milliseconds.
  * `led auto <ms>`: Configures automatic blinking with a custom interval (e.g. `led auto 1000`).
  * `led manual`: Stops automatic blinking, allowing manual control.
  * `led on`: Turns the on-board PC13 LED ON.
  * `led off`: Turns the on-board PC13 LED OFF.
  * `help`: Lists all available commands.

---

## 2. Hardware Wiring: Connecting Blue Pill to PC

```text
    STM32F103 (Blue Pill)                  USB-to-UART Adapter
  ┌───────────────────────┐              ┌─────────────────────┐
  │   PA9  (USART1_TX)   ─┼─────────────►│ RXD (Receive)       │
  │   PA10 (USART1_RX)   │◄─────────────┼─ TXD (Transmit)     │
  │   GND                 │──────────────┼─ GND                 │
  └───────────────────────┘              └──────────┬──────────┘
                                                    │ (USB Cable)
                                                    ▼
                                            Linux PC (/dev/ttyUSB0)
```

> ⚠️ **Important:** Connect **PA9 (TX) to RXD** and **PA10 (RX) to TXD**. Connect common **GND**.

---

## 3. Why Use RX Interrupts Instead of Polling?

When reading UART data, there are two approaches:

### 1. Polling (`while (!(USART1->SR & USART_SR_RXNE));`)
* **Problem:** The CPU is stuck waiting for a byte to arrive. If the CPU is busy delaying, executing calculations, or toggling an LED, incoming bytes overflow and are lost forever!

### 2. Interrupt-Driven Reception (`USART1_IRQHandler`)
* **Solution:** Whenever a byte arrives over the wire, the UART hardware immediately asserts an interrupt request to the **ARM Core NVIC (Nested Vectored Interrupt Controller)**.
* The CPU pauses whatever it is doing for a few clock cycles, saves the incoming byte into an internal RAM buffer, and returns to work. **Zero dropped characters!**

```c
/* Enable RX Not Empty Interrupt (RXNEIE) */
USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

/* Enable USART1 Interrupt in ARM Core NVIC */
NVIC_EnableIRQ(USART1_IRQn);
```

---

## 4. Interactive Terminal Line Editing

When typing in a serial terminal (like `picocom`), users expect:
1. **Echo:** Seeing the letter they just typed.
2. **Backspace Support:** Pressing backspace should erase the last character visually.
3. **Line Completion:** Pressing Enter (`\r` or `\n`) submits the line buffer for parsing.

Here is how the interrupt handler [`USART1_IRQHandler`](main.c) handles terminal interaction:

```c
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {
        char c = (char)(USART1->DR & 0xFF);

        /* 1. Enter key pressed */
        if (c == '\r' || c == '\n') {
            if (!rx_line_ready && rx_index > 0) {
                rx_line_buffer[rx_index] = '\0';
                rx_line_ready = 1; /* Signal main loop to process command */
            }
            USART1_SendChar('\r');
            USART1_SendChar('\n');
        }
        /* 2. Backspace pressed (0x08 or 0x7F) */
        else if (c == 0x08 || c == 0x7F) {
            if (rx_index > 0) {
                rx_index--;
                /* Terminal erasure trick: move back, print space, move back */
                USART1_SendChar('\b');
                USART1_SendChar(' ');
                USART1_SendChar('\b');
            }
        }
        /* 3. Normal printable character */
        else if (rx_index < (CMD_BUFFER_SIZE - 1) && !rx_line_ready) {
            rx_line_buffer[rx_index++] = c;
            USART1_SendChar(c); /* Echo character back */
        }
    }
}
```

---

## 5. Non-Blocking Concurrent Multitasking

A common mistake in beginner firmware is using blocking `delay_ms(1000)` inside the main loop, which would freeze command processing for a full second!

Instead, we use a **non-blocking timestamp state machine** based on our SysTick `g_ms_ticks`:

```c
while (1) {
    /* Task A: Non-blocking LED Auto-Blink */
    if (g_led_auto_mode) {
        if ((g_ms_ticks - last_led_toggle) >= g_led_interval_ms) {
            last_led_toggle = g_ms_ticks;
            GPIOC->ODR ^= GPIO_ODR_ODR13;
        }
    }

    /* Task B: Handle incoming command line */
    if (rx_line_ready) {
        Process_Command(rx_line_buffer);
        rx_index = 0;
        rx_line_ready = 0;
        printf("stm32> ");
    }

    /* Sleep CPU until next SysTick or UART interrupt */
    __WFI();
}
```

This achieves responsive multitasking on bare-metal hardware without requiring an operating system!

---

## 6. How to Build & Test the Interactive Shell

### Step 1: Flash the Firmware
```bash
make
make flash
```

### Step 2: Connect to Serial Terminal on Linux
Open your Linux terminal and launch `picocom`:

```bash
picocom -b 115200 /dev/ttyUSB0
```
*(To exit picocom: press `Ctrl + A` then `Ctrl + X`)*.

---

### Step 3: Test Interactive Commands

Press Enter or Reset to see the prompt:

```text
==============================================
   STM32 Bare-Metal Interactive UART Shell    
==============================================
Core Clock : 72 MHz
Baud Rate  : 115200 bps (8N1)
Type 'help' to view available commands.

stm32> help

Available Commands:
  uptime          - Print system uptime in seconds and milliseconds
  led auto <ms>   - Enable automatic blinking with specified period (e.g. 'led auto 1000')
  led manual      - Switch to manual LED control (pauses auto blink)
  led on          - Turn on PC13 LED (switches to manual)
  led off         - Turn off PC13 LED (switches to manual)
  help            - Display this help message

stm32> uptime
Uptime: 14.825 seconds (14825 total ms)

stm32> led auto 1000
LED mode: AUTO (toggling every 1000 ms)

stm32> led auto 100
LED mode: AUTO (toggling every 100 ms)

stm32> led manual
LED mode: MANUAL (auto blink disabled. Use 'led on' or 'led off')

stm32> led on
LED turned ON (Manual mode)

stm32> led off
LED turned OFF (Manual mode)
```

Notice:
* When you type `led auto 1000`, the LED toggles slowly every 1 second.
* When you type `led auto 100`, the LED strobes rapidly at 10 Hz!
* When you type `led off` or `led on`, automatic blinking stops and you have full manual control.

---

## 7. What's Next?
We have built a fully functional, bidirectional serial console with interrupts and non-blocking multitasking!

In **Part 11**, we will dive deeper into ARM hardware interrupts by implementing **External Interrupts (EXTI) & Button Input**, triggering immediate hardware responses whenever an external physical push-button is pressed!

---

[➡️ Next: Part 11 - External Interrupts (EXTI) & Button Input](../11_exti-button/readme.md)
