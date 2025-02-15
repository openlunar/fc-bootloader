
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef FLASH_H
#define FLASH_H

#include "common.h"
#include <stdint.h>

typedef struct {
	uint32_t start;
	uint32_t end;
} flash_partition_t;

// NOTE: Using a uint16_t for "page" arguments supports up to 32M of flash at a page level; switching to uint32_t probably advisable for future-proofing *plus* there's no real advantage to using 16-bit values here

int flash_init();

int flash_get_partition( uint32_t id, flash_partition_t * partition );

// NOTE: Page-level erase granularity *not* supported by V71; I could make the V71 version do the smallest sector erase that meets the requirements *or* return an error if the pages don't correspond to sectors
// int flash_erase_range( uint16_t page_start, uint16_t page_end );

int flash_write_page( uint32_t id, uint8_t * page_buffer, uint16_t page );

int flash_erase_partition( uint32_t id );

#endif // FLASH_H

#ifdef __cplusplus
}
#endif
