
#include <config.h>

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* This is important for loading into RAM - if you don't specify the ENTRY then it will set the entry to the load point of ".text" which is the vector table, meaning it will execute until it hits the first real instruction following the ".vectors" section, which could be roughly anything. */
ENTRY(reset_handler)

/* Memory Spaces Definitions */
/* TODO: TCM memory */
MEMORY
{
	/*rom (rx) : ORIGIN = 0x21000000, LENGTH = 0x00001000*/
	/*ram (rwx) : ORIGIN = 0x21001000, LENGTH = 0x00004000*/
#if defined(CONFIG_RAM_BUILD)
	rom  (rx) : ORIGIN = CONFIG_SYS_TEXT_BASE, LENGTH = 0x1000
	ram (rwx) : ORIGIN = (CONFIG_SYS_TEXT_BASE + 0x1000), LENGTH = 0x4000
#else
	rom  (rx) : ORIGIN = CONFIG_SYS_TEXT_BASE, LENGTH = 0x1000
	/* TODO: Need a constant for RAM base */
	ram (rwx) : ORIGIN = 0x21000000, LENGTH = 0x4000
	/*ram (rwx) : ORIGIN = 0x20400000, LENGTH = 0x4000*/
#endif
}

/* The stack size used by the application. NOTE: you need to adjust according to your application. */
STACK_SIZE = DEFINED(STACK_SIZE) ? STACK_SIZE : 0x1000;

SECTIONS
{
	.text :
	{
		. = ALIGN(4);
		KEEP(*(.vectors .vectors.*))
		*(.text .text.*)
		*(.rodata .rodata.*)
	} > rom

	. = ALIGN(4);
	_etext = .;

	.relocate : AT (_etext)
	{
		. = ALIGN(4);
		_srelocate = .;
		*(.data .data.*);
		. = ALIGN(4);
		_erelocate = .;
	} > ram

	.bss (NOLOAD) :
	{
		. = ALIGN(4);
		_szero = .;
		*(.bss .bss.*);
		*(COMMON)
		. = ALIGN(4);
		_ezero = .;
	} > ram

	.stack (NOLOAD) :
	{
		. = ALIGN(8);
		. = . + STACK_SIZE;
		. = ALIGN(8);
		_estack = .;
	} > ram
}
