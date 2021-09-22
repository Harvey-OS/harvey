#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "riscv.h"

static void
beaglereset(void)
{
	if(soc.wdog0) {
		u32int *wd = (u32int*)soc.wdog0;

		wd[0x13c/4] = 0x378f0765;	/* unlock regs */
		wd[0x104/4] = 1;	/* reset, not interrupt, on timer expiry */
		wd[0x108/4] = 0;	/* watchdog timer count */
		wd[0x110/4] = 1;	/* watchdog enable */
		coherence();
		for (;;)
			halt();
	}
}

void (*rvreset)(void) = beaglereset;
