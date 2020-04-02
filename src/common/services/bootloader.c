
#include "config.h"
#include "moon/services/bootloader.h"

#include "crc.h"
#include "printf.h"

// TODO: (0) [refactor] Better name for this header / replace this idea
#include "system.h"

#include "flash.h"

// TODO: The return types for most methods is 'int8_t'; however, it would probably be more clear / useful to have an enum mapped to error values. This is a good example of where mapping from the internal representation (e.g. int32_t / enum) to the "on wire" representation (probably 'int8_t') will be an interesting implementation detail. Oh, the TODO is to swap these out for enums at some point.

// Global page buffer
// TODO: (20) [refactor] @nth This would probably be even more efficient if aligned to cache line size, 32 bytes
// TODO: (20) [refactor] Possibly useful to union this with a 32-bit element array
// static uint8_t page_buffer_g[CONFIG_PAGE_SIZE] __attribute__ ((aligned (4)));

// TODO: (5) Replace '32' with e.g. CONFIG_CACHE_LINE_SIZE
typedef union __attribute__((aligned (32))) {
	uint8_t u8[CONFIG_PAGE_SIZE];
	uint32_t u32[CONFIG_PAGE_SIZE / sizeof(uint32_t)];
} page_buffer_t;

static page_buffer_t page_buffer_g;

// Configuration per "app":
// - page_no (max) (min is always 0)

void bl_ping()
{
	printf( "ping\n\r" );
}

// TODO: (10) [refactor] @error_handling Is "data_len" of 0 an error (?)
// TODO: (10) [refactor] @error_handling I should probably check for data == NULL (?); the call comes from generated code, but someone could also use these manually (that's the whole point)
// TODO: (10) [api] Define return type values (e.g. invalid offset, invalid data_len (or just the combination of them))
int8_t bl_writePageBuffer( uint16_t offset, uint8_t data_len, const uint8_t * data )
{
	printf( "write_page_buf @ $%04X len %u\n\r", offset, data_len );

	if ( ! data ) {
		return (-1);
	}

	// Check offset / length arguments
	if ( (offset + data_len) > CONFIG_PAGE_SIZE ) {
		return (-2);
	}

	uint32_t i;
	for ( i = 0; i < data_len; i++ ) {
		page_buffer_g.u8[offset + i] = data[i]; // Write data to page
	}

	return 0;
}

void bl_erasePageBuffer(void)
{
	printf( "erasePageBuffer\n\r" );

	// TODO: (70) [feature] @nth Replace this with memset
	// NOTE: Optimized slightly with the knowledge that CONFIG_PAGE_SIZE is a multiple of 4 and that the page_buffer is aligned to 4
	uint32_t i;
	for ( i = 0; i < (CONFIG_PAGE_SIZE / 4); i++ ) {
		// *(uint32_t *)&page_buffer_g[i*4] = 0xFFFFFFFF; // Clear page data
		page_buffer_g.u32[i] = 0xFFFFFFFF;
	}
}

// Ok, memory layout:
// - Designed for SAMRH71 (smallest internal flash)
// - Internal flash: 128 Kb
// 
// Bootloader size: 16 Kb (I'd prefer 8 Kb but printf is taking up some space)
// App size: (128 - 16) / 2 = 56 Kb
// 
// 0x00000 - 0x03FFF	Bootloader
// 0x04000 - 0x11FFF	APP_1
// 0x12000 - 0x1FFFF	APP_2

int8_t bl_eraseApp( AppId app_id )
{
	printf( "eraseApp %i\n\r", app_id );

	uint32_t ret;
	ret = flash_erase_partition( app_id );

	return (int8_t)ret;
}

// TODO: (10) [api] Rename argument "crc" to "page_crc" for clarity
// NOTE: The CRC is calculated on the assumption that all unset values in the page buffer are FF (matches unprogrammed flash); use erasePageBuffer before programming a page (so you don't have to send FF over the wire needlessly).
int8_t bl_writePage( AppId app_id, uint16_t page_no, uint32_t crc )
{
	printf( "writePage %i $%04X $%08X\n\r", app_id, page_no, crc );

	// Validate page_no doesn't exceed flash size
	// TODO: [refactor] @error_handling @magic Hmm. Should this error instead be handled at the 'flash_write_page' level (?). Magic number for RH71
	// if ( page_no > 512 ) {
	// 	// TODO: Define return type values; maybe an enum
	// 	return (-1);
	// }

	// Verify page CRC against buffer
	uint32_t buffer_crc = crc_32( page_buffer_g.u8, CONFIG_PAGE_SIZE );
	printf( "  buffer_crc $%08X\n\r", buffer_crc );

	if ( crc != buffer_crc ) {
		return (-2);
	}

	int ret = flash_write_page( app_id, page_buffer_g.u8, page_no );

	// TODO: Should we CRC the page after writing it (?) I just discovered the case where you didn't erase the page and then when you write to it you get the combination of the old data and the new...
	// TODO: Should maybe check for / warn for / error for writing to a page that hasn't been erased (keep a bitmask of erase state)

	return (int8_t)ret;
}

int8_t bl_setBootAction(BootAction action)
{
	printf( "setBootAction %i\n\r", action );

	// Map action to partition
	// NOTE: This is a temporary measure; this doesn't represent real system behavior (because right now this behavior is volatile)
	uint32_t id;
	switch ( action ) {
		case BOOT_APP_1:
			id = APP_1;
			break;
		case BOOT_APP_2:
			id = APP_2;
			break;
		case BOOT_BOOTLOADER:
			id = BOOTLOADER;
			break;
		case BOOT_NONE: // Currently not supported
		default:
			return (-1); // Invalid boot action 
	}

	return sys_set_boot_action( id );
}

// TODO: (0) [api] "boot" function that causes the bootloader to attempt to execute whatever the configured bootAction is

// TODO: What if the boot action is "bootloader" - does this return 0 (?) - it could also just reboot the bootloader
int8_t bl_boot()
{
	printf( "boot\n\r" );

	// Validate CRC of application
	// ...
	
	// Set global state variable for "perform boot"
	return (int8_t)sys_set_boot_enable();
}
