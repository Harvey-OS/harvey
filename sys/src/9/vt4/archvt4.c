/*
 * rae's virtex4 ml410 ppc405
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

static void	twinkle(void);

void	(*archclocktick)(void) = twinkle;
/* ether= for sys=torment */
uchar mymac[Eaddrlen] = { 0x00, 0x0A, 0x35, 0x01, 0x8B, 0xB1 };
uintptr memsz;

void
startcpu(int)
{
	for (;;)
		;
}

void
uncinit(void)
{
}

void
archreset(void)
{
	m->cpuhz = 300000000;
	m->opbhz = 66600000;
	m->clockgen = m->cpuhz;		/* it's the internal cpu clock */
}

void
archvt4link(void)
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

static int leds;
static uchar oldbits;
static Lock lightlck;

static void
ledinit(void)
{
	Gpioregs *g;

	/* set access to LED */
	g = (Gpioregs*)Gpio;
/*	g->gie = 0;		/* wouldn't use intrs even if we had them */
	g->tri = 0;		/* outputs */
	g->data = ~0;
	barriers();
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
	((Gpioregs*)Gpio)->data = (uchar)~oldbits;	/* 0 == lit */
	iunlock(&lightlck);
	barriers();
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
	((Gpioregs*)Gpio)->data = (uchar)~oldbits;	/* 0 == lit */
	iunlock(&lightlck);
	barriers();
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
	((Gpioregs*)Gpio)->data = (uchar)~oldbits;	/* 0 == lit */
	iunlock(&lightlck);
	barriers();
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
 * virtex 4 systems always have 128MB.
 */
uintptr
memsize(void)
{
	uintptr sz = 128*MB;

	return securemem? MEMTOP(sz): sz;
}

void
meminit(void)
{
	ulong paddr;
	ulong *vaddr;
	static int ro[] = {0, -1};

	/* size memory */
	securemem = gotsecuremem();
	memsz = memsize();
	conf.mem[0].npage = memsz / BY2PG;
	conf.mem[0].base = PGROUND(PADDR(end));
	conf.mem[0].npage -= conf.mem[0].base/BY2PG;

	/* verify that it really is memory */
	for (paddr = ROUNDUP(PADDR(end), MB); paddr + BY2WD*(12+1) < memsz;
	    paddr += MB) {
		vaddr = (ulong *)KADDR(paddr + BY2WD*12);  /* a few bytes in */
		*vaddr = paddr;
		barriers();
		dcflush(PTR2UINT(vaddr), BY2WD);
		if (*vaddr != paddr)
			panic("address %#lux is not memory", paddr);
	}

	memset(end, 0, memsz - PADDR(end));
	sync();
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

	ether->type = "lltemac";
	ether->port = ctlno;
	return 1;
}

void
clrmchk(void)
{
	putesr(0);			/* clears machine check */
}

/*
 * the new kernel is already loaded at address `code'
 * of size `size' and entry point `entry'.
 */
void
reboot(void *entry, void *code, ulong size)
{
	void (*f)(ulong, ulong, ulong);

	writeconf();
	shutdown(0);

	/*
	 * should be the only processor running now
	 */

	print("shutting down...\n");
	delay(200);

	splhi();

	/* turn off buffered serial console */
//	serialoq = nil;

	/* shutdown devices */
	devtabshutdown();

	/* setup reboot trampoline function */
	f = (void*)REBOOTADDR;
	memmove(f, rebootcode, sizeof(rebootcode));
	sync();
	dcflush(PTR2UINT(f), sizeof rebootcode);
	icflush(PTR2UINT(f), sizeof rebootcode);

	print("rebooting...");
	iprint("entry %#lux code %#lux size %ld\n",
		PADDR(entry), PADDR(code), size);
	delay(100);		/* wait for uart to quiesce */

	/* off we go - never to return */
	sync();
	dcflush(PTR2UINT(entry), size);
	icflush(PTR2UINT(entry), size);
	(*f)(PADDR(entry), PADDR(code), size);

	iprint("kernel returned!\n");
	archreboot();
}
