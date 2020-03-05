

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef RH71_PIO_H
#define RH71_PIO_H

#include "common.h"

// Of course the PIO control is different.
// The set of registers interface with an I/O group:
// 0 - PIOA
// 1 - PIOB
// 2 - PIOC
// 3 - PIOD
// 4 - PIOE
// 5 - PIOF
// 6 - PIOG

#define PIO_BASE	0x40008000

#define PIO_MSKRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x00) // 
#define PIO_CFGRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x04) // 
#define PIO_PDSRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x08) // 
#define PIO_LOCKSRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x0C) // 
#define PIO_SODRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x10) // 
#define PIO_CODRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x14) // 
#define PIO_ODSRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x18) // 
#define PIO_IERx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x20) // 
#define PIO_IDRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x24) // 
#define PIO_IMRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x28) // 
#define PIO_ISRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x2C) // 
#define PIO_IOFRx(x) 		MMIO32((PIO_BASE) + (0x40 * x) + 0x3C) // 

#define PIO_GROUP_A		0
#define PIO_GROUP_B		1
#define PIO_GROUP_C		2
#define PIO_GROUP_D		3
#define PIO_GROUP_E		4
#define PIO_GROUP_F		5
#define PIO_GROUP_G		6

#define PIOB_MSKR	PIO_MSKRx(PIO_GROUP_B)
#define PIOB_CFGR	PIO_CFGRx(PIO_GROUP_B)
#define PIOB_SODR	PIO_SODRx(PIO_GROUP_B)
#define PIOB_CODR	PIO_CODRx(PIO_GROUP_B)
#define PIOB_ODSR	PIO_ODSRx(PIO_GROUP_B)

#define PIOF_MSKR	PIO_MSKRx(PIO_GROUP_F)
#define PIOF_CFGR	PIO_CFGRx(PIO_GROUP_F)

#endif // RH71_PIO_H

#ifdef __cplusplus
}
#endif
