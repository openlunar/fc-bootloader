
#include "config.h"
#include "common.h"
#include "vector.h"
#include "rh71_pio.h"
#include "rh71_usart.h"
#include "rh71_flash.h"
#include "sll.h"
#include "command.h"

#include <stdint.h>

// TODO: Move vector table and start-up code into arch/arm

// Linker-provided external variables
extern uint32_t _etext; // End of text section (start of data section to copy to RAM)
extern uint32_t _srelocate; // Start of relocate section
extern uint32_t _erelocate; // End of relocate section
extern uint32_t _szero; // Start of zero-initialized section
extern uint32_t _ezero; // End of zero-initialized section
extern uint32_t _estack; // End of stack section (stack start address)

void reset_handler();
int main();

// Vector table
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
	// TODO: Define for VTOR address
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

// __attribute__( ( naked, noreturn ) ) void BootJumpASM( uint32_t SP, uint32_t RH )
// {
// 	// Suppress warnings related to unused parameter
// 	(void)SP;
// 	(void)RH;

// 	__asm__("MSR	MSP,r0"); // Set stack pointer, SP passed in r0
// 	__asm__("BX		r1"); // Branch to application entry point, RH passed in RH
// }

#define WDT_BASE	0x40100250
#define WDT_MR		MMIO32((WDT_BASE) + 0x04)

// #define RTC_BASE	0x400E1860
// #define RTC_TIMR	MMIO32((RTC_BASE) + 0x08)

// LED0 - PB19 - PWMCI_PWMH0
// LED1 - PB23 - PWMCI_PWML0
// LED2 - PF19 - NWR0
// LED3 - PF20 - NWR1

// uint8_t tohexchar( uint8_t d )
// {
// 	if ( d < 10 ) {
// 		return '0' + d;
// 	} else if ( d < 16 ) {
// 		return 'A' + (d - 10);
// 	} else {
// 		return 'X';
// 	}

// 	/* -- Not Reached -- */
// }

// void print_u8_hex( uint8_t val )
// {
// 	uint8_t s[2];
// 	s[0] = tohexchar( val >> 4 );
// 	s[1] = tohexchar( val & 0xF );
// 	usart_write( 1, s, 2 );
// }

// void print_nl()
// {
// 	usart_write( 1, (uint8_t *)"\n\r", 2 );
// }

int main()
{
	// Disable watchdog (WDT0); WDT_MR can only be written once after reset
	// TODO: Remove this - the application will configure the WDT; the bootloader will just have to deal with this for now (16s timeout)
	WDT_MR |= (1 << 15 /* WDDIS */);

	// Configure PB19 (LED0) as an output
	PIOB_MSKR = (1 << 19);
	PIOB_CFGR = (1 << 8); // DIR: Output

	// Maybe we UART quickly?
	usart_init();
	// usart_write( 1, (uint8_t *)"OLF RH71 Bootloader\n\r", 21 );

	// Write a page to flash
	// volatile uint32_t i;
	// uint8_t page_buffer[PAGE_SIZE];
	// for ( i = 0; i < PAGE_SIZE; i++ ) {
	// 	page_buffer[i] = i;
	// }
	rh71_flash_init();
	// // rh71_flash_erase_app();
	// rh71_flash_write_page( page_buffer, 0 );

	// NOTE: To save data space for now there will be no copying of data from link layer buffer to a higher-layer buffer; to decouple the link layer and command parser I'm going to pass references to the data buffer and data length from the sll_frame_t.
	sll_decode_frame_t frame;
	sll_init( &frame );

	int ret;
	uint8_t c;
	// uint8_t i;
	while (1) {
		// Read a character from USART and pass it to the sll decoder
		usart_read( 1, &c );
		ret = sll_decode( &frame, c );

		// TODO: Defines for sll_decode return values (e.g. SLL_RET_OK)
		if ( ret == 1 ) {
			// TODO: Better name (?)
			ret = cmd_exe( frame.data, frame.length );
		}

		// Check state and flip it
		if ( PIOB_ODSR & (1 << 19) ) {
			PIOB_CODR = (1 << 19);
		} else {
			PIOB_SODR = (1 << 19);
		}
	}

	// volatile uint32_t * user_app = (volatile uint32_t *)0x00404000;

	// Branch to the main application in flash
	// BootJumpASM( user_app[0], user_app[1] );

	return 0;
}
