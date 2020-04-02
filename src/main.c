
// Project common headers
#include "config.h"
#include "common.h"
#include "printf.h"
#include "system.h"
#include "moon/server.h"

// Architecture headers
#include "vector.h"

// SoC headers
#include "usart.h"
#include "flash.h"
#include "watchdog.h"

// Standard / system headers
#include <stdint.h>

int main()
{
	// Disable watchdog (WDT0); WDT_MR can only be written once after reset
	// TODO: (90) @eventually Remove this - the application will configure the WDT; the bootloader will just have to deal with this for now (16s timeout)
	watchdog_disable();
	usart_init();
	flash_init();

	printf("-- OLF Bootloader --\n\r");

	moon_server_init();

	while (1) {
		moon_server_poll();

		// A function call inside of server_poll (through the RPC API) will set
		// some state variable that causes the bootloader to jump to application
		// code; that happens here:
		//
		// ...HERE...

		sys_boot_poll();

	}

	return 0;
}
