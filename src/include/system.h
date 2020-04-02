
#ifdef __cplusplus
	extern "C" {
#endif

#ifndef SYSTEM_H
#define SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

int sys_set_boot_action( uint32_t id );
int sys_set_boot_enable();
bool sys_get_boot_enable();

void sys_boot_poll();

#endif // SYSTEM_H

#ifdef __cplusplus
}
#endif
