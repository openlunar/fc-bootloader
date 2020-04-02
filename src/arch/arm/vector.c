
#include "vector.h"
#include "config.h"
#include "common.h"

void blocking_handler( void )
{
	while (1);
}

void null_handler( void )
{
	/* Do nothing. */
}

// Linker-provided external variables
extern uint32_t _etext; // End of text section (start of data section to copy to RAM)
extern uint32_t _srelocate; // Start of relocate section
extern uint32_t _erelocate; // End of relocate section
extern uint32_t _szero; // Start of zero-initialized section
extern uint32_t _ezero; // End of zero-initialized section
extern uint32_t _estack; // End of stack section (stack start address)

// Vector table
// TODO: (90) [robustness] @nth Enforce vector table alignment requirements...somehow (here or in linker script)
__attribute__((section(".vectors")))
vector_table_t vector_table = {
	.initial_sp_value = &_estack,
	.reset = reset_handler,
	.nmi = null_handler,
	.hard_fault = blocking_handler,

// I've just been copying this but really I only need support for CM7
/* Those are defined only on CM3 or CM4 */
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
	.memory_manage_fault = blocking_handler,
	.bus_fault = blocking_handler,
	.usage_fault = blocking_handler,
	.debug_monitor = null_handler,
#endif

	.sv_call = null_handler,
	.pend_sv = null_handler,
	.systick = null_handler
};

// Should this be a naked function (?)
// void __attribute__((weak, naked)) reset_handler( void )
// void __attribute__((optimize("-fno-tree-loop-distribute-patterns"))) reset_handler()
void __attribute__((naked)) reset_handler()
{
#if defined(CONFIG_RAM_BUILD)
	// Set VTOR to FlexRAM vector table
	// TODO: (60) [refactor] @magic Define for VTOR address
	MMIO32(0xE000ED08) = (uint32_t)&vector_table;

	// Manually set the stack pointer since it's not being initialized while I load out of RAM
	__set_MSP( (uint32_t)&_estack );
#endif // defined(CONFIG_RAM_BUILD)

	// Copy data to SRAM (static data)
	// /*volatile*/ unsigned *src, *dest;
	uint32_t *src, *dest;

	// Is volatile necessary here (?)
	for ( src = &_etext, dest = &_srelocate; dest < &_erelocate; src++, dest++ ) {
		*dest = *src;
	}

	// Initialize bss (zero-initialized) section
	for ( dest = &_szero; dest < &_ezero; dest++ ) {
		*dest = 0;
	}

	// Call the main function
	main();

	// The bootloader will transfer control to the main application
	// In the future it should probably be that if the bootloader main() ever returns that the system resets
}
