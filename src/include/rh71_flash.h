
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef RH71_FLASH_H
#define RH71_FLASH_H

#include "common.h"
#include <stdint.h>

// TODO: This is a configuration value (i.e. defconfig) that changes between V71 and RH71; it should be in a centralized configuration header for the project
#define PAGE_SIZE 256

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

int rh71_flash_init();
int rh71_flash_erase_app();

int rh71_flash_write_page( uint8_t * page_buffer, uint16_t page );

#endif // RH71_FLASH_H

#ifdef __cplusplus
}
#endif
