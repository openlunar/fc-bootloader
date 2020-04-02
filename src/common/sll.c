// Stand-in Link Layer

#include "sll.h"
#include "crc.h"

// TODO: (2) @poorly_defined If the buffer is declared external to the SLL module then it should be a compile-time error to provide a buffer smaller than SLL_MAX_PAYLOD_LEN.
// TODO: (2) @poorly_defined SLL_MAX_PAYLOD_LEN should maybe be a config variable (?), so it would become CONFIG_SLL_MAX_PAYLOD_LEN

// 2 sync bytes + 1 length byte
#define SLL_FRAME_DATA_START	3

int sll_init( sll_decode_frame_t * const frame, uint8_t * buffer, const uint8_t buffer_len )
{
	// TODO: (10) [robustness] Enable these asserts
	// assert( frame, "argument 'frame' cannot be null" );
	// assert( buffer, "argument 'buffer' cannot be null" );
	// assert( (buffer_len >= SLL_MAX_MSG_LEN), "argument 'buffer_len' must be greater than or equal to SLL_MAX_MSG_LEN");
	(void)buffer_len; // TODO: @poorly_defined I have this so I can assert that it is at least SLL_MAX_MSG_LEN (for testing). I don't intend to use this at runtime because the code is written such that the buffer *has* to be SLL_MAX_MSG_LEN (it doesn't store this value anywhere and change handling). I guess it could, but it's just one more spot to fail.

	// Initialize external facing state
	frame->length = 0;
// #if CONFIG_SLL_NOCOPY_CODEC
	frame->frame_buffer = buffer;
	frame->data_buffer = &(buffer[SLL_FRAME_DATA_START]);
// #else
// 	frame->data = buffer;
// #endif // CONFIG_SLL_NOCOPY_CODEC

	// uint8_t i;
	// // Initialize frame data buffer to 0 (NOTE: mostly for debugging purposes)
	// for ( i = 0; i < SLL_MAX_PAYLOD_LEN; i++ ) {
	// 	frame->data[i] = 0x00;
	// }

	// Initialize frame internal state
	// frame->_ctx.buffer = buffer;
	frame->_ctx.state = SLL_DECODE_SYNC1;
	frame->_ctx.idx = 0;
	frame->_ctx.crc = 0;

	return 0;
}

uint8_t * sll_get_data_buffer( sll_decode_frame_t * const frame )
{
	return frame->data_buffer;
}

uint32_t sll_get_decoded_len( sll_decode_frame_t * const frame )
{
	return frame->length;
}

// Return:
// <0 - Error
// 0 - Ok but no message decoded
// 1 - Message decoded successfully
// 
// Hmm.
// I've got a couple thoughts:
// - I could use frame_len of 0 to indicate a length of 1; this allows a length of 256 bytes maximum payload while still fitting into a uint8_t
// - I don't think there's a reason to have a 0-length frame. I can just make that an error on the encode size (both client and server).
// 
// TODO: (0) [api] Change length handling such that frame_len(0) = length(1)
int sll_decode( sll_decode_frame_t * frame, uint8_t c )
{
	switch ( frame->_ctx.state ) {
		// Waiting for SYNC sequence
		// Could get clever here and use a window to speed up re-sync (prevent missing AAB kinds of errors)
		case SLL_DECODE_SYNC1:
			if ( c == SLL_SYNC_SEQ_1 ) {
				frame->_ctx.state = SLL_DECODE_SYNC2;
			}
			break;
		case SLL_DECODE_SYNC2:
			if ( c == SLL_SYNC_SEQ_2 ) {
				frame->_ctx.state = SLL_DECODE_LEN;
			}
			break;
		case SLL_DECODE_LEN:
			// NOTE: This assumes that the data buffer is large enough to hold
			// SLL_MAX_PAYLOD_LEN bytes of data
			if ( c > SLL_MAX_PAYLOD_LEN ) {
				frame->_ctx.state = SLL_DECODE_SYNC1;
				// TODO: (3) [refactor] @error_handling @poorly_defined Hmm. Should this return an error instead of 0 (?)
				break;
			}

			frame->length = c;
			frame->_ctx.idx = 0; // Reset payload index

			// Start CRC calculation
			frame->_ctx.crc = crc_16ibm_update( CRC_16IBM_INIT_VALUE, c ); // Initial CRC is 0
			
			if ( frame->length > 0 ) {
				frame->_ctx.state = SLL_DECODE_DATA;
			} else {
				frame->_ctx.state = SLL_DECODE_CRC1;
			}
			break;
		case SLL_DECODE_DATA:
			frame->data_buffer[frame->_ctx.idx] = c;
			frame->_ctx.idx++;

			// Update CRC
			frame->_ctx.crc = crc_16ibm_update( frame->_ctx.crc, c );

			if ( frame->_ctx.idx >= frame->length ) {
				frame->_ctx.state = SLL_DECODE_CRC1;
			}
			break;
		case SLL_DECODE_CRC1:
			frame->_ctx.crc = crc_16ibm_update( frame->_ctx.crc, c ); // LSB comes in first
			frame->_ctx.state = SLL_DECODE_CRC2;
			break;
		case SLL_DECODE_CRC2:
			frame->_ctx.crc = crc_16ibm_update( frame->_ctx.crc, c ); // MSB second
			
			frame->_ctx.state = SLL_DECODE_SYNC1;

			if ( frame->_ctx.crc == 0 ) {
				return 1; // CRC match
			} else {
				return (-1); // CRC fail
			}

			break;
		default:
			frame->_ctx.state = SLL_DECODE_SYNC1;
	}

	return 0;
}

int sll_encode( sll_decode_frame_t * const frame, uint32_t data_len )
{
	// TODO: (10) [robustness] Enable these asserts
	// assert( frame not null, "" );
	// assert( len >= SLL_MAX_PAYLOD_LEN, "" );

	// if ( (! out_buffer) || (! data) ) {
	// 	return (-1);
	// }

	if ( data_len >= SLL_MAX_PAYLOD_LEN ) {
		return (-1);
	}

	// Configure header
	// NOTE: The sync bytes are always rewritten just in case they were overwritten
	// assert( sync bytes haven't been written, "" );
	frame->frame_buffer[0] = SLL_SYNC_SEQ_1;
	frame->frame_buffer[1] = SLL_SYNC_SEQ_2;
	frame->frame_buffer[2] = (uint8_t)data_len;

	// Initialize CRC, calculate CRC of data_len
	uint16_t crc = crc_16ibm_update( CRC_16IBM_INIT_VALUE, data_len );

	// Calculate CRC of the message data
	uint32_t i;
	for ( i = 0; i < data_len; i++ ) {
		// frame->frame_buffer[3+i] = data[i]; // NO COPY
		crc = crc_16ibm_update( crc, frame->data_buffer[i] );
	}

	// Append CRC
	i += SLL_FRAME_DATA_START; // Calculate CRC LSB index (stored little endian) by offsetting 'i' by the start of the data section in the frame
	frame->frame_buffer[i++] = (crc & 0xFF);
	frame->frame_buffer[i++] = ((crc >> 8) & 0xFF);

	return i;
}
