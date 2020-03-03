
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef RH71_PMC_H
#define RH71_PMC_H

#include "common.h"
#include <stdint.h>

#define PMC_BASE	0x4000C000
// #define PMC_SCER	MMIO32((PMC_BASE) + 0x00)
#define PMC_PCR		MMIO32((PMC_BASE) + 0x10C)

#endif // RH71_PMC_H

#ifdef __cplusplus
}
#endif
