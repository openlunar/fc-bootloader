
#include "rh71_flash.h"
#include "rh71_pmc.h"

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

int rh71_flash_init()
{
	// The write/erase clock is provided by the PMC. It must be configured at 2 MHz and enabled before using the erase / write feature on Flash memory.
	PMC_PCR = ((1 << 28 /* EN */) | (1 << 12 /* CMD - Write Mode */) | (50 /* PID8 */))
		| (1 << 29 /* GCLKEN */) | (1 << 8 /* GCLKCSS : MAIN_CLK */) | (1 << 20 /* GCLKDIV */);

	// PMC_PCR_EN_Msk | PMC_PCR_CMD_Msk | PMC_PCR_PID(50)  /* HEFC */
 //        | PMC_PCR_GCLKEN_Msk | PMC_PCR_GCLKCSS_MAIN_CLK | PMC_PCR_GCLKDIV(5);

	return 0;
}

int rh71_flash_erase_app()
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
	// hefc_cmd_epa( 32, HEFC_CMD_EPA_FARG_NP_16 );
	// hefc_command( HEFC_CMD_ES,  );
	uint32_t fcr = HEFC_FCR_FKEY(HEFC_FCR_FKEY_PASSWD) | HEFC_FCR_FCMD(HEFC_CMD_EPA);
	// Ahhhhh. I'll probably want a function or macro that does the proper division based on *_NP_* argument, e.g. NP_16 divides page number by 16, the actual argument is in [15:3]
	// fcr |= HEFC_FCR_FARG( HEFC_CMD_EPA_ARG((2 << 2), HEFC_CMD_EPA_ARG_NP_16 ) );
	fcr |= HEFC_FCR_FARG( HEFC_CMD_EPA_ARG((0 << 2), HEFC_CMD_EPA_ARG_NP_16 ) );

	// This should be checking the FRDY bit *before* executing a command
	// The command execution flow chart should probably be implemented in the hefc_command() (or per-command) function

	// Write command to command register
	HEFC_FCR = fcr;

	// Wait for erase to complete (HEFC_FSR.FRDY)
	uint32_t fsr;
	while ( ! ((fsr = HEFC_FSR) & HEFC_FSR_FRDY) );

	// fsr &= (HEFC_FSR_FCMDE | HEFC_FSR_FLOCKE | HEFC_FSR_FLERR);
	if ( fsr & (HEFC_FSR_FCMDE | HEFC_FSR_FLOCKE | HEFC_FSR_FLERR) ) {
		return (-fsr); // Some error occurred during the operation
	}

	return 0;
}

// int write_page_buffer( frame_t * frame, uint8_t * page_buffer );
// Must be a full page
int rh71_flash_write_page( uint8_t * page_buffer, uint16_t page )
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

	// Copy data from application page buffer into HEFC page; must be written in 4-byte chunks
	// volatile uint32_t * app_start_addr = (volatile uint32_t *)0x10004000 + (page * PAGE_SIZE);
	volatile uint32_t * app_start_addr = (volatile uint32_t *)0x10000000 + (page * PAGE_SIZE);
	uint16_t i;
	for ( i = 0; i < PAGE_SIZE; i += 4 ) {
		// Write word
		*app_start_addr = page_buffer[i] | (page_buffer[i+1] << 8)
			 | (page_buffer[i+2] << 16) | (page_buffer[i+3] << 24);

		// Increment address
		app_start_addr++;
	}

	// Synchronize pipeline
	__ISB();
	__DSB();

	uint32_t fsr;
	while ( ! ((fsr = HEFC_FSR) & HEFC_FSR_FRDY) );

	// Commit page
	uint32_t fcr = HEFC_FCR_FKEY(HEFC_FCR_FKEY_PASSWD) | HEFC_FCR_FCMD(HEFC_CMD_WP);
	// fcr |= HEFC_FCR_FARG( (32 + page) ); // 
	fcr |= HEFC_FCR_FARG( page ); // 

	// Write command to command register
	HEFC_FCR = fcr;

	while ( ! ((fsr = HEFC_FSR) & HEFC_FSR_FRDY) );

	if ( fsr & (HEFC_FSR_FCMDE | HEFC_FSR_FLOCKE | HEFC_FSR_FLERR) ) {
		return (-fsr); // Some error occurred during the operation
	}

	return 0;
}