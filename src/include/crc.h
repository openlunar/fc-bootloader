
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef CRC_H
#define CRC_H

#include <stdint.h>

uint16_t crc_16ibm_update( uint16_t crc, uint8_t data );

uint32_t crc_32_update( uint32_t crc, uint8_t data );

#endif // CRC_H

#ifdef __cplusplus
}
#endif
