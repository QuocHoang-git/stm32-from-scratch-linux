# -----------------------------------------------------------------------------
# Bare-Metal Makefile for STM32F103C8T6 (ARM Cortex-M3)
# Author: Duong Quoc Hoang
# -----------------------------------------------------------------------------

TARGET    = firmware
BUILD_DIR = build

# Toolchain definitions
CC        = arm-none-eabi-gcc
OBJCOPY   = arm-none-eabi-objcopy
SIZE      = arm-none-eabi-size
OPENOCD   = openocd

# Source files
C_SOURCES = \
    startup_stm32f103c8.c \
    main.c

# Linker script
LDSCRIPT  = stm32f103c8.ld

# CPU & Architecture flags
MCU       = -mcpu=cortex-m3 -mthumb

# C Compiler flags
CFLAGS    = $(MCU) -Wall -Wextra -O0 -g3 \
            -ffunction-sections -fdata-sections \
            -specs=nano.specs -specs=nosys.specs

# Linker flags
LDFLAGS   = $(MCU) -T$(LDSCRIPT) \
            -Wl,--gc-sections \
            -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
            -specs=nano.specs -specs=nosys.specs

# Object files list
OBJECTS   = $(addprefix $(BUILD_DIR)/, $(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

# Default rule: build everything
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin size

# Compile C source files to object files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "  CC    $<"
	@$(CC) -c $(CFLAGS) $< -o $@

# Link object files into ELF executable
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@echo "  LD    $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Generate Intel HEX binary
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@echo "  HEX   $@"
	@$(OBJCOPY) -O ihex $< $@

# Generate raw flat binary
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "  BIN   $@"
	@$(OBJCOPY) -O binary -S $< $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $@

# Print memory size statistics
size: $(BUILD_DIR)/$(TARGET).elf
	@echo "================ Memory Consumption ================"
	@$(SIZE) --format=berkeley $<
	@echo "===================================================="

# Flash firmware via OpenOCD & ST-Link
flash: all
	@echo "Flashing $(TARGET).elf to STM32..."
	$(OPENOCD) -f interface/stlink.cfg -f target/stm32f1x.cfg \
	           -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)

.PHONY: all clean size flash

