
#include "watchdog.h"
#include "config.h"
#include "common.h"

#define WDT_BASE	0x40100250
#define WDT_MR		MMIO32((WDT_BASE) + 0x04)

int watchdog_disable()
{
	WDT_MR |= (1 << 15 /* WDDIS */);

	return 0;
}
