/*
 * bcm2835 (e.g. raspberry pi) architecture-specific stuff
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "arm.h"

#include "../port/netif.h"
#include "etherif.h"

#define	POWERREGS	(VIRTIO+0x100000)

enum {
	Wdogfreq	= 65536,
	Wdogtime	= 5,	/* seconds, ≤ 15 */
};

/*
 * Power management / watchdog registers
 */
enum {
	Rstc		= 0x1c>>2,
		Password	= 0x5A<<24,
		CfgMask		= 0x03<<4,
		CfgReset	= 0x02<<4,
	Rsts		= 0x20>>2,
	Wdog		= 0x24>>2,
};

void
archreset(void)
{
	fpon();
}

void
archreboot(void)
{
	u32int *r;

	r = (u32int*)POWERREGS;
	r[Wdog] = Password | 1;
	r[Rstc] = Password | (r[Rstc] & ~CfgMask) | CfgReset;
	coherence();
	for(;;)
		;
}

static void
wdogfeed(void)
{
	u32int *r;

	r = (u32int*)POWERREGS;
	r[Wdog] = Password | (Wdogtime * Wdogfreq);
	r[Rstc] = Password | (r[Rstc] & ~CfgMask) | CfgReset;
}

void
wdogoff(void)
{
	u32int *r;

	r = (u32int*)POWERREGS;
	r[Rstc] = Password | (r[Rstc] & ~CfgMask);
}
	
void
cpuidprint(void)
{
	print("cpu%d: %dMHz ARM1176JZF-S\n", m->machno, m->cpumhz);
}

void
archbcmlink(void)
{
	addclock0link(wdogfeed, HZ);
}

int
archether(unsigned ctlrno, Ether *ether)
{
	switch(ctlrno) {
	case 0:
		ether->type = "usb";
		ether->ctlrno = ctlrno;
		ether->irq = -1;
		ether->nopt = 0;
		ether->mbps = 100;
		return 1;
	}
	return -1;
}

