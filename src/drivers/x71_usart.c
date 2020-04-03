
#include "usart.h"
#include "config.h"
#include "common.h"

#include <stddef.h>

#define USART_CR_OFFSET		0x00
#define USART_MR_OFFSET		0x04

#define USART_CSR_OFFSET	0x14
#define USART_RHR_OFFSET	0x18
#define USART_THR_OFFSET	0x1C
#define USART_BRGR_OFFSET	0x20

static inline uint32_t __usart_getreg( volatile uint32_t base, uint32_t offset )
{
	return (*(volatile uint32_t *)(base + offset));
}

static inline void __usart_setreg( volatile uint32_t base, uint32_t offset, uint32_t value )
{
	*(volatile uint32_t *)(base + offset) = (value);
}

typedef struct usart_config_t {
	uint32_t regbase;	// Base address of USART registers
	uint8_t periph_id; // Peripheral identifier; used for NVIC and PMC
	struct { // Addresses, volatile handled by GPIO HAL (for now)
		uint32_t tx_base;
		uint32_t rx_base;

		uint8_t tx_pin;
		uint8_t tx_func;

		uint8_t rx_pin;
		uint8_t rx_func;
	} gpio;
} usart_config_t;

#if defined(CONFIG_SOC_SERIES_SAMV71)
	// -- V71 -- //
	// NOTE: [RT]XDn is USART and U[RT]Xn is UART
	// 
	// V71 xplained
	// EDBG USART
	// USART1
	// RXD1 PA21
	// TXD1 PB04 (PIO "D")
	#include "v71_pio.h"

	#define USART0_BASE	0x40024000
	#define USART1_BASE 0x40028000

	#define PMC_BASE	0x400E0600
	#define PMC_SCER	MMIO32((PMC_BASE) + 0x00)
	#define PMC_PCER0	MMIO32((PMC_BASE) + 0x10)

	#define MATRIX_BASE	0x40088000
	#define CCFG_SYSIO	MMIO32((MATRIX_BASE) + 0x114)

	#define USART_FIXED_BAUD	(20 & 0xFFFF) // 38400 at 12 MHz
	#define USART_N_PERIPH		10

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

	void __usart_hw_init( usart_config_t * usart )
	{
		// NOTE: Special case for USART0; this should really be handled by the pin manager
		if ( usart == &usart1_cfg_g ) {
			// Disable SYSIO4 function (TDI) in favor of PB4 normal function
			CCFG_SYSIO |= (1 << 4); // SYSIO4
		}

		// Configure TX pin
		PIO_ABCDSR( usart->gpio.tx_base, usart->gpio.tx_pin, usart->gpio.tx_func );
		PIO_PDR( usart->gpio.tx_base ) = (1 << usart->gpio.tx_pin );

		// Configure RX pin
		PIO_ABCDSR( usart->gpio.rx_base, usart->gpio.rx_pin, usart->gpio.rx_func );
		PIO_PDR( usart->gpio.rx_base ) = (1 << usart->gpio.rx_pin );

		// Enable peripheral clock
		// FIXME: I know that USART[0:2] are in PMC_PCER0 so I can hard-code it here; however, this should all be managed by a PMC API
		PMC_PCER0 = (1 << usart->periph_id);
	}

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

	// -- V71 -- //
#elif defined(CONFIG_SOC_SERIES_SAMRH71)
	// -- RH71 -- //
	// PF29 - FLEXCOM1_IO1 - TXD (RXD)
	// PF30 - FLEXCOM1_IO0 - RXD (TXD)

	#include "rh71_pio.h"
	#include "rh71_pmc.h"

	#define FLEXCOM_USART_BASE_OFFSET	0x200
	#define FLEXCOM_MR_OFFSET	0x00

	#define FLEXCOM0_BASE	0x40010000
	#define USART0_BASE (FLEXCOM0_BASE + FLEXCOM_USART_BASE_OFFSET)

	#define FLEXCOM1_BASE	0x40014000
	#define USART1_BASE (FLEXCOM1_BASE + FLEXCOM_USART_BASE_OFFSET) // FLEXCOM1 USART registers

	#define USART_FIXED_BAUD	(14 & 0xFFFF) // 19200 at ~4 MHz
	#define USART_N_PERIPH		10

	static usart_config_t usart0_cfg_g = {
		.regbase = USART0_BASE,
		.periph_id = 7,
		.gpio = {
			// TX = PC21
			.tx_base = PIO_GROUP_C,
			.tx_pin = 21,
			.tx_func = 1, // A
			// RX = PC22
			.rx_base = PIO_GROUP_C,
			.rx_pin = 22,
			.rx_func = 1, // A
		}
	};

	static usart_config_t usart1_cfg_g = {
		.regbase = USART1_BASE,
		.periph_id = 8,
		.gpio = {
			// TX = PF30
			.tx_base = PIO_GROUP_F,
			.tx_pin = 30,
			.tx_func = 1, // A
			// RX = PF29
			.rx_base = PIO_GROUP_F,
			.rx_pin = 29,
			.rx_func = 1, // A
		}
	};

	// NOTE: Looks like FLEXCOM is *always* function A, if the pin supports it
	// PC21 (A) - FLEXCOM0_IO0
	// PC22 (A) - FLEXCOM0_IO1
	// 
	// PF30 (A) - FLEXCOM1_IO0
	// PF29 (A) - FLEXCOM1_IO1
	// 
	// PA2 (A)  - FLEXCOM2_IO0
	// PA6 (A)  - FLEXCOM2_IO1
	// 
	// PID:
	// FLEXCOM0 - 7
	// FLEXCOM1 - 8
	// FLEXCOM2 - 13
	// 
	// I/O:
	// FLEXCOM_IO0 - TXD
	// FLEXCOM_IO1 - RXD

	// TODO: Genericize this to use the usart config struct
	void __usart_hw_init( usart_config_t * usart )
	{
		// Configure TX pin
		// PIOF_MSKR = (1 << 30) | (1 << 29); // PF30 - FLEXCOM1_IO0 ; PF29 - FLEXCOM1_IO1
		// // PIOF_CFGR = (1 << 10 /* Pull-Up */) | (1); // PF29 - A - FLEXCOM1_IO1, FUNC[2:0] = 1 : PERIPH_A
		// PIOF_CFGR = (1); // FUNC[2:0] = 1 : PERIPH_A

		PIO_MSKRx(usart->gpio.tx_base) = (1 << usart->gpio.tx_pin);
		PIO_CFGRx(usart->gpio.tx_base) = usart->gpio.tx_func;

		PIO_MSKRx(usart->gpio.rx_base) = (1 << usart->gpio.rx_pin);
		PIO_CFGRx(usart->gpio.rx_base) = usart->gpio.rx_func;

		// Configure FLEXCOM
		__usart_setreg( (usart->regbase - FLEXCOM_USART_BASE_OFFSET), FLEXCOM_MR_OFFSET, 1 ); // OPMODE[1:0] = USART = 1

		// Enable peripheral clock
		// PMC_PCR = ((1 << 28 /* EN */) | (1 << 12 /* CMD - Write Mode */) | (8 /* PID8 */));
		PMC_PCR = ((1 << 28 /* EN */) | (1 << 12 /* CMD - Write Mode */) | (usart->periph_id /* PID*/));
	}

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

	// -- RH71 -- //
#endif // CONFIG_SOC_SERIES_*

int __usart_init( usart_config_t * usart )
{
	if ( ! usart ) {
		return (-1);
	}

	__usart_hw_init( usart );

	// -- Configure USART peripheral -- //
	
	// Baud rate
	__usart_setreg( usart->regbase, USART_BRGR_OFFSET, USART_FIXED_BAUD );

	// No parity, 8 bit payload
	__usart_setreg( usart->regbase, USART_MR_OFFSET, (4 << 9 /* PAR[2:0] bits 11:9, 4 = no parity */)
		| (3 << 6 /* CHRL[1:] bits 7:6, 3 = 8 bit */) );

	// Enable transmitter
	__usart_setreg( usart->regbase, USART_CR_OFFSET, (1 << 6 /* TXEN */) | (1 << 4 /* RXEN */) );

	return 0;
}

// -- API ------------------------------------------------------------------- //

// NOTE: This is...a way to do this...
int usart_init()
{
	uint32_t i;
	usart_config_t * usart;

	for ( i = 0; i < USART_N_PERIPH; i++ ) {
		usart = __get_usart_struct( i );

		if ( ! usart ) {
			continue;
		}

		__usart_init( usart );
	}

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
