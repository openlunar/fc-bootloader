
#include "moon/server.h"
#include "moon/transport.h"
#include "moon/codec.h"

static moon_msg_t message_g;

// Internal function used to respond to messages based on moon_services_handler() return value (e.g. responding appropriately to MOON_RET_E_* versus MOON_RET_OK)
moon_ret_t _server_response( moon_ret_t ret );

moon_ret_t moon_server_init()
{
	moon_ret_t ret;

	ret = moon_transport_init();

	if ( ret < 0 ) {
		return MOON_RET_E_TRANSPORT;
	}

	// Get reference to buffer from the transport layer (for use with codec)
	message_g.buffer = moon_transport_get_msg_buffer();
	message_g.read_len = 0;

	// assert( buffer not null, "" );

	return 0;
}

// NOTE: Having a shared buffer, i.e. no-copy, prevents sending a response before the RPC call has been made; basically, the CTRL type message for ACK/NAK is disallowed by this implementation.

// Non-blocking server call
// 
// TODO: I'm not a fan of mixing integer values and the enums right now...
// NOTE: Errors are only generated for problems that the layer above the server cares about - e.g. transport failure - not errors like "service not found" (that's the problem of the client)
moon_ret_t moon_server_poll()
{
	// int32_t ret;
	moon_ret_t ret;

	ret = moon_transport_read();

	// Return if a message isn't ready or if there's an error from the transport layer
	if ( (ret == MOON_RET_MSG_NOT_READY) || ( ret != MOON_RET_MSG_READY )  ) {
		return ret;
	}

	// Set the read length (used by shim functions to validate message syntax)
	message_g.read_len = moon_transport_get_read_length();

	// TODO: Check that read_len gte 3 (message_header size)
	// TODO: Validate message type (only support: single normal)
	
	// Read the header
	ret = moon_codec_read_header( message_g.buffer, &(message_g.header) );
	// TODO: Check result of this function (only failure possible is header version not matching MOON_CODEC_VERSION); what is the action if the header version doesn't match? We can't rely on *any* of the data in the header in that case (well, we could add the ability to support old versions, but probably won't)

	// Message ready, pass to service handler
	ret = moon_services_handler( &message_g );

	return _server_response( ret );
}

// Internal function that handles sending the message response
moon_ret_t _server_response( moon_ret_t ret )
{
// #ifdef CONFIG_MOON_DISABLE_ERR_RESP
// 	if ( ret != MOON_RET_OK ) {
// 		return MOON_RET_OK;
// 		// return MOON_RET_E_FAIL; // Or this, not sure...
// 	}
// #else
// 	// Allow responding to erroneous messages
// #endif // CONFIG_MOON_DISABLE_ERR_RESP

	// NOTE: Default value of protocol; over-written if the message has failed (see below). I'm working to isolate the protocol field from the shim functions - the protocol field should indicate server-layer stuff, not message functionality (this is a change from encoding a boolean response in the protocol field).
	// message_g.header.protocol = MOON_PROT_OK; // OK

	// Shims will *only* return MOON_RET_OK or a value mapping to the "single normal" response protocol field (e.g. MOON_RET_E_NO_SERVICE)
	if ( ret != MOON_RET_OK ) {
		// Configure response 
		message_g.header.type = MSG_TYPE_SINGLE_NORMAL;
		// NOTE: Protocol was set by the error producer

		// TODO: Replace with constant for "message header leng" as a function of the message type (e.g. single normal)
		message_g.write_len = 3;
		moon_codec_write_header( message_g.buffer, &(message_g.header) );
	}

	return moon_transport_write( message_g.write_len );
}
