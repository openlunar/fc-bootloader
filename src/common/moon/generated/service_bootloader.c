// GENERATED FILE (by Logan)

// TODO: Header name
#include "service_bootloader.h"
#include "moon/codec.h"

int service_bootloader_handler( moon_msg_t * message ) {

	switch ( message->header.method ) {
		case kBootloader_bl_ping_id:
			return bl_ping_shim( message );
		case kBootloader_bl_writePageBuffer_id:
			return bl_writePageBuffer_shim( message );
		case kBootloader_bl_erasePageBuffer_id:
			return bl_erasePageBuffer_shim( message );
		case kBootloader_bl_eraseApp_id:
			return bl_eraseApp_shim( message );
		case kBootloader_bl_writePage_id:
			return bl_writePage_shim( message );
		case kBootloader_bl_setBootAction_id:
			return bl_setBootAction_shim( message );
		case kBootloader_bl_boot_id:
			return bl_boot_shim( message );
		default:
			message->header.protocol = MOON_PROT_E_NO_METHOD;
			return MOON_RET_E_NO_METHOD;
	}
}

// TODO: (2) [refactor] Make it easier (and more efficient) to encode the common case of the header - "single normal", "OK", matching service and method ID that resulted in the shim call.

// TODO: One question to ask yourself while working through manually writing
// these shims is: What will be different about them, if anything, in the RTOS
// implementation? The shims use the moon_codec* set of functions and the
// moon_msg_t. The message type exposes a buffer and the header. Right now we're
// manually passing the buffer to the codec. The codec functions return void -
// they assume you've made sure the index is within bounds of the provided
// buffer.
//
// A different implementation might have a buffer with bounds checking. Or a
// write that keeps track of how many bytes have been written (so write_len
// isn't set "manually" (it's set by the generated code manually, not the
// user)).
//
// Just think about how these APIs might change, especially if rewritten in C++.
// Also, there might be a case where the codec is used by more than just the
// server shims.

int bl_ping_shim( moon_msg_t * message )
{
	// Call actual served function
	bl_ping();

	// Build response
	message->header.type = MSG_TYPE_SINGLE_NORMAL;
	message->header.protocol = MOON_PROT_OK;
	
	moon_codec_write_header( message->buffer, &(message->header) );
	message->write_len = 3;

	return MOON_RET_OK;
}

int bl_writePageBuffer_shim( moon_msg_t * message )
{
	// Collect arguments
	// 
	// XXX: There's a padding byte between the header and the start of arguments (force 32-bit alignment)
	// XXX offset = [4,2]
	// XXX list = [6,1,7,list_len]
	// 
	// The message has been manually packed such that the list length comes before offset followed by the array (optimal alignment)
	// offset = [4,2]
	// data_len = [3,1]
	// data = [6,data_len]

	uint16_t offset;
	moon_codec_read_u16( message->buffer, &offset, 4 );

	uint8_t data_len;
	uint8_t * data;

	data = &message->buffer[6]; // The list is of u8 elements and is thus naturally aligned; use in place
	// NOTE: Hmmm......it'd be neat if I could read a u8 into a u32; probably want to store list length in the minimum-sized type but then expand to 32-bits on the server end...
	moon_codec_read_u8( message->buffer, &data_len, 3 );

	// if ( data_len > BL_WPB_MAX_LEN ) {
	// 	return MOON_RET_E_SYNTAX;
	// }

	// Call actual served function
	int8_t resp;
	resp = bl_writePageBuffer( offset, data_len, data );

	// Build response
	message->header.type = MSG_TYPE_SINGLE_NORMAL;
	message->header.protocol = MOON_PROT_OK;
	
	// TODO: Why might this fail (?)
	// ret = moon_codec_write_header( buffer, header );
	moon_codec_write_header( message->buffer, &(message->header) );
	moon_codec_write_i8( message->buffer, resp, 3 );
	message->write_len = 4;

	return MOON_RET_OK;
}

// -- New methods ----------------------------------------------------------- //
// I'm implementing these methods kind of quickly so I can get a bootloader demo going. The real work is still going on in the server implementation.

int bl_erasePageBuffer_shim( moon_msg_t * message )
{
	// Call actual served function
	bl_erasePageBuffer();

	// Build response
	message->header.type = MSG_TYPE_SINGLE_NORMAL;
	message->header.protocol = MOON_PROT_OK;

	moon_codec_write_header( message->buffer, &(message->header) );
	message->write_len = 3;

	return MOON_RET_OK;
}

int bl_eraseApp_shim( moon_msg_t * message )
{
	// Arguments
	// This presents an interesting challenge - if I want to store a variable in one type on the line and then expand it to a different type on the other size (e.g. u8 to int(enum)) then I either need to create a function that can take an additional argument of "line size" and then "target variable" or do what I'm going to do below which is allocate a second variable of the target type and then cast it.
	uint8_t _app_id;
	moon_codec_read_u8( message->buffer, &_app_id, 3 );
	// TODO: "AppId" (and really, all bootloader service members) needs the namespace of "bl" and I would like them to end with "_t" to indicate it's a type (at least in C)
	AppId app_id = (AppId)(_app_id); // Cast to enum type

	// Call actual served function
	int8_t resp;
	resp = bl_eraseApp( app_id );

	// Build response
	message->header.type = MSG_TYPE_SINGLE_NORMAL;
	message->header.protocol = MOON_PROT_OK;

	moon_codec_write_header( message->buffer, &(message->header) );
	moon_codec_write_i8( message->buffer, resp, 3 );
	message->write_len = 4;

	return MOON_RET_OK;
}

int bl_writePage_shim( moon_msg_t * message )
{
	// Arguments
	uint8_t _app_id;
	AppId app_id;
	uint16_t page_no;
	uint32_t crc;

	// XXX: NOTE: u8 padding manually added at index 3 for alignment purposes; arguments start at index 4
	// NOTE: Arguments ordered for alignment
	moon_codec_read_u8( message->buffer, &_app_id, 3 );
	app_id = (AppId)(_app_id); // Cast to enum type
	moon_codec_read_u32( message->buffer, &crc, 4 );
	moon_codec_read_u16( message->buffer, &page_no, 8 );

	// Call actual served function
	int8_t resp;
	resp = bl_writePage( app_id, page_no, crc );

	// Build response
	message->header.type = MSG_TYPE_SINGLE_NORMAL;
	message->header.protocol = MOON_PROT_OK;

	moon_codec_write_header( message->buffer, &(message->header) );
	moon_codec_write_i8( message->buffer, resp, 3 );
	message->write_len = 4;

	return MOON_RET_OK;
}

int bl_setBootAction_shim( moon_msg_t * message )
{
	// Arguments
	uint8_t _action;
	moon_codec_read_u8( message->buffer, &_action, 3 );
	BootAction action = (BootAction)(_action); // Cast to enum type

	// Call actual served function
	int8_t resp;
	resp = bl_setBootAction( action );

	// Build response
	message->header.type = MSG_TYPE_SINGLE_NORMAL;
	message->header.protocol = MOON_PROT_OK;

	moon_codec_write_header( message->buffer, &(message->header) );
	moon_codec_write_i8( message->buffer, resp, 3 );
	message->write_len = 4;

	return MOON_RET_OK;
}

int bl_boot_shim( moon_msg_t * message )
{
	// Call actual served function
	int8_t resp;
	resp = bl_boot();

	// Build response
	message->header.type = MSG_TYPE_SINGLE_NORMAL;
	message->header.protocol = MOON_PROT_OK;
	
	moon_codec_write_header( message->buffer, &(message->header) );
	moon_codec_write_i8( message->buffer, resp, 3 );
	message->write_len = 4;

	return MOON_RET_OK;
}
