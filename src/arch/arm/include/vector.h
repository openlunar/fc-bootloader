

#ifdef __cplusplus
	extern "C" {
#endif

#ifndef VECTOR_H
#define VECTOR_H

#include <stdint.h>

typedef void (*vector_table_entry_t)(void);

typedef struct {
	uint32_t *initial_sp_value; /**< Initial stack pointer value. */
	vector_table_entry_t reset;
	vector_table_entry_t nmi;
	vector_table_entry_t hard_fault;
	vector_table_entry_t memory_manage_fault; /* not in CM0 */
	vector_table_entry_t bus_fault;           /* not in CM0 */
	vector_table_entry_t usage_fault;         /* not in CM0 */
	vector_table_entry_t reserved_x001c[4];
	vector_table_entry_t sv_call;
	vector_table_entry_t debug_monitor;       /* not in CM0 */
	vector_table_entry_t reserved_x0034;
	vector_table_entry_t pend_sv;
	vector_table_entry_t systick;
	// vector_table_entry_t irq[NVIC_IRQ_COUNT];
} vector_table_t;

void blocking_handler( void );
void null_handler( void );
void reset_handler();

int main();

#endif // VECTOR_H

#ifdef __cplusplus
}
#endif

