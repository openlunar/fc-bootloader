
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef CRC_H
#define CRC_H

#include <stdint.h>

#define CRC_16IBM_INIT_VALUE	0x0000

#define CRC_32_INIT_VALUE	0xFFFFFFFFUL

uint16_t crc_16ibm_update( uint16_t crc, uint8_t data );

uint32_t crc_32_update( uint32_t crc, uint8_t data );
uint32_t crc_32_finalize( uint32_t crc );
uint32_t crc_32( uint8_t * data, uint32_t len );

#endif // CRC_H

#ifdef __cplusplus
}
#endif
