

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef USART_H
#define USART_H

#include <stdint.h>

#define USART_BLOCKING 0

#define USART_FLAGS_NONBLOCK	0x1

int usart_init();

int usart_write( uint32_t usart_no, uint8_t * data, uint32_t length );

// Reads a single character
int usart_read( uint32_t usart_no, uint8_t * data, uint32_t flags );

#endif // USART_H

#ifdef __cplusplus
}
#endif
