
#include <config.h>

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* This is important for loading into RAM - if you don't specify the ENTRY then it will set the entry to the load point of ".text" which is the vector table, meaning it will execute until it hits the first real instruction following the ".vectors" section, which could be roughly anything. */
ENTRY(reset_handler)

// This linker script supports executing the bootloader from flash or RAM. In
// the case where it executes in RAM, the RAM is split into a "bootloader code"
// section and a "RAM" section.
//
// The bootloader will only ever occupy a small portion of FLASH (TBD: ideally
// 8-32K); the remaining flash is allocated to application storage (likely two
// slots).

// TODO: Think about any alignment requirements for all of this

// NOTE: I think presently this is independent of load method
#define ROM_SIZE		(CONFIG_BOOTLOADER_SIZE)

// TODO: Should this be calculated outside of the linker script (?) - seems like this information might be useful for other code and should probably only be calculated once...
#if defined(CONFIG_RAM_BUILD)
// Execute from RAM
	#define ROM_ADDR	(CONFIG_SRAM_BASE_ADDRESS)

	#define RAM_ADDR	(CONFIG_SRAM_BASE_ADDRESS + CONFIG_BOOTLOADER_SIZE)
	#define RAM_SIZE	(CONFIG_SRAM_SIZE - CONFIG_BOOTLOADER_SIZE)

#else // ! defined(CONFIG_RAM_BUILD)
// Execute from FLASH
	#define ROM_ADDR	(CONFIG_FLASH_BASE_ADDRESS)

	#define RAM_ADDR	(CONFIG_SRAM_BASE_ADDRESS)
	#define RAM_SIZE	(CONFIG_SRAM_SIZE)
#endif // defined(CONFIG_RAM_BUILD)

// Memory Spaces Definitions
// TODO: TCM memory
MEMORY
{
	FLASH (rw)  : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
	SRAM  (rwx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
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
		. = . + CONFIG_STACK_SIZE;
		. = ALIGN(8);
		_estack = .;
	} > SRAM
}
