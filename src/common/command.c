
#include "config.h"

#include "command.h"
#include "sll.h"

#include "usart.h"

// Global frame instance used to send responses
static uint8_t sll_buffer_g[SLL_MAX_MSG_LEN];

// TODO:
// - This internally should be split into a "command parsing" and "command execution" phase

int _write_ram( uint8_t * msg );

int _write_page_buffer( uint8_t * msg, uint8_t * page_buffer );

int cmd_exe( uint8_t * data, uint8_t len )
{
	if ( ! data ) {
		return (-1);
	}

	// TODO: Maybe an error (?)
	if ( ! len ) {
		return 0;
	}

	// Validate system ID this command is for matches this system
	uint8_t id = data[0];
	if ( id != CMD_BOOTLOADER_SYS_ID ) {
		// TODO: Need a response message type for this
		return (-1); // TODO: Return value should indicate error cause (?) e.g. CMD_ERR_SYSID
	}

	// Retrieve verb from data payload
	cmd_bl_verb_t verb = (cmd_bl_verb_t)data[1]; // NOTE: This is an 'uint8_t' to 'int' cast

	// Process verb
	int ret = 0;
	switch ( verb ) {
	// -- Production Commands ----------------------------------------------- //
		case CMD_BL_VERB_PING:
			cmd_resp( CMD_BL_VERB_PING, CMD_RESP_ACK );
			break;
		case CMD_BL_VERB_WRITE_TO_PAGE_BUFFER:
			break;
	// -- Debug Commands ---------------------------------------------------- //
		case CMD_BL_VERB_DBG_WRITE_TO_RAM:
			ret = _write_ram( data );
			break;
		case CMD_BL_VERB_DBG_COMMIT_PAGE:
			// rh71_flash_write_page( page_buffer, 0 );
			// break;
		default:
			ret = (-1);
	}

	if ( ret ) {
		cmd_resp( verb, CMD_RESP_NACK );
	} else {
		cmd_resp( verb, CMD_RESP_ACK );
	}

	return ret;
}

int _write_ram( uint8_t * msg )
{
	// arg0: byte offset (u16), big endian
	// arg1: length (u8)
	// arg2: data (u8,arg1:length)
	
	// TODO: Pulling out the arguments shouldn't be done in this function from the frame, it should be done...somewhere else.
	// TODO: Need to validate that the data length from the frame matches arguments + data_len argument
	uint32_t offset = (msg[2] << 8) | msg[3]; // 16-bit field loaded into 32-bit variable
	uint8_t length = msg[4];
	uint8_t * payload = &(msg[5]);

	// Actually, it probably makes sense to only allow word-aligned access and have the offset field be in words not bytes; length must then be a word multiple of bytes
	// Another option is to allow unaligned RAM access by reading the data in the field and or'ing it with the received data, allowing unaligned offsets and non-word lengths. That's more complicated so we're not going to do it yet. This is a debug feature, anyway
	
	// It's an error if length is not word aligned
	if ( (length % 4) != 0 ) {
		return (-1);
	}

	// TODO: Should be checking bounds of write, for now we ignore it...

	uint32_t i;
	volatile uint32_t * ram = (volatile uint32_t *)(CONFIG_RAM_LOAD_ADDR + (offset << 2));
	for ( i = 0; i < length; i += 4 ) {
		// Load word into RAM
		*ram = payload[i] | (payload[i+1] << 8)
			 | (payload[i+2] << 16) | (payload[i+3] << 24);

		// Increment RAM address pointer
		ram++;
	}

	return 0;
}

int _write_page_buffer( uint8_t * msg, uint8_t * page_buffer )
{
	// arg0: byte offset (u16)
	// arg1: length (u8)
	// arg2: data (u8,arg1:length)
	
	// TODO: Pulling out the arguments shouldn't be done in this function from the frame, it should be done...somewhere else.
	// TODO: Need to validate that the data length from the frame matches arguments + data_len argument
	uint16_t offset = (msg[2] << 8) | msg[3];
	uint8_t length = msg[4];
	uint8_t * data = &(msg[5]);

	// Check offset / length arguments
	if ( (offset + length - 1) >= CONFIG_PAGE_SIZE ) {
		return (-1);
	}

	// TODO: Move "64" into constant for maximum write_page_buffer data size
	// NOTE: The EDBG / ttyACM / something in the USART setup between Linux and the EDBG USART output seems to have a 64-byte maximum packet size so I've been using 32 byte page packets
	// Limit packet size to 64 bytes
	// if ( length > 64 ) {
	// 	return (-1);
	// }

	uint16_t page_idx;
	for ( page_idx = offset; page_idx < (offset + length); page_idx++ ) {
		page_buffer[page_idx] = *data; // Write data to page
		data++; // Increment data index
	}

	return 0;
}

int cmd_resp( cmd_bl_verb_t verb, cmd_resp_t resp )
{
	uint8_t resp_payload[3] = { CMD_BOOTLOADER_SYS_ID, verb, (uint8_t)resp };

	// Encode response into sll frame
	int ret = sll_encode( sll_buffer_g, resp_payload, 3 );
	// TODO: Check return code

	// TODO: Argh, I was trying not to couple this to the USART link
	usart_write( 1, sll_buffer_g, ret );

	return 0;
}
