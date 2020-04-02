
#include "flash.h"
#include "config.h"

// TODO: Should probably validate these configs somehow...
// TODO: Macros to convert addresses into pages
// TODO: Stretch goal: device tree configuration

#define PARTITION_ID_BOOTLOADER		255
#define PARTITION_ID_APP1			0
#define PARTITION_ID_APP2			1

#define PARTITION_APP_SIZE			0xE000
#define PARTITION_BOOTLOADER_SIZE	(CONFIG_BOOTLOADER_SIZE * 1024)

#define PARTITION_BOOTLOADER_START	(CONFIG_FLASH_BASE_ADDRESS + 0x00000)
#define PARTITION_BOOTLOADER_END	(PARTITION_BOOTLOADER_START + PARTITION_BOOTLOADER_SIZE)

#define PARTITION_APP1_START		PARTITION_BOOTLOADER_END
#define PARTITION_APP1_END			(PARTITION_APP1_START + PARTITION_APP_SIZE)

#define PARTITION_APP2_START		PARTITION_APP1_END
#define PARTITION_APP2_END			(PARTITION_APP2_START + PARTITION_APP_SIZE)

// Validate partition configuration
#if (PARTITION_APP2_END > (CONFIG_FLASH_BASE_ADDRESS + (CONFIG_FLASH_SIZE * 1024)))
	#error "Partition table is invalid, PARTITION_APP2_END exceeds CONFIG_FLASH_SIZE"
#endif

// TODO: Validate alignment requirements

#if defined(CONFIG_RAM_BUILD)
	static flash_partition_t partion_bootloader = {
		.start = PARTITION_BOOTLOADER_START,
		.end = PARTITION_BOOTLOADER_END
	};
#endif // defined(CONFIG_RAM_BUILD)

static flash_partition_t partion_app1 = {
	.start = PARTITION_APP1_START,
	.end = PARTITION_APP1_END
};

static flash_partition_t partion_app2 = {
	.start = PARTITION_APP2_START,
	.end = PARTITION_APP2_END
};

void __copy_partition_struct( flash_partition_t * src, flash_partition_t * dest )
{
	dest->start = src->start;
	dest->end = src->end;
}

// This function copies the partition data (instead of returning a pointer to it) so that the user can't accidentally change it
int flash_get_partition( uint32_t id, flash_partition_t * partition )
{
	switch ( id ) {
#if defined(CONFIG_RAM_BUILD)
// NOTE: Access to the bootloader partition is *only* allowed in a RAM-based build; this is for the purpose of bootloading the bootloader
		case PARTITION_ID_BOOTLOADER:
			__copy_partition_struct( &partion_bootloader, partition );
			break;
#endif // defined(CONFIG_RAM_BUILD)
		case PARTITION_ID_APP1:
			__copy_partition_struct( &partion_app1, partition );
			break;
		case PARTITION_ID_APP2:
			__copy_partition_struct( &partion_app2, partition );
			break;
		default:
			return (-1); // Partition doesn't exist
	}

	return 0;
}
