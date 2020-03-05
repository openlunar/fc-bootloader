
#include "usart.h"
#include "config.h"
#include "common.h"

#include "v71_pio.h"

// V71 xplained
// EDBG USART
// USART1
// RXD1 PA21
// TXD1 PB04 (PIO "D")

// NOTE: [RT]XDn is USART and U[RT]Xn is UART

#define USART1_BASE 0x40028000

#define USART_CR(base)		MMIO32((base) + 0x00)
#define USART_MR(base)		MMIO32((base) + 0x04)

#define USART_CSR(base)		MMIO32((base) + 0x14)
#define USART_RHR(base)		MMIO32((base) + 0x18)
#define USART_THR(base)		MMIO32((base) + 0x1C)
#define USART_BRGR(base)	MMIO32((base) + 0x20)

#define USART1_CR	USART_CR(USART1_BASE) // 0x40028000
#define USART1_MR	USART_MR(USART1_BASE)

#define USART1_CSR	USART_CSR(USART1_BASE) // 0x40028014
#define USART1_RHR	USART_RHR(USART1_BASE) // 0x40028018
#define USART1_THR	USART_THR(USART1_BASE)
#define USART1_BRGR	USART_BRGR(USART1_BASE)

#define PMC_BASE	0x400E0600
#define PMC_SCER	MMIO32((PMC_BASE) + 0x00)
#define PMC_PCER0	MMIO32((PMC_BASE) + 0x10)

#define MATRIX_BASE	0x40088000
#define CCFG_SYSIO	MMIO32((MATRIX_BASE) + 0x114)

// Default clock on V71: 12 MHz
// static const uint32_t sys_clk = 12000000;

// Fixed configuration:
// 8N1
// 38400 baud (2.3% error)
// TX only
// No interrupts enabled
int usart_init()
{
	// Disable SYSIO4 function (TDI) in favor of PB4 normal function
	CCFG_SYSIO |= (1 << 4); // SYSIO4

	// Configure TX pin
	// PIO_ABCDSR1 and PIO_ABCDSR2 both set to "1" to select "D"
	PIOB_ABCDSR1 |= (1 << 4);
	PIOB_ABCDSR2 |= (1 << 4);
	PIOB_PDR = (1 << 4);

	// -- Configure USART peripheral -- //
	
	// Enable peripheral clock
	PMC_PCER0 = (1 << 14 /* PID14 : USART1 */);

	// Baud rate
	USART1_BRGR = (20 & 0xFFFF); // Set baud rate to 38400
	// USART1_BRGR = (78 & 0xFFFF); // Set baud rate to 9600

	// No parity, 8 bit payload
	USART1_MR = (4 << 9 /* PAR[2:0] bits 11:9, 4 = no parity */)
		| (3 << 6 /* CHRL[1:] bits 7:6, 3 = 8 bit */);

	// Enable transmitter
	USART1_CR |= (1 << 6 /* TXEN */) | (1 << 4 /* RXEN */);

	return 0;
}

// Blocking transmission
int usart_write( uint32_t id, uint8_t * data, uint32_t length )
{
	// Ignore ID for now; probably not even how it's going to be implemented anyway but will give me flexibility before the full HAL is defined / implemented
	(void)id;

	// Not an error - we successfully sent 0 bytes
	if ( length == 0 ) {
		return 0;
	}

	// Is an error, data pointer is invalid
	if ( ! data ) {
		return (-1);
	}

	// Wait for the THR to be clear by polling TXRDY
	while ( length ) {
		while ( ! (USART1_CSR & (1 << 1 /* TXRDY */)) );

		// Write data to transmit data register
		USART1_THR = (*data) & 0xFF;

		// Increment the data pointer and decrement length value
		data++;
		length--;
	}

	return 0;
}

int usart_read( uint32_t id, uint8_t * data )
{
	// Ignore for now
	(void)id;

	// Is an error, data pointer is invalid
	if ( ! data ) {
		return (-1);
	}

	// NOTE: Ignoring OVRE for now - it will set when a character is received and RXRDY is already set; it overwrites the previous value

	uint32_t csr = USART1_CSR;
	while ( ! (csr & ( 1 << 0 /* RXRDY */)) ) {
		csr = USART1_CSR;
		// if ( csr  & (1 << 5 /* OVRE */) ) {
		// 	__builtin_trap();
		// }
	}

	*data = USART1_RHR;

	return 0;
}
