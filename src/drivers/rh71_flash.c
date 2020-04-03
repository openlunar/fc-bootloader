
#include "flash.h"

#include "config.h"

#include "rh71_pmc.h"


// Register interface
#define HEFC_BASE	0x40004000

#define HEFC_FMR	MMIO32(HEFC_BASE+0x00)
#define HEFC_FCR	MMIO32(HEFC_BASE+0x04)
#define HEFC_FSR	MMIO32(HEFC_BASE+0x08)
#define HEFC_FRR	MMIO32(HEFC_BASE+0x0C)

// HEFC_FCR
#define HEFC_FCR_FKEY_PASSWD	0x5A

#define HEFC_FCR_FKEY(fkey) ((fkey & 0xFF) << 24)
#define HEFC_FCR_FARG(farg) ((farg & 0xFFFF) << 8)
#define HEFC_FCR_FCMD(fcmd) (fcmd & 0xFF)

#define HEFC_FCR_FARG_SHIFT 8
#define HEFC_FCR_FARG_MASK (0xFFFF << HEFC_FCR_FARG_SHIFT)

#define HEFC_CMD_EPA_ARG_NP(np)	(np & 0x03) // Erase size in pages
#define HEFC_CMD_EPA_ARG_NP_4	HEFC_CMD_EPA_ARG_NP(0) // 4
#define HEFC_CMD_EPA_ARG_NP_8	HEFC_CMD_EPA_ARG_NP(1) // 8
#define HEFC_CMD_EPA_ARG_NP_16	HEFC_CMD_EPA_ARG_NP(2) // 16
#define HEFC_CMD_EPA_ARG_NP_32	HEFC_CMD_EPA_ARG_NP(3) // 32

#define HEFC_CMD_EPA_ARG_SP(sp)	((sp & 0xFFF) << 2) // Start page for page erase

#define HEFC_CMD_EPA_ARG(sp,np)	(HEFC_CMD_EPA_ARG_SP(sp) | HEFC_CMD_EPA_ARG_NP(np))

// HEFC_FSR

#define HEFC_FSR_FRDY		(1 << 0)
#define HEFC_FSR_FCMDE		(1 << 1)
#define HEFC_FSR_FLOCKE		(1 << 2)
#define HEFC_FSR_FLERR		(1 << 3)

// Write sequence:
//
// 1. Write the data to be programmed in the latch buffer.
// 2. Write the programming command in HEFC_FCR. This automatically clears the
//    HEFC_FSR.FRDY bit.
// 3. When Flash programming is completed, the HEFC_FSR.FRDY bit rises. If an
//    interrupt has been enabled by setting the HEFC_FMR.FRDY bit, the interrupt
//    line of the HEFC is activated.
//
// Three errors can be detected in HEFC_FSR after a programming sequence:
//
// - Command Error: A bad keyword has been written in HEFC_FCR.
// - Lock Error: The page to be programmed belongs to a locked region. A command
//   must be run previously to unlock the corresponding region.
// - Flash Error: When programming is completed, the WriteVerify test of the
//   Flash memory has failed.
//
// NOTE:
// - One optimization is to check if the page is already erased before erasing
//   it (also useful for programming partial pages)
//
// Erase:
//
// - Erase All Memory (EA): All memory is erased. The processor must not fetch
//   code from the Flash memory.
// - Erase Pages (EPA): 4, 8, 16, or 32 pages are erased in the Flash sector
//   selected. The first page to be erased is specified in the FARG[15:2] field
//   of the HEFC_FCR. The first page number must be a multiple of 8, 16, or 32
//   depending on the number of pages to erase simultaneously.
// - Erase Sector (ES): A full memory sector is erased. Sector size depends on
//   the Flash memory. HEFC_FCR.FARG must be set with a page number that is in
//   the sector to be erased.
//
// Erase sequence:
//
// 1. Erase starts immediately one of the erase commands and the FARG field are
//    written in HEFC_FCR. For the EPA command, the two lowest bits of the FARG
//    field define the number of pages to be erased (FARG[1:0]), see table
//    below.
// 2. When erasing is completed, the HEFC_FSR.FRDY bit rises. If an interrupt
//    has been enabled by setting the HEFC_FMR.FRDY bit, the interrupt line of
//    the interrupt controller is activated.
//
// Three errors can be detected in HEFC_FSR after an erasing sequence:
// - Command Error: A bad keyword has been written in HEFC_FCR.
// - Lock Error: At least one page to be erased belongs to a locked region. The
//   erase command has been refused, no page has been erased. A command must be
//   run previously to unlock the corresponding region.
// - Flash Error: At the end of the erase period, the EraseVerify test of the
//   Flash memory has failed.

// -- Internal -------------------------------------------------------------- //

typedef enum {
	HEFC_CMD_GETD = 0x00,	// Get Flash descriptor
	HEFC_CMD_WP = 0x01,		// Write page
	HEFC_CMD_WPL = 0x02,	// Write page and lock
	// HEFC_CMD_EWP = 0x03,	// Erase page and write page
	// HEFC_CMD_EWPL = 0x04,	// Erase page and write page then lock
	HEFC_CMD_EA = 0x05,		// Erase all
	HEFC_CMD_EP = 0x06,		// Erase page
	HEFC_CMD_EPA = 0x07,	// Erase pages
	HEFC_CMD_SLB = 0x08,	// Set lock bit
	HEFC_CMD_CLB = 0x09,	// Clear lock bit
	HEFC_CMD_GLB = 0x0A,	// Get lock bit
	HEFC_CMD_SGPB = 0x0B,	// Set GPNVM bit
	HEFC_CMD_CGPB = 0x0C,	// Clear GPNVM bit
	HEFC_CMD_GGPB = 0x0D,	// Get GPNVM bit
	// HEFC_CMD_STUI = 0x0E,	// Start read unique identifier
	// HEFC_CMD_SPUI = 0x0F,	// Stop read unique identifier
	HEFC_CMD_GCALB = 0x10,	// Get CALIB bit
	// HEFC_CMD_ES = 0x11,		// Erase sector
	HEFC_CMD_WUS = 0x12,	// Write user signature
	HEFC_CMD_EUS = 0x13,	// Erase user signature
	HEFC_CMD_STUS = 0x14,	// Start read user signature
	HEFC_CMD_SPUS = 0x15	// Stop read user signature
} hefc_cmd_enum_t;


// This is neat and will work but it's portable (endian issues). Could maybe use
// gcc attribute `scalar_storage_order`.
// 
// typedef union __attribute__((packed, aligned(4))) {
// 	uint32_t reg;
// 	struct /*packed*/ {
// 		uint8_t fcmd;
// 		uint16_t farg;
// 		uint8_t fkey;
// 	} fields;
// } hefc_cmd_t;

/*
 * @brief      Construct, validate, and execute a flash command
 *
 * @param[in]  cmd   The command
 *
 * @return     { description_of_the_return_value }
 */
// int hefc_command( hefc_cmd_enum_t cmd, uint16_t arg )
// {
// 	// Initialize register value with access key and command
// 	uint32_t fcr = HEFC_FCR_FKEY(HEFC_FKEY_PASSWD) | HEFC_FCR_FCMD(cmd);

// 	// Command and argument validation
// 	switch ( cmd ) {
// 		case HEFC_CMD_EA:
// 		case HEFC_CMD_GETD:
// 		case HEFC_CMD_GLB:
// 		case HEFC_CMD_GGPB:
// 		case HEFC_CMD_STUI:
// 		case HEFC_CMD_SPUI:
// 		case HEFC_CMD_GCALB:
// 		case HEFC_CMD_WUS:
// 		case HEFC_CMD_EUS:
// 		case HEFC_CMD_STUS:
// 		case HEFC_CMD_SPUS:
// 			// No argument needed; farg must be 0
// 			break;
// 		case HEFC_CMD_ES:
// 			// FARG must be written with any page number within the sector to be
// 			// erased

// 			break;
// 		case HEFC_CMD_EPA:
// 			// - FARG[15:2] Start page
// 			// - FARG[1:0]  Number of pages to be erased
// 			//   - 0: 4 pages, FARG[15:2] = Page_Number / 4
// 			//     - 2 KB total
// 			//     - Only valid for 8 KB sectors
// 			//   - 1: 8 pages, FARG[15:3] = Page_Number / 8
// 			//     - 4 KB total
// 			//     - FARG[2] = 0
// 			//     - Only valid for 8 KB sectors
// 			//   - 2: 16 pages, FARG[15:4] = Page_Number / 16
// 			//     - 8 KB total
// 			//     - FARG[3:2] = 0
// 			//   - 3: 32 pages, FARG[15:5] = Page_Number / 32
// 			//     - 16 KB total
// 			//     - FARG[4:2] = 0
// 			//     - Not valid for 8 KB sectors
			
// 			break;
// 		default:
// 			// Invalid command
// 			return (-1);
// 	}

// 	// Write command to command register
// 	HEFC_FCR = fcr; 

// 	// ...

// 	return 0;
// }

// -- API ------------------------------------------------------------------- //

// RH71 Flash
// 
// Memory size   = 128 KB
// Page size     = 256 bytes
// Page count    = 512 (10 bits)
// Sector count  = xxx

int flash_init()
{
	// The write/erase clock is provided by the PMC. It must be configured at 2 MHz and enabled before using the erase / write feature on Flash memory.
	PMC_PCR = ((1 << 28 /* EN */) | (1 << 12 /* CMD - Write Mode */) | (50 /* PID8 */))
		| (1 << 29 /* GCLKEN */) | (1 << 8 /* GCLKCSS : MAIN_CLK */) | (1 << 20 /* GCLKDIV */);

	// PMC_PCR_EN_Msk | PMC_PCR_CMD_Msk | PMC_PCR_PID(50)  /* HEFC */
 //        | PMC_PCR_GCLKEN_Msk | PMC_PCR_GCLKCSS_MAIN_CLK | PMC_PCR_GCLKDIV(5);

	return 0;
}

int flash_erase_range( uint16_t page_start, uint16_t page_end )
{
	// TODO: (2) [refactor] @magic Replace "512" with (?) CONFIG value for MAX_PAGE_NO
	if ( (page_start > 512) || (page_end > 512) ) {
		return (-1);
	}

	if ( page_end < page_start ) {
		return (-1);
	}

	// TODO: (60) [feature] @nth Be more clever about usage of EP vs EPA for page ranges

	uint32_t fcr = HEFC_FCR_FKEY(HEFC_FCR_FKEY_PASSWD) | HEFC_FCR_FCMD(HEFC_CMD_EP);
	uint32_t fsr;
	
	for ( ; page_start <= page_end; page_start++ ) {
		fcr &= ~HEFC_FCR_FARG_MASK; // Clear page argument
		fcr |= HEFC_FCR_FARG( page_start ); // Set page argument

		// Write command to command register
		HEFC_FCR = fcr;

		// Wait for erase to complete (HEFC_FSR.FRDY)
		while ( ! ((fsr = HEFC_FSR) & HEFC_FSR_FRDY) );

		// fsr &= (HEFC_FSR_FCMDE | HEFC_FSR_FLOCKE | HEFC_FSR_FLERR);
		if ( fsr & (HEFC_FSR_FCMDE | HEFC_FSR_FLOCKE | HEFC_FSR_FLERR) ) {
			return (-fsr); // Some error occurred during the operation
		}
	}

	return 0;
}

// int flash_erase_app()
// {
// 	// TODO: (60) [feature] @nth Check that the page isn't already erased

// 	// The app is located at 0x00404000 (0x4000) and is 0x1000 bytes (4 KB)
// 	// App start sector : 0
// 	// App start page   : 32
// 	//
// 	// So that's a EPA (erase pages) command with (32,2) as the argument
// 	// (start page = 32, erase 16 pages (smallest size available for 128 KB
// 	// sectors))

// 	// Issue the erase sector command
// 	// hefc_cmd_epa( 32, HEFC_CMD_EPA_FARG_NP_16 );
// 	// hefc_command( HEFC_CMD_ES,  );
// 	uint32_t fcr = HEFC_FCR_FKEY(HEFC_FCR_FKEY_PASSWD) | HEFC_FCR_FCMD(HEFC_CMD_EPA);
// 	// Ahhhhh. I'll probably want a function or macro that does the proper division based on *_NP_* argument, e.g. NP_16 divides page number by 16, the actual argument is in [15:3]
// 	// fcr |= HEFC_FCR_FARG( HEFC_CMD_EPA_ARG((2 << 2), HEFC_CMD_EPA_ARG_NP_16 ) );
// 	fcr |= HEFC_FCR_FARG( HEFC_CMD_EPA_ARG((0 << 2), HEFC_CMD_EPA_ARG_NP_16 ) );

// 	// This should be checking the FRDY bit *before* executing a command
// 	// The command execution flow chart should probably be implemented in the hefc_command() (or per-command) function

// 	// Write command to command register
// 	HEFC_FCR = fcr;

// 	// Wait for erase to complete (HEFC_FSR.FRDY)
// 	uint32_t fsr;
// 	while ( ! ((fsr = HEFC_FSR) & HEFC_FSR_FRDY) );

// 	// fsr &= (HEFC_FSR_FCMDE | HEFC_FSR_FLOCKE | HEFC_FSR_FLERR);
// 	if ( fsr & (HEFC_FSR_FCMDE | HEFC_FSR_FLOCKE | HEFC_FSR_FLERR) ) {
// 		return (-fsr); // Some error occurred during the operation
// 	}

// 	return 0;
// }

// TODO: Probably want a timeout and to indicate an error if it times out
// Blocking wait for FSR
static inline uint32_t wait_fsr_frdy() {
	uint32_t fsr;
	while ( ! ((fsr = HEFC_FSR) & HEFC_FSR_FRDY) );
	return fsr;
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
//
// We're treating the memory available in the V71 like it's the RH71, so only
// the first 128 Kb. Number of pages to be erased using EPA is 4, 8, 16, or 32 -
// however, only 16 and 32 are valid outside of the 8 Kb sectors. Since the
// bootloader occupies the two 8 Kb sectors that means there is no use for the 4
// and 8 page erase method.
//
// So, erase sizes are 8 Kb or 16 Kb. This means the 8 Kb (16 pages) needs to be
// used to achieve the granularity needed to erase the app (56 Kb / 8 = 7 EPA);
// the erase has to be aligned to the erase size (i.e. you can't erase an 8 Kb
// chunk from an arbitrary start page, the start page must be on an 8 Kb
// boundary).
//
// RH71 page size = 256 bytes
// So, the memory regions defined in pages:
// 0   - 63		Bootloader
// 64  - 287	APP_1
// 288 - 511	APP_2

// Configuration per "app":
// - page_start
// - page_end
// - => valid page range (for error handling)
// - flash_offset (e.g. 0x4000)

// NOTE: For now this just erases the first app
// TODO: A better implementation might check that the page range isn't already erased
int flash_erase_partition( uint32_t id )
{
	// TODO: Check that the page isn't already erased
	
	flash_partition_t partition;
	if ( flash_get_partition( id, &partition ) < 0 ) {
		return (-1); // Partition not found
	}

	// NOTE: This could be slightly more efficient if I used a more clever erase algorithm. Instead of doing 7 16-page erases I could do 2 32-page erases and then 3 16-page erases. I would need to compare the erase time - it might be constant time and in that case this improvement wouldn't really matter...
	// 
	// 16 pages (8 KB total)
	// - ARG[15:4] = Page_Number / 16
	// - FARG[3:2] = 0
	uint32_t page;
	uint32_t fsr;
	uint32_t fcr;
	for ( page = (partition.start / CONFIG_PAGE_SIZE); page < (partition.end / CONFIG_PAGE_SIZE); page += 16 ) {
		// Wait for flash to be available
		wait_fsr_frdy();

		// Construct and issue command
		fcr = HEFC_FCR_FKEY(HEFC_FCR_FKEY_PASSWD)
			| HEFC_FCR_FCMD(HEFC_CMD_EPA)
			| HEFC_FCR_FARG( HEFC_CMD_EPA_ARG((page & (~0xF)), HEFC_CMD_EPA_ARG_NP_16 ) );
		HEFC_FCR = fcr;

		// Wait for command to complete
		fsr = wait_fsr_frdy();

		// Check that the erase succeeded
		if ( fsr & (HEFC_FSR_FCMDE | HEFC_FSR_FLOCKE | HEFC_FSR_FLERR) ) {
			return (-fsr); // Some error occurred during the operation
		}
	}

	return 0;
}

// int write_page_buffer( frame_t * frame, uint8_t * page_buffer );
// Must be a full page
int flash_write_page( uint32_t id, uint8_t * page_buffer, uint16_t page )
{
	flash_partition_t partition;
	if ( flash_get_partition( id, &partition ) < 0 ) {
		return (-1); // Partition not found
	}

	// Validate page argument
	uint32_t page_offset = (page * CONFIG_PAGE_SIZE);
	if ( page_offset >= partition.end ) {
		return (-2); // Invalid page number
	}

	// There are probably alignment requirements for this sort of thing; I know
	// from the datasheet that using DMA requires 32-bit alignment
	//
	// If a single byte has to be written in a 32-bit word, the rest of the word
	// must be written with ones.

	// The data has been written from a binary file over a byte-oriented
	// interface into a byte-based buffer; the original endianness of the data
	// has been maintained. Thus, it should be valid to just copy 4 bytes of
	// data at a time, assembling words in little-endian format (for ARM)...?

	// NOTE: This assumes that the page_buffer pointer passed in aligned to 4-byte boundary
	// TODO: Enforce alignment / assert alignment
	uint32_t i;
	volatile uint32_t * app_start_addr = (volatile uint32_t *)(partition.start + page_offset);
	for ( i = 0; i < CONFIG_PAGE_SIZE; i += 4 ) {
		*app_start_addr = *(uint32_t *)&page_buffer[i];

		// Increment address
		app_start_addr++;
	}

	// Synchronize pipeline
	// TODO: Is the ISB necessary (?)
	__ISB();
	__DSB();

	uint32_t fsr;
	while ( ! ((fsr = HEFC_FSR) & HEFC_FSR_FRDY) );

	// Commit page
	page += (partition.start / CONFIG_PAGE_SIZE);
	volatile uint32_t fcr = HEFC_FCR_FKEY(HEFC_FCR_FKEY_PASSWD)
		| HEFC_FCR_FCMD(HEFC_CMD_WP)
		| HEFC_FCR_FARG( page );

	// Write command to command register
	HEFC_FCR = fcr;

	while ( ! ((fsr = HEFC_FSR) & HEFC_FSR_FRDY) );

	if ( fsr & (HEFC_FSR_FCMDE | HEFC_FSR_FLOCKE | HEFC_FSR_FLERR) ) {
		return (-fsr); // Some error occurred during the operation
	}

	return 0;
}
