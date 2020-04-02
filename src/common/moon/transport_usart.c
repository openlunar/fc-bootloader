
#include "moon/server.h"
#include "moon/transport.h"

#include "sll.h"
#include "usart.h"

// Sanity check message size against maximum payload size
#if (MOON_MAX_MESSAGE_LEN > SLL_MAX_PAYLOD_LEN)
	#error "Maximum message size cannot be greater than SLL maximum payload size"
#endif

// TODO: This should maybe be in a header; It should at least be a CONFIG variable
#define TRANSPORT_USART_NO 1

// Internal frame and buffer declarations
static sll_decode_frame_t sll_frame_g;
static uint8_t sll_buffer_g[SLL_MAX_MSG_LEN];

moon_ret_t moon_transport_init()
{
	// Configure USART instance (for now it's hard-coded)
	// The USART configuration is hard-coded in the driver right now; in the future this section would configure baud rate, etc.

	int32_t ret;

	// Initialize SLL (link layer)
	//
	// NOTE: The size of the buffer is calculated using sizeof() here to prevent
	// the error of using a constant to declare its size and then passing a
	// different value in this argument (but you can't use the sizeof() trick in
	// the sll_init() call because you're passing a pointer to the buffer (the
	// sizeof() which would be 4 bytes))
	ret = sll_init( &sll_frame_g, sll_buffer_g, sizeof(sll_buffer_g) / sizeof(sll_buffer_g[0]) );

	if ( ret < 0 ) {
		return MOON_RET_E_TRANSPORT;
	}

	return MOON_RET_OK;
}

uint8_t * moon_transport_get_msg_buffer()
{
	return sll_get_data_buffer( &sll_frame_g );
}

uint32_t moon_transport_get_read_length()
{
	return sll_get_decoded_len( &sll_frame_g );
}

// Non-blocking transport 
moon_ret_t moon_transport_read()
{
	int ret;
	uint8_t c;

	// TODO: How to handle hardware level failures? (e.g. buffer overrun,
	// framing error). Any transport implementation will have potential hardware
	// failures. Additionally, there could be logic errors at the hardware or
	// link layer - e.g. CRC failure, authentication failure, desynchronization
	// of stream (decoder failure).
	//
	// There are probably some errors that should cause the system to restart if
	// they happen / happen enough. However, that probably shouldn't be handled
	// by the actual server implementation, that's more of a system-level
	// concern. For example, if the radio fails and has to be restarted then the
	// server should probably be reinitialized. However, that should be handled
	// by a higher level system management FSM. Probably some sort of simple
	// "system health monitor" that detects those failures and restarts services
	// as needed. So, I think we just pass up a generic E_TRANSPORT

	// Check if there's a character ready from USART
	ret = usart_read( TRANSPORT_USART_NO, &c, USART_FLAGS_NONBLOCK );

	// TODO: Distinguish between error types
	if ( ret < 0 ) {
		return MOON_RET_E_TRANSPORT;
	} else if ( ret == 0 ) {
		return MOON_RET_MSG_NOT_READY;
	}

	// Advance the SLL FSM; check if a frame is ready
	ret = sll_decode( &sll_frame_g, c );

	// TODO: Distinguish between error types
	if ( ret < 0 ) {
		return MOON_RET_E_TRANSPORT; // Error
	} else if ( ret == 0 ) {
		return MOON_RET_MSG_NOT_READY;
	}

	// TODO: It's possible that the link layer has a larger message size than
	// the command protocol layer; ideally the user sets the IDL max message
	// length to match the link layer maximum payload size; however, they could
	// set it smaller for some reason. If so, we need to check that the returned
	// size of the payload is lte to the max message length.

	// Frame is ready, indicate so
	return MOON_RET_MSG_READY;
}

moon_ret_t moon_transport_write( uint32_t len )
{
	// TODO: Is len == 0 an error (?)

	// TODO: I might want to implement an equivalent sll_get_encoded_len()
	// function to have a consistent API for SLL. Some of these weird API issues
	// are because I'm writing this in C...
	//
	// sll_encode returns the length of the encoded frame or ltz on failure
	int ret = sll_encode( &sll_frame_g, len );

	if ( ret < 0 ) {
		return MOON_RET_E_TRANSPORT;
	}

	ret = usart_write( TRANSPORT_USART_NO, sll_buffer_g, ret );

	if ( ret < 0 ) {
		return MOON_RET_E_TRANSPORT;
	}

	return MOON_RET_OK;
}
