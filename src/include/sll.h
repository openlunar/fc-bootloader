/**
 * @brief      Stand-in Link Layer
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
#define SLL_MAX_MSG_LEN		(SLL_MAX_PAYLOD_LEN + 5) // 5 = 2 sync bytes, 1 length byte, 2 crc bytes

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

typedef struct {
	uint8_t length; // Payload length
	uint8_t data[SLL_MAX_PAYLOD_LEN];
	
	// -- Internal State -- //
	struct {
		sll_decode_state_t state;
		uint8_t idx;
		uint16_t crc;
	} _ctx;
} sll_decode_frame_t;

/**
 * @brief      Initialize a SLL frame structure
 *
 * @param      frame  The frame
 *
 * @return     { description_of_the_return_value }
 */
int sll_init( sll_decode_frame_t * const frame );

/**
 * @brief      Execute the SLL decode FSM with the character 'c'
 *
 * @param      frame  The frame
 * @param[in]  c      { parameter_description }
 *
 * @return     { description_of_the_return_value }
 */
int sll_decode( sll_decode_frame_t * frame, uint8_t c );

int sll_encode( uint8_t * buffer, uint8_t * data, uint8_t len );

#endif // SLL_H
