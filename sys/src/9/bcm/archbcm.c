/*
 * bcm2835 (e.g. original raspberry pi) architecture-specific stuff
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

Soc soc = {
	.dramsize	= 512*MiB,
	.physio		= 0x20000000,
	.busdram	= 0x40000000,
	.busio		= 0x7E000000,
	.armlocal	= 0,
	.l1ptedramattrs = Cached | Buffered,
	.l2ptedramattrs = Cached | Buffered,
};

enum {
	Wdogfreq	= 65536,
	Wdogtime	= 10,	/* seconds, ≤ 15 */
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

void
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
	
char *
cputype2name(char *buf, int size)
{
	seprint(buf, buf + size, "1176JZF-S");
	return buf;
}

void
cpuidprint(void)
{
	char name[64];

	cputype2name(name, sizeof name);
	delay(50);				/* let uart catch up */
	print("cpu%d: %dMHz ARM %s\n", m->machno, m->cpumhz, name);
}

int
getncpus(void)
{
	return 1;
}

int
startcpus(uint)
{
	return 1;
}

void
archbcmlink(void)
{
	addclock0link(wdogfeed, HZ);
}

int
archether(unsigned ctlrno, Ether *ether)
{
	ether->type = "usb";
	ether->ctlrno = ctlrno;
	ether->irq = -1;
	ether->nopt = 0;
	return 1;
}

int
l2ap(int ap)
{
	return (AP(3, (ap))|AP(2, (ap))|AP(1, (ap))|AP(0, (ap)));
}

/*
 * atomic ops
 * make sure that we don't drag in the C library versions
 */

long
adec(long *p)
{
	long s, v;

	s = splhi();
	v = --*p;
	splx(s);
	return v;
}

long
ainc(long *p)
{
	long s, v;

	s = splhi();
	v = ++*p;
	splx(s);
	return v;
}

int
cas32(void* addr, u32int old, u32int new)
{
	int r, s;

	s = splhi();
	if(r = (*(u32int*)addr == old))
		*(u32int*)addr = new;
	splx(s);
	if (r)
		coherence();
	return r;
}

int
cmpswap(long *addr, long old, long new)
{
	return cas32(addr, old, new);
}

