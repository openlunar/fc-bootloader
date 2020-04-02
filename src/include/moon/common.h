
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef MOON_COMMON_H
#define MOON_COMMON_H

#include "moon/moon_config.h"

#include <stdint.h>

typedef enum {
	MOON_RET_OK = 0,
	MOON_RET_MSG_NOT_READY = 0, // Equivalent in response OK but more clear in certain contexts
	MOON_RET_MSG_READY = 1,

// -- Errors -- //	
	MOON_RET_E_NO_SERVICE = -1,
	MOON_RET_E_NO_METHOD = -2,
	MOON_RET_E_SYNTAX = -3,
	// MOON_RET_E_UNAVAIL = -4, // Not currently supported

	// Internal server failure errors
	MOON_RET_E_TRANSPORT = -10, // Transport layer failed
	MOON_RET_E_CODEC = -11
} moon_ret_t;

typedef enum {
	MSG_TYPE_SINGLE_NORMAL = 0,
	MSG_TYPE_SINGLE_CONTROL = 1
	// MSG_TYPE_MULTI_FIXED = 2,
	// MSG_TYPE_MULTI_STREAM = 3
} msg_type_t;

// TODO: (60) [feature] @poorly_defined Define enum for protocol
// TODO: Make protocol member of struct a union of the enums available for protocols
// TODO: The current structure won't work for MULTI messages (protocol is 12 bits in that case)

typedef struct {
	msg_type_t type;
	uint8_t service;
	uint8_t method;
	uint8_t sequence;
	uint8_t protocol;
} moon_msg_hdr_t;

typedef struct {
	moon_msg_hdr_t header;
	uint8_t * buffer;
	// NOTE: These are shared because this is a zero-copy implementation. This is mostly used by the generated shim functions. The only place it's used otherwise is handling NO_SERVICE and NO_METHOD.
	union {
		uint32_t read_len;
		uint32_t write_len;
	};
} moon_msg_t;

#endif // MOON_COMMON_H

