/**
 * @brief      Command protocol
 *
 * @author     Logan Smith <logan@openlunar.org
 */

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

#define CMD_BOOTLOADER_SYS_ID	1

typedef enum {
	// -- Production Commands ----------------------------------------------- //
	CMD_BL_VERB_PING = 0,
	CMD_BL_VERB_BOOT = 1,
	CMD_BL_VERB_WRITE_TO_PAGE_BUFFER = 2,
	CMD_BL_VERB_ERASE_APP = 4,
	CMD_BL_VERB_COMMIT_PAGE_BUFFER = 5,
	// -- Debug Commands ---------------------------------------------------- //
	// Debug commands have 3 MSB set
	CMD_BL_VERB_DBG_WRITE_TO_RAM = 0xE1,
	CMD_BL_VERB_DBG_COMMIT_PAGE = 0xE2,
	CMD_BL_VERB_DBG_ERASE_RANGE = 0xE3,
	CMD_BL_VERB_DBG_JUMP_TO_ADDR = 0xE4
} cmd_bl_verb_t;

typedef enum {
	CMD_RESP_ACK = 0x00,
	CMD_RESP_NACK = 0xFF
} cmd_resp_t;

// NOTE:
// - This is going to be an all-encompassing function for now
// - In the future it maybe won't be (?)
// - The flow is:
//   - Link layer parses characters until a frame is received successfully
//   - The command layer parses the data from the link layer and validates:
//     - Command is for this device
//     - Command verb is supported
//     - Command structure fits verb

int cmd_exe( uint8_t * data, uint8_t len );

// This function is just intended to serve as an ACK / NACK; a separate function "cmd_resp_long" will probably be written for sending an extended response payload (maybe)
int cmd_resp( cmd_bl_verb_t verb, cmd_resp_t resp );

#endif // COMMAND_H
