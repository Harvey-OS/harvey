/*
 *	``Nevermore!''
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

typedef struct Raven Raven;
struct Raven
{
	ushort	vid;
	ushort	did;
	uchar	_pad4;
	uchar	rev;
	uchar	_pad6[2];
	ushort	gcsr;
	ushort	feat;
	uchar	_padC[7];
	uchar	padj;
	uchar	_pad14[12];
	ushort	errtst;
	ushort	erren;
	uchar	_pad24[3];
	uchar	errst;
	ulong	errad;
	uchar	_pad2C[2];
	ushort	errat;
	ulong	piack;
	uchar	_pad34[12];

	struct {
		ushort	start;
		ushort	end;
		ushort	off;
		uchar	_pad;
		uchar	attr;
	} map[4];

	struct {
		ulong	cntl;
		uchar	_pad[3];
		uchar	stat;
	} wdt[2];

	ulong	gpr[4];
};

enum {
	/* map[] attr bits */
	Iom = (1<<0),
	Mem = (1<<1),
	Wpen = (1<<4),
	Wen = (1<<6),
	Ren = (1<<7),
};

static Raven *raven = (Raven*)RAVEN;
static ulong mpic;

static void
setmap(int i, ulong addr, ulong len, ulong busaddr, int attr)
{
	raven->map[i].start = addr>>16;
	raven->map[i].end = (addr+len-1)>>16;
	raven->map[i].off = (busaddr-addr)>>16;
	raven->map[i].attr = attr;
}

static ulong
swap32(ulong x)
{
	return (x>>24)|((x>>8)&0xff00)|((x<<8)&0xff0000)|(x<<24);
}

static ulong
mpic32r(int rno)
{
	return swap32(*(ulong*)(mpic+rno));
}

static void
mpic32w(int rno, ulong x)
{
	*(ulong*)(mpic+rno) = swap32(x);
	eieio();
}

void
raveninit(void)
{
	int i;
	Pcidev *p;

	if(raven->vid != 0x1057 || raven->did !=0x4801)
		panic("raven not found");

	/* set up a sensible hardware memory/IO map */
	setmap(0, PCIMEM0, PCISIZE0, 0, Wen|Ren|Mem);
	setmap(1, KZERO, IOSIZE, 0, Wen|Ren);	/* keeps PPCbug happy */
	setmap(2, PCIMEM1, PCISIZE1, PCISIZE0, Wen|Ren|Mem);
	setmap(3, IOMEM, IOSIZE, 0, Wen|Ren);	/* I/O must be slot 3 for PCI cfg space */

	p = pcimatch(nil, 0x1057, 0x4801);
	if(p == nil)
		panic("raven PCI regs not found");
	mpic = (p->mem[1].bar+PCIMEM0);

	/* ensure all interrupts are off, and routed to cpu 0 */
	for(i = 0; i < 16; i++) {
		mpic32w(0x10000+0x20*i, (1<<31));	/* mask */
		mpic32w(0x10010+0x20*i, 1);			/* route to cpu 0 */
	}

	mpic32w(0x20080, 1);			/* cpu 0 task pri */
//	mpic32w(0x21080, 1);			/* cpu 1 task pri */
	mpic32w(0x1020, (1<<29));		/* Mixed mode (8259 & Raven intrs both available) */
}

void
mpicenable(int vec, Vctl *v)
{
	ulong x;

	x = (1<<22)|(15<<16)|vec;
	if(vec == 0)
		x |= (1<<23);
	mpic32w(0x10000+0x20*vec, x);
	if(vec != 0)
		v->eoi = mpiceoi;
}

void
mpicdisable(int vec)
{
	mpic32w(0x10000+0x20*vec, (1<<31));
}

int
mpicintack(void)
{
	return mpic32r(0x200A0 + (m->machno<<12));
}

int
mpiceoi(int vec)
{
	USED(vec);
	mpic32w(0x200B0 + (m->machno<<12), 0);
	return 0;
}
