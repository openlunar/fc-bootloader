
#include "moon/codec.h"

// TODO: (10) [refactor] Add in support for "always aligned" CONFIG

// TODO: (5) [analysis] Are these calls getting inlined (?)

// TODO: Provide getter/setter for all header fields (?); e.g. "codec_header_get_service" and "codec_header_set_service"

// TODO: How much validation should this function do? Maybe none? Like what if an invalid service_id is passed to it (one that exceeds 5 bits of data)?
void moon_codec_write_header( uint8_t * buffer, moon_msg_hdr_t * header )
{
	// Header Layout
	// 23    20 19            16 15   12 11 8 7      4 3          0
	// [      ] [              ] [     ] [  ] [      ] [          ]
	// 
	// 23    20 19    18     17   13 12       7 6         2 1     0
	// [ PTCL ] [ T ] [ SM ] [ SEQ ] [ REQ_ID ] [ SERV_ID ] [ VER ]
	//     4      1     1       5        6           5         2
	//
	// TODO: Replace hard-coded values with macros
	uint32_t hdr = (MOON_CODEC_VERSION & 0x3)
		| ((header->service & 0x1F) << 2)	// w: 5, s: 2
		| ((header->method & 0x3F) << 7)	// w: 6, s: 7
		| ((header->sequence & 0x1F) << 13)	// w: 5, s: 13
		| ((header->type & 0x3) << 18)		// w: 2, s: 18
		| ((header->protocol & 0xF) << 20);	// w: 4, s: 20

	// NOTE: Writing the header is the first step of building a response and
	// thus any data in the buffer right now can be overwritten, thus we write 4
	// bytes instead of 3 bytes (since this codec only supports SINGLE type
	// messages)
	moon_codec_write_u32( buffer, hdr, 0 );
}

moon_ret_t moon_codec_read_header( uint8_t * buffer, moon_msg_hdr_t * header )
{
	uint32_t hdr;
	moon_codec_read_u32( buffer, &hdr, 0 );

	// Check that the codec version matches
	if ( (hdr & 0x3) != MOON_CODEC_VERSION ) {
		return MOON_RET_E_CODEC; // TODO: Replace with E_CODEC_VERSION or E_CODEC or something
	}

	// TODO: Replace hard-coded values with macros
	header->service = ((hdr >> 2) & 0x1F);
	header->method = ((hdr >> 7) & 0x3F);
	header->sequence = ((hdr >> 13) & 0x1F);
	header->type = ((hdr >> 18) & 0x3);
	header->protocol = ((hdr >> 20) & 0xF);

	return MOON_RET_OK;
}

// -- WRITE ----------------------------------------------------------------- //

// -- UNSIGNED -- //

void moon_codec_write_u8( uint8_t * buffer, uint8_t var, uint32_t offset )
{
	*(uint8_t *)&buffer[offset] = var;
}

void moon_codec_write_u16( uint8_t * buffer, uint16_t var, uint32_t offset )
{
	*(uint16_t *)&buffer[offset] = var;
}

void moon_codec_write_u32( uint8_t * buffer, uint32_t var, uint32_t offset )
{
	*(uint32_t *)&buffer[offset] = var;
}

// -- SIGNED -- //

// TODO: These could probably just cast the result of the matching u* call but since it's already one line that seems pointless. Might be worth considering for the "always aligned" version.

void moon_codec_write_i8( uint8_t * buffer, int8_t var, uint32_t offset )
{
	*(int8_t *)&buffer[offset] = var;
}

void moon_codec_write_i16( uint8_t * buffer, int16_t var, uint32_t offset )
{
	*(int16_t *)&buffer[offset] = var;
}

void moon_codec_write_i32( uint8_t * buffer, int32_t var, uint32_t offset )
{
	*(int32_t *)&buffer[offset] = var;
}

// -- READ ------------------------------------------------------------------ //
// -- UNSIGNED -- //

void moon_codec_read_u8( uint8_t * buffer, uint8_t * var, uint32_t offset )
{
	*var = *(uint8_t *)&buffer[offset];
}

void moon_codec_read_u16( uint8_t * buffer, uint16_t * var, uint32_t offset )
{
	*var = *(uint16_t *)&buffer[offset];
}

void moon_codec_read_u32( uint8_t * buffer, uint32_t * var, uint32_t offset )
{
	*var = *(uint32_t *)&buffer[offset];
}

// -- SIGNED -- //

void moon_codec_read_i8( uint8_t * buffer, int8_t * var, uint32_t offset )
{
	*var = *(int8_t *)&buffer[offset];
}

void moon_codec_read_i16( uint8_t * buffer, int16_t * var, uint32_t offset )
{
	*var = *(int16_t *)&buffer[offset];
}

void moon_codec_read_i32( uint8_t * buffer, int32_t * var, uint32_t offset )
{
	*var = *(int32_t *)&buffer[offset];
}

// NOTE: When preparing a response message it's not necessary to write the service / request / sequence ids if they haven't been modified...one could make an argument about making sure the the proper values are in them by overwriting them...tbd

