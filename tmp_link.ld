OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)
ENTRY(reset_handler)
MEMORY
{
 FLASH (rw) : ORIGIN = (0x00400000), LENGTH = (16) * 1K
 SRAM (rwx) : ORIGIN = (0x20400000), LENGTH = (384) * 1K
}
SECTIONS
{
 .text :
 {
  . = ALIGN(4);
  KEEP(*(.vectors .vectors.*))
  *(.text .text.*)
  *(.rodata .rodata.*)
 } > FLASH
 . = ALIGN(4);
 _etext = .;
 .relocate : AT (_etext)
 {
  . = ALIGN(4);
  _srelocate = .;
  *(.data .data.*);
  . = ALIGN(4);
  _erelocate = .;
 } > SRAM
 .bss (NOLOAD) :
 {
  . = ALIGN(4);
  _szero = .;
  *(.bss .bss.*);
  *(COMMON)
  . = ALIGN(4);
  _ezero = .;
 } > SRAM
 .stack (NOLOAD) :
 {
  . = ALIGN(8);
  . = . + 4096;
  . = ALIGN(8);
  _estack = .;
 } > SRAM
}
