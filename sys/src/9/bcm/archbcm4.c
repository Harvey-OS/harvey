/*
 * bcm2711 (raspberry pi 4) architecture-specific stuff
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

typedef struct Mbox Mbox;
typedef struct Mboxes Mboxes;

#define	POWERREGS	(VIRTIO+0x100000)

Soc soc = {
	.dramsize	= 0xFC000000,
	.physio		= 0xFE000000,
	.busdram	= 0xC0000000,
	.busio		= 0x7E000000,
	.armlocal	= 0xFF800000,
	.oscfreq	= 54000000,
	.l1ptedramattrs = Cached | Buffered | L1wralloc | L1sharable,
	.l2ptedramattrs = Cached | Buffered | L2wralloc | L2sharable,
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

/*
 * Arm local regs for smp
 */
struct Mbox {
	u32int	doorbell;
	u32int	mbox1;
	u32int	mbox2;
	u32int	startcpu;
};
struct Mboxes {
	Mbox	set[4];
	Mbox	clr[4];
};

enum {
	Mboxregs	= 0x80
};

static Lock startlock[MAXMACH + 1];

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
	u32int r;
	uint part;
	char *p;

	r = cpidget();			/* main id register */
	assert((r >> 24) == 'A');
	part = (r >> 4) & MASK(12);
	switch(part){
	case 0xc07:
		p = seprint(buf, buf + size, "Cortex-A7");
		break;
	case 0xd03:
		p = seprint(buf, buf + size, "Cortex-A53");
		break;
	case 0xd08:
		p = seprint(buf, buf + size, "Cortex-A72");
		break;
	default:
		p = seprint(buf, buf + size, "Unknown-%#x", part);
		break;
	}
	seprint(p, buf + size, " r%ldp%ld",
		(r >> 20) & MASK(4), r & MASK(4));
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
	int n, max;
	char *p;

	n = 4;
	if(n > MAXMACH)
		n = MAXMACH;
	p = getconf("*ncpu");
	if(p && (max = atoi(p)) > 0 && n > max)
		n = max;
	return n;
}

static int
startcpu(uint cpu)
{
	Mboxes *mb;
	int i;
	void cpureset();

	mb = (Mboxes*)(ARMLOCAL + Mboxregs);
	if(mb->clr[cpu].startcpu)
		return -1;
	mb->set[cpu].startcpu = PADDR(cpureset);
	coherence();
	sev();
	for(i = 0; i < 1000; i++)
		if(mb->clr[cpu].startcpu == 0)
			return 0;
	mb->clr[cpu].startcpu = PADDR(cpureset);
	mb->set[cpu].doorbell = 1;
	return 0;
}

void
mboxclear(uint cpu)
{
	Mboxes *mb;

	mb = (Mboxes*)(ARMLOCAL + Mboxregs);
	mb->clr[cpu].mbox1 = 1;
}

void
wakecpu(uint cpu)
{
	Mboxes *mb;

	mb = (Mboxes*)(ARMLOCAL + Mboxregs);
	mb->set[cpu].mbox1 = 1;
}

int
startcpus(uint ncpu)
{
	int i, timeout;

	for(i = 0; i < ncpu; i++)
		lock(&startlock[i]);
	cachedwbse(startlock, sizeof startlock);
	for(i = 1; i < ncpu; i++){
		if(startcpu(i) < 0)
			return i;
		timeout = 10000000;
		while(!canlock(&startlock[i]))
			if(--timeout == 0)
				return i;
		unlock(&startlock[i]);
	}
	return ncpu;
}

void
archbcm4link(void)
{
	addclock0link(wdogfeed, HZ);
}

int
archether(unsigned ctlrno, Ether *ether)
{
	switch(ctlrno){
	case 0:
		ether->type = "genet";
		break;
	case 1:
		ether->type = "4330";
		break;
	default:
		return 0;
	}
	ether->ctlrno = ctlrno;
	ether->irq = -1;
	ether->nopt = 0;
	ether->maxmtu = 9014;
	return 1;
}

int
l2ap(int ap)
{
	return (AP(0, (ap)));
}

int
cmpswap(long *addr, long old, long new)
{
	return cas((ulong*)addr, old, new);
}

void
cpustart(int cpu)
{
	Mboxes *mb;
	void machon(int);

	up = nil;
	machinit();
	mb = (Mboxes*)(ARMLOCAL + Mboxregs);
	mb->clr[cpu].doorbell = 1;
	trapinit();
	clockinit();
	mmuinit1();
	timersinit();
	cpuidprint();
	archreset();
	machon(m->machno);
	unlock(&startlock[cpu]);
	schedinit();
	panic("schedinit returned");
}
