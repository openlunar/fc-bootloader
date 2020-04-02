
#include "usart.h"
#include "config.h"
#include "common.h"

#include "v71_pio.h"

#include <stddef.h>

// V71 xplained
// EDBG USART
// USART1
// RXD1 PA21
// TXD1 PB04 (PIO "D")

// NOTE: [RT]XDn is USART and U[RT]Xn is UART

#define USART0_BASE	0x40024000
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

// -------------------------------------------------------------------------- //
// Developing basic (shitty) support for multiple USART instances

// Eh, this will do for now

#define USART_CR_OFFSET		0x00
#define USART_MR_OFFSET		0x04

#define USART_CSR_OFFSET	0x14
#define USART_RHR_OFFSET	0x18
#define USART_THR_OFFSET	0x1C
#define USART_BRGR_OFFSET	0x20

inline uint32_t __usart_getreg( volatile uint32_t base, uint32_t offset )
{
	return (*(volatile uint32_t *)(base + offset));
}

inline void __usart_setreg( volatile uint32_t base, uint32_t offset, uint32_t value )
{
	*(volatile uint32_t *)(base + offset) = (value);
}

typedef struct usart_config_t {
	uint32_t regbase;	// Base address of USART registers
	// uint32_t nvic_irq; // IRQ number

	uint8_t periph_id; // Peripheral identifier; used for NVIC and PMC

	// uint32_t baud;

	// Other potential bit settings: rx_enable, tx_enable, rx_tx_swap, data_invert, msb_first, n_bits
	// uint32_t stop_bits	: 2;
	// uint32_t parity		: 2;
	// uint32_t rx_invert	: 1;
	// uint32_t tx_invert	: 1;
	// uint32_t usused		: 24;
	// uint8_t parity;

	struct { // Addresses, volatile handled by GPIO HAL (for now)
		uint32_t tx_base;
		uint32_t rx_base;

		uint8_t tx_pin;
		uint8_t tx_func;

		uint8_t rx_pin;
		uint8_t rx_func;
	} gpio;

	// struct {
	// 	volatile uint32_t * rcc; // The current HAL expects a pointer
	// 	uint32_t en;
	// } clk;

	// rb_context_t tx_buf_g;
	// rb_context_t rx_buf_g;

	// void (* rx_callback)( struct usart_config_t * usart, uint8_t data ); // Ignoring that data could be 9-bits for now...

	// clk_t clk;
} usart_config_t;

static usart_config_t usart0_cfg_g = {
	.regbase = USART0_BASE,
	.periph_id = 13,
	.gpio = {
		// TX = PB01
		.tx_base = PIOB_BASE,
		.tx_pin = 1,
		.tx_func = PIO_ACBDSR_PERIPH_C,
		// RX = PB00
		.rx_base = PIOB_BASE,
		.rx_pin = 0,
		.rx_func = PIO_ACBDSR_PERIPH_C
	}
};

static usart_config_t usart1_cfg_g = {
	.regbase = USART1_BASE,
	.periph_id = 14,
	.gpio = {
		// TX = PB04
		.tx_base = PIOB_BASE,
		.tx_pin = 4,
		.tx_func = PIO_ACBDSR_PERIPH_D,
		// RX = PA21
		.rx_base = PIOA_BASE,
		.rx_pin = 21,
		.rx_func = PIO_ACBDSR_PERIPH_A
	}
};

// -------------------------------------------------------------------------- //

// Default clock on V71: 12 MHz
// static const uint32_t sys_clk = 12000000;

usart_config_t * __get_usart_struct( uint32_t usart_no )
{
	switch ( usart_no ) {
		case 0:
			return &usart0_cfg_g;
		case 1:
			return &usart1_cfg_g;
		default:
			return NULL; // Invalid USART number
	}

	// Not reached
}

// Fixed configuration:
// 8N1
// 38400 baud (2.3% error)
// TX only
// No interrupts enabled
int __usart_init( uint32_t usart_no )
{
	usart_config_t * usart = __get_usart_struct( usart_no );
	if ( ! usart ) {
		return (-1);
	}

	// NOTE: Special case for USART0; this should really be handled by the pin manager
	if ( usart_no == 1 ) {
		// Disable SYSIO4 function (TDI) in favor of PB4 normal function
		CCFG_SYSIO |= (1 << 4); // SYSIO4
	}

	// Configure TX pin
	PIO_ABCDSR( usart->gpio.tx_base, usart->gpio.tx_pin, usart->gpio.tx_func );
	PIO_PDR( usart->gpio.tx_base ) = (1 << usart->gpio.tx_pin );

	// Configure RX pin
	PIO_ABCDSR( usart->gpio.rx_base, usart->gpio.rx_pin, usart->gpio.rx_func );
	PIO_PDR( usart->gpio.rx_base ) = (1 << usart->gpio.rx_pin );

	// -- Configure USART peripheral -- //
	
	// Enable peripheral clock
	// FIXME: I know that USART[0:2] are in PMC_PCER0 so I can hard-code it here; however, this should all be managed by a PMC API
	PMC_PCER0 = (1 << usart->periph_id);

	// Baud rate
	__usart_setreg( usart->regbase, USART_BRGR_OFFSET, (20 & 0xFFFF) );

	// No parity, 8 bit payload
	__usart_setreg( usart->regbase, USART_MR_OFFSET, (4 << 9 /* PAR[2:0] bits 11:9, 4 = no parity */)
		| (3 << 6 /* CHRL[1:] bits 7:6, 3 = 8 bit */) );

	// Enable transmitter
	__usart_setreg( usart->regbase, USART_CR_OFFSET, (1 << 6 /* TXEN */) | (1 << 4 /* RXEN */) );

	return 0;
}

int usart_init()
{
	__usart_init( 0 );
	__usart_init( 1 );

	return 0;
}

// Blocking transmission
int usart_write( uint32_t usart_no, uint8_t * data, uint32_t length )
{
	usart_config_t * usart = __get_usart_struct( usart_no );
	if ( ! usart ) {
		return (-1);
	}

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
		while ( ! (__usart_getreg( usart->regbase, USART_CSR_OFFSET ) & (1 << 1 /* TXRDY */)) );

		// Write data to transmit data register
		__usart_setreg( usart->regbase, USART_THR_OFFSET, *data );

		// Increment the data pointer and decrement length value
		data++;
		length--;
	}

	return 0;
}

int usart_read( uint32_t usart_no, uint8_t * data, uint32_t flags )
{
	usart_config_t * usart = __get_usart_struct( usart_no );
	if ( ! usart ) {
		return (-1);
	}

	// Is an error, data pointer is invalid
	if ( ! data ) {
		return (-1);
	}

	// NOTE: Ignoring OVRE for now - it will set when a character is received and RXRDY is already set; it overwrites the previous value

	uint32_t csr = __usart_getreg( usart->regbase, USART_CSR_OFFSET );

	if ( flags & USART_FLAGS_NONBLOCK ) {
		if ( ! (csr & ( 1 << 0 /* RXRDY */)) ) {
			return 0;
		}
	} else {
		while ( ! (csr & ( 1 << 0 /* RXRDY */)) ) {
			// csr = USART1_CSR;
			csr = __usart_getreg( usart->regbase, USART_CSR_OFFSET );
			// if ( csr  & (1 << 5 /* OVRE */) ) {
			// 	__builtin_trap();
			// }
		}
	}

	*data = __usart_getreg( usart->regbase, USART_RHR_OFFSET );;
	// __DSB();

	return 1;
}
