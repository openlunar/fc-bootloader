

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef MOON_TRANSPORT_H
#define MOON_TRANSPORT_H

#include "moon/server.h"

#include <stdint.h>

// int32_t moon_transport_init( uint8_t * buffer, uint32_t buffer_size );

// For this single-instance implementation the transport buffer is managed by the transport layer internally
moon_ret_t moon_transport_init();

// Return pointer to message buffer managed by transport layer
uint8_t * moon_transport_get_msg_buffer();

// Return length of message associated with moon_transport_read() returning MOON_RET_MSG_READY
uint32_t moon_transport_get_read_length();

/**
 * @brief      Non-blocking read from the transport layer implementation
 *
 * @return     Integer representing the result of the read
 */
moon_ret_t moon_transport_read();

moon_ret_t moon_transport_write( uint32_t len );

#endif // MOON_TRANSPORT_H
