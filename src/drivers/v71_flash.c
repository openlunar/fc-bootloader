
#include "v71_flash.h"

// TODO: Move these into a device header (?)

// Register interface
#define EEFC_BASE	0x400E0C00

#define EEFC_FMR	MMIO32(EEFC_BASE+0x00)
#define EEFC_FCR	MMIO32(EEFC_BASE+0x04)
#define EEFC_FSR	MMIO32(EEFC_BASE+0x08)
#define EEFC_FRR	MMIO32(EEFC_BASE+0x0C)

// EEFC_FCR
#define EEFC_FCR_FKEY_PASSWD	0x5A

#define EEFC_FCR_FKEY(fkey) ((fkey & 0xFF) << 24)
#define EEFC_FCR_FARG(farg) ((farg & 0xFFFF) << 8)
#define EEFC_FCR_FCMD(fcmd) (fcmd & 0xFF)

#define EEFC_CMD_EPA_ARG_NP(np)	(np & 0x03) // Erase size in pages
#define EEFC_CMD_EPA_ARG_NP_4	EEFC_CMD_EPA_ARG_NP(0) // 4
#define EEFC_CMD_EPA_ARG_NP_8	EEFC_CMD_EPA_ARG_NP(1) // 8
#define EEFC_CMD_EPA_ARG_NP_16	EEFC_CMD_EPA_ARG_NP(2) // 16
#define EEFC_CMD_EPA_ARG_NP_32	EEFC_CMD_EPA_ARG_NP(3) // 32

#define EEFC_CMD_EPA_ARG_SP(sp)	((sp & 0xFFF) << 2) // Start page for page erase

#define EEFC_CMD_EPA_ARG(sp,np)	(EEFC_CMD_EPA_ARG_SP(sp) | EEFC_CMD_EPA_ARG_NP(np))

// EEFC_FSR

#define EEFC_FSR_FRDY		(1 << 0)
#define EEFC_FSR_FCMDE		(1 << 1)
#define EEFC_FSR_FLOCKE		(1 << 2)
#define EEFC_FSR_FLERR		(1 << 3)

// Write sequence:
//
// 1. Write the data to be programmed in the latch buffer.
// 2. Write the programming command in EEFC_FCR. This automatically clears the
//    EEFC_FSR.FRDY bit.
// 3. When Flash programming is completed, the EEFC_FSR.FRDY bit rises. If an
//    interrupt has been enabled by setting the EEFC_FMR.FRDY bit, the interrupt
//    line of the EEFC is activated.
//
// Three errors can be detected in EEFC_FSR after a programming sequence:
//
// - Command Error: A bad keyword has been written in EEFC_FCR.
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
//   of the EEFC_FCR. The first page number must be a multiple of 8, 16, or 32
//   depending on the number of pages to erase simultaneously.
// - Erase Sector (ES): A full memory sector is erased. Sector size depends on
//   the Flash memory. EEFC_FCR.FARG must be set with a page number that is in
//   the sector to be erased.
//
// Erase sequence:
//
// 1. Erase starts immediately one of the erase commands and the FARG field are
//    written in EEFC_FCR. For the EPA command, the two lowest bits of the FARG
//    field define the number of pages to be erased (FARG[1:0]), see table
//    below.
// 2. When erasing is completed, the EEFC_FSR.FRDY bit rises. If an interrupt
//    has been enabled by setting the EEFC_FMR.FRDY bit, the interrupt line of
//    the interrupt controller is activated.
//
// Three errors can be detected in EEFC_FSR after an erasing sequence:
// - Command Error: A bad keyword has been written in EEFC_FCR.
// - Lock Error: At least one page to be erased belongs to a locked region. The
//   erase command has been refused, no page has been erased. A command must be
//   run previously to unlock the corresponding region.
// - Flash Error: At the end of the erase period, the EraseVerify test of the
//   Flash memory has failed.

// -- Internal -------------------------------------------------------------- //

typedef enum {
	EEFC_CMD_GETD = 0x00,	// Get Flash descriptor
	EEFC_CMD_WP = 0x01,		// Write page
	EEFC_CMD_WPL = 0x02,	// Write page and lock
	EEFC_CMD_EWP = 0x03,	// Erase page and write page
	EEFC_CMD_EWPL = 0x04,	// Erase page and write page then lock
	EEFC_CMD_EA = 0x05,		// Erase all
	EEFC_CMD_EPA = 0x07,	// Erase pages
	EEFC_CMD_SLB = 0x08,	// Set lock bit
	EEFC_CMD_CLB = 0x09,	// Clear lock bit
	EEFC_CMD_GLB = 0x0A,	// Get lock bit
	EEFC_CMD_SGPB = 0x0B,	// Set GPNVM bit
	EEFC_CMD_CGPB = 0x0C,	// Clear GPNVM bit
	EEFC_CMD_GGPB = 0x0D,	// Get GPNVM bit
	EEFC_CMD_STUI = 0x0E,	// Start read unique identifier
	EEFC_CMD_SPUI = 0x0F,	// Stop read unique identifier
	EEFC_CMD_GCALB = 0x10,	// Get CALIB bit
	EEFC_CMD_ES = 0x11,		// Erase sector
	EEFC_CMD_WUS = 0x12,	// Write user signature
	EEFC_CMD_EUS = 0x13,	// Erase user signature
	EEFC_CMD_STUS = 0x14,	// Start read user signature
	EEFC_CMD_SPUS = 0x15	// Stop read user signature
} eefc_cmd_enum_t;


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
// } eefc_cmd_t;

/*
 * @brief      Construct, validate, and execute a flash command
 *
 * @param[in]  cmd   The command
 *
 * @return     { description_of_the_return_value }
 */
// int eefc_command( eefc_cmd_enum_t cmd, uint16_t arg )
// {
// 	// Initialize register value with access key and command
// 	uint32_t fcr = EEFC_FCR_FKEY(EEFC_FKEY_PASSWD) | EEFC_FCR_FCMD(cmd);

// 	// Command and argument validation
// 	switch ( cmd ) {
// 		case EEFC_CMD_EA:
// 		case EEFC_CMD_GETD:
// 		case EEFC_CMD_GLB:
// 		case EEFC_CMD_GGPB:
// 		case EEFC_CMD_STUI:
// 		case EEFC_CMD_SPUI:
// 		case EEFC_CMD_GCALB:
// 		case EEFC_CMD_WUS:
// 		case EEFC_CMD_EUS:
// 		case EEFC_CMD_STUS:
// 		case EEFC_CMD_SPUS:
// 			// No argument needed; farg must be 0
// 			break;
// 		case EEFC_CMD_ES:
// 			// FARG must be written with any page number within the sector to be
// 			// erased

// 			break;
// 		case EEFC_CMD_EPA:
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
// 	EEFC_FCR = fcr; 

// 	// ...

// 	return 0;
// }

// -- API ------------------------------------------------------------------- //

// V71 Flash
// 
// Memory size   = 2048 KB
// Page size     = 512 bytes
// Page count    = 4096 (12 bits)
// Sector count  = 16 (4 bits)
// 
// Sector 0         - 128 KB ; 256 pages
//   Small Sector 0 - 8 KB   ; 16 pages
//   Small Sector 1 - 8 KB   ; 16 pages
//   Large Sector   - 112 KB ; 224 pages
// Sector 1..n      - 128 KB ; 256 pages

int v71_flash_erase_app()
{
	// TODO: Check that the page isn't already erased

	// The app is located at 0x00404000 (0x4000) and is 0x1000 bytes (4 KB)
	// App start sector : 0
	// App start page   : 32
	//
	// So that's a EPA (erase pages) command with (32,2) as the argument
	// (start page = 32, erase 16 pages (smallest size available for 128 KB
	// sectors))

	// Issue the erase sector command
	// eefc_cmd_epa( 32, EEFC_CMD_EPA_FARG_NP_16 );
	// eefc_command( EEFC_CMD_ES,  );
	uint32_t fcr = EEFC_FCR_FKEY(EEFC_FCR_FKEY_PASSWD) | EEFC_FCR_FCMD(EEFC_CMD_EPA);
	// Ahhhhh. I'll probably want a function or macro that does the proper division based on *_NP_* argument, e.g. NP_16 divides page number by 16, the actual argument is in [15:3]
	fcr |= EEFC_FCR_FARG( EEFC_CMD_EPA_ARG((2 << 2), EEFC_CMD_EPA_ARG_NP_16 ) );

	// This should be checking the FRDY bit *before* executing a command
	// The command execution flow chart should probably be implemented in the eefc_command() (or per-command) function

	// Write command to command register
	EEFC_FCR = fcr;

	// Wait for erase to complete (EEFC_FSR.FRDY)
	uint32_t fsr;
	while ( ! ((fsr = EEFC_FSR) & EEFC_FSR_FRDY) );

	// fsr &= (EEFC_FSR_FCMDE | EEFC_FSR_FLOCKE | EEFC_FSR_FLERR);
	if ( fsr & (EEFC_FSR_FCMDE | EEFC_FSR_FLOCKE | EEFC_FSR_FLERR) ) {
		return (-fsr); // Some error occurred during the operation
	}

	return 0;
}

// int write_page_buffer( frame_t * frame, uint8_t * page_buffer );
// Must be a full page
int v71_flash_write_page( uint8_t * page_buffer, uint16_t page )
{
	// There are probably alignment requirements for this sort of thing; I know
	// from the datasheet that using DMA requires 32-bit alignment
	//
	// If a single byte has to be written in a 32-bit word, the rest of the word
	// must be written with ones.

	// The data has been written from a binary file over a byte-oriented
	// interface into a byte-based buffer; the original endianness of the data
	// has been maintained. Thus, it should be valid to just copy 4 bytes of
	// data at a time, assembling words in little-endian format (for ARM)...?

	// Copy data from application page buffer into EEFC page; must be written in 4-byte chunks
	volatile uint32_t * app_start_addr = (volatile uint32_t *)0x00404000 + (page * PAGE_SIZE);
	uint16_t i;
	for ( i = 0; i < PAGE_SIZE; i += 4 ) {
		// Write word
		*app_start_addr = page_buffer[i] | (page_buffer[i+1] << 8)
			 | (page_buffer[i+2] << 16) | (page_buffer[i+3] << 24);

		// Increment address
		app_start_addr++;
	}

	// Commit page
	uint32_t fcr = EEFC_FCR_FKEY(EEFC_FCR_FKEY_PASSWD) | EEFC_FCR_FCMD(EEFC_CMD_WP);
	fcr |= EEFC_FCR_FARG( (32 + page) ); // 

	// Write command to command register
	EEFC_FCR = fcr;

	uint32_t fsr;
	while ( ! ((fsr = EEFC_FSR) & EEFC_FSR_FRDY) );

	if ( fsr & (EEFC_FSR_FCMDE | EEFC_FSR_FLOCKE | EEFC_FSR_FLERR) ) {
		return (-fsr); // Some error occurred during the operation
	}

	return 0;
}
