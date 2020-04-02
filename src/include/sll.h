/**
 * @brief      Stand-in Link Layer
 * 
 * TODO: (10) [doc] Notes on nocopy implementation
 *
 * @author     Logan Smith <logan@openlunar.org
 */

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef SLL_H
#define SLL_H

#include <stdint.h>

#define SLL_MAX_PAYLOD_LEN	128
#define SLL_OVERHEAD_LEN	5
#define SLL_MAX_MSG_LEN		(SLL_MAX_PAYLOD_LEN + SLL_OVERHEAD_LEN) // 5 = 2 sync bytes, 1 length byte, 2 crc bytes

// TODO: (10) [robustness] Assert (macro error) that SLL_MAX_MSG_LEN is lte to UINT8_T_MAX

#define SLL_SYNC_SEQ_1		0x5A
#define SLL_SYNC_SEQ_2		0x7E

typedef enum {
	SLL_DECODE_SYNC1 = 0,	// 0
	SLL_DECODE_SYNC2,		// 1
	SLL_DECODE_LEN,			// 2
	SLL_DECODE_DATA,		// 3
	SLL_DECODE_CRC1,		// 4
	SLL_DECODE_CRC2,		// 5
	SLL_DECODE_N_STATES
} sll_decode_state_t;

// TODO: (0) [refactor] I'm going to try and make this entire structure "internal state" so
// that the user shouldn't ever need to use it; It's still going to be declared
// externally so that instances of it can be declared statically (without its
// prototype the compiler can't allocate the correct amount of memory).
// 
// TODO: (0) [refactor] Rename to sll_state_t
// TODO: (0) [refactor] Remove _ctx sub-struct and reorganize fields
typedef struct {
	uint32_t length; // Payload length
	// uint8_t data[SLL_MAX_PAYLOD_LEN];
	uint8_t * data_buffer;
	uint8_t * frame_buffer;
	
	// -- Internal State -- //
	struct {
		// uint8_t * buffer; // Buffer used by codec
		sll_decode_state_t state;
		uint32_t idx;
		uint16_t crc;
	} _ctx;
} sll_decode_frame_t;

/**
 * @brief      Initialize a SLL frame structure
 *
 * @param      frame       The frame
 * @param      buffer      The buffer
 * @param[in]  buffer_len  The buffer length
 *
 * @return     { description_of_the_return_value }
 */
int sll_init( sll_decode_frame_t * const frame, uint8_t * buffer, const uint8_t buffer_len );

uint8_t * sll_get_data_buffer( sll_decode_frame_t * const frame );

// Only valid after sll_decode() returns 1
uint32_t sll_get_decoded_len( sll_decode_frame_t * const frame );

/**
 * @brief      Execute the SLL decode FSM with the character 'c'
 *
 * @param      frame  The frame
 * @param[in]  c      { parameter_description }
 *
 * @return     { description_of_the_return_value }
 */
int sll_decode( sll_decode_frame_t * frame, uint8_t c );

// no-copy prototype; the frame has the buffer references
int sll_encode( sll_decode_frame_t * const frame, uint32_t data_len );
// int sll_encode( uint8_t * out_buffer, uint8_t * data, uint8_t len );

#endif // SLL_H
