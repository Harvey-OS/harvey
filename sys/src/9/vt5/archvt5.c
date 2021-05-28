/*
 * rae's virtex5 ml510 ppc440x5
 * similar to the ml410 but only 4 leds, not 8.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#include	"io.h"
#include	"../port/netif.h"
#include	"etherif.h"
#include	"../ip/ip.h"

#include "reboot.h"

enum {
	Ledmask = (1<<4) - 1,	/* for 4 bits */
};

typedef struct Gpioregs Gpioregs;

struct Gpioregs {
	ulong	data;		/* write only? */
	ulong	tri;		/* tri-state */
	ulong	data2;		/* 2nd channel */
	ulong	tri2;

	/* silly bits */
	ulong	pad[67];	/* sheesh */
	ulong	gie;		/* global intr enable */
	ulong	ier;		/* intr enable */
	ulong	isr;		/* intr status */
};

void	machninit(int);

static void	twinkle(void);

void	(*archclocktick)(void) = twinkle;
/* ether= for sys=anguish */
uchar mymac[Eaddrlen] = { 0x00, 0x0A, 0x35, 0x01, 0xE1, 0x48 };
uintptr memsz;

void
startcpu(int machno)
{
	machninit(machno);
	mmuinit();
	/*
	 * can't print normally on cpu1 until we move shared locks
	 * into uncached sram and change synchronisation between
	 * the cpus fairly drastically.  cpu1 will then be a slave.
	 */
uartlputc('Z');
uartlputc('Z');
uartlputc('0'+machno);
	splhi();
	for(;;) {
		putmsr(getmsr() | MSR_WE);
		barriers();
	}
}

void
uncinit(void)
{
}

void
archreset(void)
{
	m->cpuhz = 400000000;
	m->opbhz = 66600000;
	m->clockgen = m->cpuhz;		/* it's the internal cpu clock */
}

void
archvt5link(void)
{
}

int
cmpswap(long *addr, long old, long new)
{
	return cas32(addr, old, new);
}

/*
 * only use of GPIO now is access to LEDs
 */

//static int leds;
static uchar oldbits;
static Lock lightlck;

static void
setleds(void)
{
	((Gpioregs*)Gpio)->data = ~oldbits & Ledmask;	/* 0 == lit */
	barriers();
}

static void
ledinit(void)
{
	Gpioregs *g;

	/* set access to LED */
	g = (Gpioregs*)Gpio;
/*	g->gie = 0;		/* wouldn't use intrs even if we had them */
	g->tri = 0;		/* outputs */
	barriers();
	setleds();
}

uchar
lightstate(int state)
{
	int r;

	if (m->machno != 0)
		return oldbits;
	ilock(&lightlck);
	r = oldbits;
	oldbits &= ~(Ledtrap|Ledkern|Leduser|Ledidle);
	oldbits |= state & (Ledtrap|Ledkern|Leduser|Ledidle);
	iunlock(&lightlck);
	setleds();
	return r;
}

uchar
lightbiton(int bit)
{
	int r;

	if (m->machno != 0)
		return oldbits;
	ilock(&lightlck);
	r = oldbits;
	oldbits |= bit;
	iunlock(&lightlck);
	setleds();
	return r;
}

uchar
lightbitoff(int bit)
{
	int r;

	if (m->machno != 0)
		return oldbits;
	ilock(&lightlck);
	r = oldbits;
	oldbits &= ~bit;
	iunlock(&lightlck);
	setleds();
	return r;
}

static void
twinkle(void)
{
	static int bit;

	if (m->machno != 0)
		return;
	if(m->ticks % (HZ/4) == 0) {
		bit ^= 1;
		if (bit)
			lightbiton(Ledpulse);
		else
			lightbitoff(Ledpulse);
		barriers();
	}

	if(securemem)
		qtmclock();
	if(m->ticks % HZ == 0) {
		intrs1sec = 0;
		etherclock();
	}
	barriers();
}

/*
 * size by watching for bus errors (machine checks), but avoid
 * tlb faults for unmapped memory beyond the maximum we expect.
 */
static uintptr
memsize(void)
{
	int fault;
	uintptr sz;

	/* try powers of two */
	fault = 0;
	for (sz = MB; sz != 0 && sz < MAXMEM; sz <<= 1)
		if (probeaddr(KZERO|sz) < 0) {
			fault = 1;
			break;
		}
	if (sz >= MAXMEM && !fault)
		/* special handling for maximum size */
		if (probeaddr(KZERO|MEMTOP(sz)) < 0)
			sz >>= 1;

	return securemem? MEMTOP(sz): sz;
}

void
meminit(void)
{
	securemem = gotsecuremem();

	/* size memory */
	memsz = memsize();
	conf.mem[0].npage = memsz / BY2PG;
	conf.mem[0].base = PGROUND(PADDR(end));
	conf.mem[0].npage -= conf.mem[0].base/BY2PG;

	/* avoid clobbering bootstrap's stack in case cpu1 is using it */
}

void
ioinit(void)
{
	ledinit();
	addconf("console", "0");
	addconf("nobootprompt", "tcp");
}

int
archether(int ctlno, Ether *ether)
{
	if(ctlno > 0)
		return -1;

	if (probeaddr(Temac) >= 0) {
		ether->type = "lltemac";
		ether->port = ctlno;
		return 1;
	}
	return -1;
}

void
clrmchk(void)
{
	putmcsr(~0);			/* clear machine check causes */
	sync();
	isync();
	putesr(0);			/* clears machine check */
}

/*
 * the new kernel is already loaded at address `code'
 * of size `size' and (virtual) entry point `entry'.
 */
void
reboot(void *entry, void *code, ulong size)
{
	int cpu;
	void (*f)(ulong, ulong, ulong);

	writeconf();
	shutdown(0);

	/*
	 * should be the only processor running now
	 */
	cpu = getpir();
	if (cpu != 0)
		panic("rebooting on cpu %d", cpu);

	print("shutting down...\n");
	delay(200);

	splhi();

	/* turn off buffered serial console */
//	serialoq = nil;

	/* shutdown devices */
	devtabshutdown();
	splhi();		/* device shutdowns could have lowered pl */
	intrshutdown();
	putmsr(getmsr() & ~MSR_DE);	/* disable debug facilities */
	putdbsr(~0);

	/* setup reboot trampoline function */
	f = (void*)REBOOTADDR;
	sync();
	memmove(f, rebootcode, sizeof(rebootcode));
	sync();
	dcflush(PTR2UINT(f), sizeof rebootcode);
	icflush(PTR2UINT(f), sizeof rebootcode);

	iprint("restart: entry %#p code %#p size %ld; trampoline %#p",
		entry, code, size, f);
	delay(100);		/* wait for uart to quiesce */

	/* off we go - never to return */
	dcflush(PTR2UINT(entry), size);
	icflush(PTR2UINT(entry), size);
	/*
	 * trampoline code now at REBOOTADDR will copy code to entry,
	 * then jump to entry.
	 */
	iprint("\n");
	delay(10);
	(*f)((uintptr)entry, (uintptr)code, size);

	/* never reached */
	iprint("new kernel returned!\n");
	archreboot();
}
