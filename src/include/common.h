
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

// TODO: (90) [refactor] @organization Move the inline assembler arguments to an ARM header

__attribute__((always_inline)) static inline void __ISB(void)
{
  __asm volatile ("isb 0xF":::"memory");
}

__attribute__((always_inline)) static inline void __DSB(void)
{
  __asm volatile ("dsb 0xF":::"memory");
}

// Set Main Stack Pointer
__attribute__( ( always_inline ) ) static inline void __set_MSP( uint32_t top_of_stack )
{
	__asm volatile ( "MSR msp, %0\n" : : "r" (top_of_stack) : "sp" );
}

#define MMIO32(addr)		(*(volatile uint32_t *)(addr))

#endif // COMMON_H

#ifdef __cplusplus
}
#endif

