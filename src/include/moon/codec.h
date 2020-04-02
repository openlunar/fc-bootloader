


#ifdef __cplusplus
	extern "C" {
#endif

#ifndef CMD_CODEC_H
#define CMD_CODEC_H

#include "moon/common.h"

#include <stdint.h>

// TODO: (10) [feature] @required Implement remaining codec functions (e.g. float, double)
// TODO: (10) [feature] @nth Implement alignment support via CONFIG_MOON_ALWAYS_ALIGNED or something; right now it assumes aligned (or, really, it doesn't care because the machine handles it appropriately)

// For now only building out support for:
// - Single-Normal and Single-Control type messages

// Header is stored little-endian, bits are addressed big-endian (e.g. VER[1:0] places VER[1] at 0 and VER[0] at 1)

// Some helpful macros for building bitfields

// Start, width -> mask

// #define BITFIELD(start,width)

// Pre-shift
// #define _BF_MASK1() ((2 << width) - 1)
// #define _BF_MASK2() (_BF_MASK1() << start)

// #define BF_MASK(val) (_BF_MASK1() & val)

// #define BF_VALUE()	

#define MOON_CODEC_VERSION 0

typedef enum {
	MOON_PROT_OK = 0,
	MOON_PROT_E_NO_SERVICE = 1,
	MOON_PROT_E_NO_METHOD = 2,
	MOON_PROT_E_BAD_SYNTAX = 3,
	MOON_PROT_E_UNAVAIL = 4
} moon_codec_prot_t;

moon_ret_t moon_codec_read_header( uint8_t * buffer, moon_msg_hdr_t * header );

void moon_codec_write_header( uint8_t * buffer, moon_msg_hdr_t * header );

void moon_codec_write_u8( uint8_t * buffer, uint8_t var, uint32_t offset );
void moon_codec_write_u16( uint8_t * buffer, uint16_t var, uint32_t offset );
void moon_codec_write_u32( uint8_t * buffer, uint32_t var, uint32_t offset );

void moon_codec_write_i8( uint8_t * buffer, int8_t var, uint32_t offset );
void moon_codec_write_i16( uint8_t * buffer, int16_t var, uint32_t offset );
void moon_codec_write_i32( uint8_t * buffer, int32_t var, uint32_t offset );

// NOTE: For now I'm going to have the user pass in a pointer where the resulting value can be stored; I could also return the (basic) types (e.g. u8, i16, etc.). I'm currently doing void return types on the assumption that the user has validated the buffer and offset combo (i.e. checked for buffer overrun); however, future iterations may return a value indicating failure (or use the internal failure member variable with a getter).
void moon_codec_read_u8( uint8_t * buffer, uint8_t * var, uint32_t offset );
void moon_codec_read_u16( uint8_t * buffer, uint16_t * var, uint32_t offset );
void moon_codec_read_u32( uint8_t * buffer, uint32_t * var, uint32_t offset );

void moon_codec_read_i8( uint8_t * buffer, int8_t * var, uint32_t offset );
void moon_codec_read_i16( uint8_t * buffer, int16_t * var, uint32_t offset );
void moon_codec_read_i32( uint8_t * buffer, int32_t * var, uint32_t offset );

#endif // CMD_CODEC_H

#ifdef __cplusplus
}
#endif
