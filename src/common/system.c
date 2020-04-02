
#include "system.h"
#include "config.h"
#include "common.h"
#include "printf.h"

#include "flash.h"

static bool boot_enable_g = false;

#define BOOT_INVALID_ENTRY 0xFFFFFFFF
static uint32_t boot_entry_g = BOOT_INVALID_ENTRY; // Initialize to invalid entry point

int sys_set_boot_action( uint32_t id )
{
	flash_partition_t partition;
	if ( flash_get_partition( id, &partition ) < 0 ) {
		return (-1); // Partition not found
	}

	boot_entry_g = partition.start;

	return 0;
}

int sys_set_boot_enable()
{
	if ( boot_entry_g == BOOT_INVALID_ENTRY ) {
		return (-1);
	}

	boot_enable_g = true;

	return 0;
}

bool sys_get_boot_enable()
{
	return boot_enable_g;
}

__attribute__( ( naked, noreturn ) ) void BootJumpASM( uint32_t SP, uint32_t RH )
{
	// Suppress warnings related to unused parameter
	(void)SP;
	// (void)RH;

	__asm__("MSR	MSP,r0"); // Set stack pointer, SP passed in r0
	// NOTE: https://www.keil.com/pack/doc/CMSIS/Core/html/using_VTOR_pg.html
	// TODO: Replace hard-coded reference to VTOR address
	// __disable_irq(); // I don't think this is necessary here - interrupts *haven't* been enabled
	MMIO32(0xE000ED08) = (uint32_t)&RH; // Set new vector table
	__DSB();
	// __enable_irq();
	__asm__("BX		r1"); // Branch to application entry point, RH passed in RH
}

void sys_boot_poll()
{
	// Only boot if enabled
	if ( ! sys_get_boot_enable() ) {
		return;
	}

	volatile uint32_t * entry = (volatile uint32_t *)(boot_entry_g);
	printf("Booting from $%08x\n\r", (uint32_t)entry );
	BootJumpASM( entry[0], entry[1] );
}
