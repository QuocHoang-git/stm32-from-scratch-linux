/**
 ******************************************************************************
 * @file      stm32f103c8.ld
 * @author    Duong Quoc Hoang
 * @brief     Minimal Linker Script for STM32F103C8T6 (64KB Flash, 20KB SRAM).
 ******************************************************************************
 */

/* Entry point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);    /* End of 20KB RAM = 0x20005000 */

/* Generate a link error if heap and stack don't fit into RAM */
_min_heap_size  = 0x200; /* 512 bytes */
_min_stack_size = 0x400; /* 1024 bytes */

/* Specify the memory areas */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

/* Define output sections */
SECTIONS
{
  /* The startup vector table goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Vector table */
    . = ALIGN(4);
  } > FLASH

  /* The program code and other data into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* Global symbol at end of code */
  } > FLASH

  /* Constant data into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections */
    . = ALIGN(4);
  } > FLASH

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections into RAM, loaded from FLASH */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* Create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* Define a global symbol at data end */
  } > RAM AT > FLASH

  /* Uninitialized data section into RAM */
  .bss :
  {
    . = ALIGN(4);
    _sbss = .;         /* Define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* Define a global symbol at bss end */
    __bss_end__ = _ebss;
  } > RAM

  /* User Stack and Heap validation */
  ._user_heap_stack :
  {
    . = ALIGN(4);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _min_heap_size;
    . = . + _min_stack_size;
    . = ALIGN(4);
  } > RAM

  .ARM.attributes 0 : { *(.ARM.attributes) }
}

