// Don't know about the name of this file for now but whatever

#include "printf.h"
#include "usart.h"

void _putchar( char character )
{
	usart_write( 0, (uint8_t *)&character, 1 );
}
