

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef V71_USART_H
#define V71_USART_H

#include <stdint.h>

int usart_init();

int usart_write( uint32_t id, uint8_t * data, uint32_t length );

// Reads a single character
int usart_read( uint32_t id, uint8_t * data );

#endif // V71_USART_H

#ifdef __cplusplus
}
#endif
