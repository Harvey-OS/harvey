/*
 *	EB164 and similar
 *	CPU:	21164
 *	Core Logic: 21172 CIA or 21174 PYXIS
  */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

static ulong *core;
static ulong *wind;

static ulong windsave[16];
static ulong coresave[1];

ulong	iobase0;
ulong	iobase1;
#define	iobase(p)	(iobase0+(p))

static int
ident(void)
{
	return 0;	/* bug! */
}

static uvlong* sgmap;

static void
sginit(void)
{
	ulong pa;
	uvlong *pte;

	sgmap = xspanalloc(BY2PG, BY2PG, 0);
	memset(sgmap, 0, BY2PG);

	/*
	 * Prepare scatter-gather map for 0-8MB.
	 */
	pte = sgmap;
	for(pa = 0; pa < 8*1024*1024; pa += BY2PG)
		*pte++ = ((pa>>PGSHIFT)<<1)|1;

	/*
	 * Set up a map for ISA DMA accesses to physical memory.
	 * Addresses presented by an ISA device between ISAWINDOW
	 * and ISAWINDOW+8MB will be translated to between 0 and
	 * 0+8MB of physical memory.
	 */
	wind[0x400/4] = ISAWINDOW|2|1;		/* window base */
	wind[0x440/4] = 0x00700000;		/* window mask */
	wind[0x480/4] = PADDR(sgmap)>>2;	/* <33:10> of sg map */

	wind[0x100/4] = 3;			/* invalidate tlb cache */
}

static void *
kmapio(ulong space, ulong offset, int size)
{
	return kmapv(((uvlong)space<<32LL)|offset, size);
}

static void
coreinit(void)
{
	int i;

	core = kmapio(0x87, 0x40000000, 0x10000);
	wind = kmapio(0x87, 0x60000000, 0x1000);

	iobase0 = (ulong)kmapio(0x89, 0, 0x20000);

	/* hae_io = core[0x440/4];
	iobase1 = (ulong)kmapio(0x89, hae_io, 0x10000); */

	/* save critical parts of hardware memory mapping */
	for (i = 4; i < 8; i++) {
		windsave[4*(i-4)+0] = wind[(i*0x100+0x00)/4];
		windsave[4*(i-4)+1] = wind[(i*0x100+0x40)/4];
		windsave[4*(i-4)+2] = wind[(i*0x100+0x80)/4];
	}
	coresave[0] = core[0x140/4];

	/* disable windows */
	wind[0x400/4] = 0;
	wind[0x500/4] = 0;
	wind[0x600/4] = 0;
	wind[0x700/4] = 0;

	sginit();

	/*
	 * Set up a map for PCI DMA accesses to physical memory.
	 * Addresses presented by a PCI device between PCIWINDOW
	 * and PCIWINDOW+1GB will be translated to between 0 and
	 * 0+1GB of physical memory.
	 */
	wind[0x500/4] = PCIWINDOW|1;
	wind[0x540/4] = 0x3ff00000;
	wind[0x580/4] = 0;

	/* clear error state */
	core[0x8200/4] = 0x7ff;

	/* set config: byte/word enable, no monster window, etc. */
	core[0x140/4] = 0x21;

	/* turn off mcheck on master abort.  now we can probe PCI space. */
	core[0x8280/4] &= ~(1<<7);

	/* set up interrupts. */
	i8259init();
	cserve(52, 4);		/* enable SIO interrupt */
}

void
ciaerror(void)
{
	print("cia error 0x%luX\n", core[0x8200/4]);
}

static void
corehello(void)
{
	print("cpu%d: CIA revision %ld; cnfg %lux cntrl %lux\n",
			0,	/* BUG */
			core[0x80/4] & 0x7f, core[0x140/4], core[0x100/4]);
	print("cpu%d: HAE_IO %lux\n", 0, core[0x440/4]);
	print("\n");
}

static void
coredetach(void)
{
	int i;

	for (i = 4; i < 8; i++) {
		wind[(i*0x100+0x00)/4] = windsave[4*(i-4)+0];
		wind[(i*0x100+0x40)/4] = windsave[4*(i-4)+1];
		wind[(i*0x100+0x80)/4] = windsave[4*(i-4)+2];
	}
	core[0x140/4] = coresave[0];
/*	for (i = 0; i < 4; i++)
		if (i != 4)
			cserve(53, i);		/* disable interrupts */
}

static Lock	pcicfgl;
static ulong	pcimap[256];

static void*
pcicfg2117x(int tbdf, int rno)
{
	int space, bus;
	ulong base;

	bus = BUSBNO(tbdf);
	lock(&pcicfgl);
	base = pcimap[bus];
	if (base == 0) {
		if(bus)
			space = 0x8B;
		else
			space = 0x8A;
		pcimap[bus] = base = (ulong)kmapio(space, MKBUS(0, bus, 0, 0), (1<<16));
	}
	unlock(&pcicfgl);
	return (void*)(base + BUSDF(tbdf) + rno);
}

static void*
pcimem2117x(int addr, int len)
{
	return kmapio(0x88, addr, len);
}

static int
intrenable164(Vctl *v)
{
	int vec, irq;

	irq = v->irq;
	if(irq > MaxIrqPIC) {
		print("intrenable: irq %d out of range\n", v->irq);
		return -1;
	}
	if(BUSTYPE(v->tbdf) == BusPCI) {
		vec = irq+VectorPCI;
		cserve(52, irq);
	}
	else {
		vec = irq+VectorPIC;
		if(i8259enable(irq, v->tbdf, v) == -1)
			return -1;
	}
	return vec;
}

/*
 *	I have a function pointer in PCArch for every one of these, because on
 *	some Alphas we have to use sparse mode, but on others we can use
 *	MOVB et al.  Additionally, the PC164 documentation threatened us
 *	with the lie that the SIO is in region B, but everything else in region A.
 *	This turned out not to be the case.  Given the cost of this solution, it
 *	may be better just to use sparse mode for I/O space on all platforms.
 */
int
inb2117x(int port)
{
	mb();
	return *(uchar*)(iobase(port));
}

ushort
ins2117x(int port)
{
	mb();
	return *(ushort*)(iobase(port));
}

ulong
inl2117x(int port)
{
	mb();
	return *(ulong*)(iobase(port));
}

void
outb2117x(int port, int val)
{
	mb();
	*(uchar*)(iobase(port)) = val;
	mb();
}

void
outs2117x(int port, ushort val)
{
	mb();
	*(ushort*)(iobase(port)) = val;
	mb();
}

void
outl2117x(int port, ulong val)
{
	mb();
	*(ulong*)(iobase(port)) = val;
	mb();
}

void
insb2117x(int port, void *buf, int len)
{
	int i;
	uchar *p, *q;

	p = (uchar*)iobase(port);
	q = buf;
	for(i = 0; i < len; i++){
		mb();
		*q++ = *p;
	}
}

void
inss2117x(int port, void *buf, int len)
{
	int i;
	ushort *p, *q;

	p = (ushort*)iobase(port);
	q = buf;
	for(i = 0; i < len; i++){
		mb();
		*q++ = *p;
	}
}

void
insl2117x(int port, void *buf, int len)
{
	int i;
	ulong *p, *q;

	p = (ulong*)iobase(port);
	q = buf;
	for(i = 0; i < len; i++){
		mb();
		*q++ = *p;
	}
}

void
outsb2117x(int port, void *buf, int len)
{
	int i;
	uchar *p, *q;

	p = (uchar*)iobase(port);
	q = buf;
	for(i = 0; i < len; i++){
		mb();
		*p = *q++;
	}
}

void
outss2117x(int port, void *buf, int len)
{
	int i;
	ushort *p, *q;

	p = (ushort*)iobase(port);
	q = buf;
	for(i = 0; i < len; i++){
		mb();
		*p = *q++;
	}
}

void
outsl2117x(int port, void *buf, int len)
{
	int i;
	ulong *p, *q;

	p = (ulong*)iobase(port);
	q = buf;
	for(i = 0; i < len; i++){
		mb();
		*p = *q++;
	}
}

PCArch arch164 = {
	"EB164",
	ident,
	coreinit,
	corehello,
	coredetach,
	pcicfg2117x,
	pcimem2117x,
	intrenable164,
	nil,
	nil,

	inb2117x,
	ins2117x,
	inl2117x,
	outb2117x,
	outs2117x,
	outl2117x,
	insb2117x,
	inss2117x,
	insl2117x,
	outsb2117x,
	outss2117x,
	outsl2117x,
};
