// Stand-in Link Layer

#include "sll.h"
#include "crc.h"

int sll_init( sll_decode_frame_t * const frame )
{
	frame->length = 0;

	uint8_t i;
	// Initialize frame data buffer to 0 (NOTE: mostly for debugging purposes)
	for ( i = 0; i < SLL_MAX_PAYLOD_LEN; i++ ) {
		frame->data[i] = 0x00;
	}

	// Initialize frame internal state
	frame->_ctx.state = SLL_DECODE_SYNC1;
	frame->_ctx.idx = 0;
	frame->_ctx.crc = 0;

	return 0;
}

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
			if ( c > SLL_MAX_PAYLOD_LEN ) {
				frame->_ctx.state = SLL_DECODE_SYNC1;
				// Hmm. Should this return an error instead of 0 (?)
				break;
			}

			frame->length = c;
			frame->_ctx.idx = 0; // Reset payload index

			// Start CRC calculation
			frame->_ctx.crc = crc_16ibm_update( 0x0000, c ); // Initial CRC is 0
			
			if ( frame->length > 0 ) {
				frame->_ctx.state = SLL_DECODE_DATA;
			} else {
				frame->_ctx.state = SLL_DECODE_CRC1;
			}
			break;
		case SLL_DECODE_DATA:
			frame->data[frame->_ctx.idx] = c;
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

int sll_encode( uint8_t * buffer, uint8_t * data, uint8_t len )
{
	if ( (! buffer) || (! data) ) {
		return (-1);
	}

	if ( len > SLL_MAX_PAYLOD_LEN ) {
		return (-1);
	}

	// Configure header
	buffer[0] = SLL_SYNC_SEQ_1;
	buffer[1] = SLL_SYNC_SEQ_2;
	buffer[2] = len;

	// Initialize CRC
	uint16_t crc = crc_16ibm_update( 0x0000, len );

	// Add data
	uint8_t i;
	for ( i = 0; i < len; i++ ) {
		buffer[3+i] = data[i];
		crc = crc_16ibm_update( crc, data[i] );
	}

	// Append CRC
	i += 3;
	buffer[i++] = (crc & 0xFF);
	buffer[i++] = ((crc >> 8) & 0xFF);

	return i;
}
