
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef V71_PIO_H
#define V71_PIO_H

#define PIOA_BASE	0x400E0E00
#define PIOB_BASE	0x400E1000
#define PIOC_BASE	0x400E1200

#define PIO_PER(port) 		MMIO32((port) + 0x00) // PIO Enable Register
#define PIO_PDR(port) 		MMIO32((port) + 0x04) // PIO Disable Register
#define PIO_OER(port) 		MMIO32((port) + 0x10) // Output Enable Register
#define PIO_SODR(port) 		MMIO32((port) + 0x30) // Set Output Data Register
#define PIO_CODR(port) 		MMIO32((port) + 0x34) // Clear Output Data Register
#define PIO_ODSR(port) 		MMIO32((port) + 0x38) // Output Data Status Register

#define PIO_ABCDSR1(port) 		MMIO32((port) + 0x70)
#define PIO_ABCDSR2(port) 		MMIO32((port) + 0x74)

// #define PIOA_PER	PIO_PER(PIOA_BASE)
#define PIOA_OER		PIO_OER(PIOA_BASE)
#define PIOA_PDR		PIO_PDR(PIOA_BASE)
#define PIOA_SODR		PIO_SODR(PIOA_BASE)
#define PIOA_CODR		PIO_CODR(PIOA_BASE)
#define PIOA_ODSR		PIO_ODSR(PIOA_BASE)
#define PIOA_ABCDSR1	PIO_ABCDSR1(PIOA_BASE)
#define PIOA_ABCDSR2	PIO_ABCDSR2(PIOA_BASE)

#define PIOB_PER		PIO_PER(PIOB_BASE)
#define PIOB_PDR		PIO_PDR(PIOB_BASE)
#define PIOB_OER		PIO_OER(PIOB_BASE)
#define PIOB_SODR		PIO_SODR(PIOB_BASE)
#define PIOB_CODR		PIO_CODR(PIOB_BASE)
#define PIOB_ODSR		PIO_ODSR(PIOB_BASE)
#define PIOB_ABCDSR1	PIO_ABCDSR1(PIOB_BASE)
#define PIOB_ABCDSR2	PIO_ABCDSR2(PIOB_BASE)

#define PIOC_OER	PIO_OER(PIOC_BASE)
#define PIOC_SODR	PIO_SODR(PIOC_BASE)
#define PIOC_CODR	PIO_CODR(PIOC_BASE)
#define PIOC_ODSR	PIO_ODSR(PIOC_BASE)

#endif // V71_PIO_H

#ifdef __cplusplus
}
#endif
