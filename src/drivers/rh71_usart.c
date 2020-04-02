
#include "usart.h"

#include "rh71_pio.h"
#include "rh71_pmc.h"
#include "common.h"
// #include "v71_pio.h"

// USART dependencies:
// - PIO : Configure pin as USART function
// - PMC : Enable peripheral clock; (eventually) know system clock so baud clock divisor can be calculated
// - (Potentially; definitely in application) Vector table access (install interrupt handlers)

// PF29 - FLEXCOM1_IO1 - TXD (RXD)
// PF30 - FLEXCOM1_IO0 - RXD (TXD)

#define FLEXCOM1_BASE	0x40014000
#define FLEX1_MR MMIO32(FLEXCOM1_BASE + 0x00)

#define USART1_BASE (FLEXCOM1_BASE + 0x200) // FLEXCOM1 USART registers

#define USART_CR(base)		MMIO32((base) + 0x00)
#define USART_MR(base)		MMIO32((base) + 0x04)

#define USART_CSR(base)		MMIO32((base) + 0x14)
#define USART_RHR(base)		MMIO32((base) + 0x18)
#define USART_THR(base)		MMIO32((base) + 0x1C)
#define USART_BRGR(base)	MMIO32((base) + 0x20)

#define USART1_CR	USART_CR(USART1_BASE)
#define USART1_MR	USART_MR(USART1_BASE)

#define USART1_CSR	USART_CSR(USART1_BASE)
#define USART1_RHR	USART_RHR(USART1_BASE)
#define USART1_THR	USART_THR(USART1_BASE)
#define USART1_BRGR	USART_BRGR(USART1_BASE)

// Default clock on V71: 4 MHz
// static const uint32_t sys_clk = 4000000;

// Fixed configuration:
// 8N1
// 19200 baud
// TX only
// No interrupts enabled
int usart_init()
{
	// Configure TX pin
	// PIOF_MSKR = (1 << 29); // PF29 - FLEXCOM1_IO1
	// PIOF_MSKR = (1 << 30); // PF29 - FLEXCOM1_IO1
	PIOF_MSKR = (1 << 30) | (1 << 29); // PF30 - FLEXCOM1_IO0 ; PF29 - FLEXCOM1_IO1
	// PIOF_CFGR = (1 << 10 /* Pull-Up */) | (1); // PF29 - A - FLEXCOM1_IO1, FUNC[2:0] = 1 : PERIPH_A
	PIOF_CFGR = (1); // FUNC[2:0] = 1 : PERIPH_A

	// Configure FLEXCOM
	FLEX1_MR = 1; // OPMODE[1:0] = USART = 1

	// -- Configure USART peripheral -- //
	
	// Enable peripheral clock
	PMC_PCR = ((1 << 28 /* EN */) | (1 << 12 /* CMD - Write Mode */) | (8 /* PID8 */));
	// PMC_PCR = ((1 << 28 /* EN */) | (1 << 12 /* CMD - Write Mode */) | (34 /* PID34 */));

	// Baud rate
	// USART1_BRGR = (7 & 0xFFFF); // Set baud rate to 38400
	USART1_BRGR = (14 & 0xFFFF); // Set baud rate to 19200
	// USART1_BRGR = (78 & 0xFFFF); // Set baud rate to 9600

	// No parity, 8 bit payload
	USART1_MR = (4 << 9 /* PAR[2:0] bits 11:9, 4 = no parity */)
		| (3 << 6 /* CHRL[1:] bits 7:6, 3 = 8 bit */);

	// Enable transmitter and receiver
	USART1_CR = (1 << 6 /* TXEN */) | (1 << 4 /* RXEN */);

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

int usart_read( uint32_t id, uint8_t * data, uint32_t flags )
{
	// Ignore for now
	(void)id;
	(void)flags;

	// Is an error, data pointer is invalid
	if ( ! data ) {
		return (-1);
	}

	// NOTE: Ignoring OVRE for now - it will set when a character is received and RXRDY is already set; it overwrites the previous value

	uint32_t csr = USART1_CSR;
#if (USART_BLOCKING == 1)
	while ( ! (csr & ( 1 << 0 /* RXRDY */)) ) {
		csr = USART1_CSR;
		// if ( csr  & (1 << 5 /* OVRE */) ) {
		// 	__builtin_trap();
		// }
	}
#else
	if ( ! (csr & ( 1 << 0 /* RXRDY */)) ) {
		return 0;
	}
#endif

	*data = USART1_RHR;

	return 1;
}
