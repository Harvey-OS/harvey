/*
 * Vestigial Segmented Virtual Memory.
 * To do:
 *	dynamic allocation and free of descriptors;
 *	IST should perhaps point to a different handler;
 *	user-level descriptors (if not dynamic).
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "amd64.h"
#include "ureg.h"

typedef struct Gd Gd;
typedef u64int Sd;
typedef u16int Ss;
typedef struct Tss Tss;

struct Gd {
	Sd	sd;
	u64int	hi;
};

struct Tss {
	u32int	_0_;
	u32int	rsp0[2];
	u32int	rsp1[2];
	u32int	rsp2[2];
	u32int	_28_[2];
	u32int	ist[14];
	u16int	_92_[5];
	u16int	iomap;
};

enum {
	Ngdt		= 16,			/* max. entries in gdt */
	Nidt		= 256,			/* max. entries in idt */
};

static Sd gdt64[Ngdt] = {
	0ull,					/* NULL descriptor */
	SdL|SdP|SdDPL0|SdS|SdCODE,		/* CS */
	SdG|SdD|SdP|SdDPL0|SdS|SdW,		/* DS */
	SdG|SdD|SdP|SdDPL3|SdS|SdCODE|SdR|Sd4G,	/* User CS 32-bit */
	SdG|SdD|SdP|SdDPL3|SdS|SdW|Sd4G,	/* User DS */
	SdL|SdP|SdDPL3|SdS|SdCODE,		/* User CS 64-bit */

	0ull,					/* FS */
	0ull,					/* GS */

	0ull,					/* TSS lower */
	0ull,					/* TSS upper */
};
static int ngdt64 = 10;

static Gd idt64[Nidt];

static Sd
mksd(u64int base, u64int limit, u64int bits, u64int* upper)
{
	Sd sd;

	sd = bits;
	sd |= (((limit & 0x00000000000f0000ull)>>16)<<48)
	     |(limit & 0x000000000000ffffull);
	sd |= (((base & 0x00000000ff000000ull)>>24)<<56)
	     |(((base & 0x0000000000ff0000ull)>>16)<<32)
	     |((base & 0x000000000000ffffull)<<16);
	if(upper != nil)
		*upper = base>>32;

	return sd;
}

static void
mkgd(Gd* gd, u64int offset, Ss ss, u64int bits, int ist)
{
	Sd sd;

	sd = bits;
	sd |= (((offset & 0x00000000ffff0000ull)>>16)<<48)
	     |(offset & 0x000000000000ffffull);
	sd |= ((ss & 0x000000000000ffffull)<<16);
	sd |= (ist & (SdISTM>>32))<<32;
	gd->sd = sd;
	gd->hi = offset>>32;
}

static void
idtinit(void)
{
	Gd *gd;
	int ist, v;
	u64int dpl;
	uintptr offset;

	gd = idt64;
	offset = PTR2UINT(idthandlers);

	for(v = 0; v < Nidt; v++){
		ist = 0;
		dpl = SdP|SdDPL0|SdIG;
		switch(v){
		default:
			break;
		case IdtBP:			/* #BP */
			dpl = SdP|SdDPL3|SdIG;
			break;
		case IdtUD:			/* #UD */
		case IdtDF:			/* #DF */
			ist = 1;
			break;
		}
		mkgd(gd, offset, SSEL(SiCS, SsTIGDT|SsRPL0), dpl, ist);
		gd++;
		offset += 6;
	}
}

void
tssrsp0(uintptr sp)
{
	Tss *tss;

	tss = m->tss;
	tss->rsp0[0] = sp;
	tss->rsp0[1] = sp>>32;
}

static void
tssinit(uintptr sp)
{
	int ist;
	Tss *tss;

	tss = m->tss;
	memset(tss, 0, sizeof(Tss));

	tssrsp0(sp);

	sp = PTR2UINT(m->vsvm+PGSZ);
	for(ist = 0; ist < 14; ist += 2){
		tss->ist[ist] = sp;
		tss->ist[ist+1] = sp>>32;
	}
	tss->iomap = 0xdfff;
}

void
vsvminit(int size)
{
	Sd *sd;
	u64int r;

	if(m->machno == 0)
		idtinit();

	m->gdt = m->vsvm;
	memmove(m->gdt, gdt64, sizeof(gdt64));
	m->tss = &m->vsvm[ROUNDUP(sizeof(gdt64), 16)];

	sd = &((Sd*)m->gdt)[SiTSS];
	*sd = mksd(PTR2UINT(m->tss), sizeof(Tss)-1, SdP|SdDPL0|SdaTSS, sd+1);

	tssinit(m->stack+size);

	gdtput(sizeof(gdt64)-1, PTR2UINT(m->gdt), SSEL(SiCS, SsTIGDT|SsRPL0));
	idtput(sizeof(idt64)-1, PTR2UINT(idt64));
	trput(SSEL(SiTSS, SsTIGDT|SsRPL0));

	wrmsr(FSbase, 0ull);
	wrmsr(GSbase, PTR2UINT(&sys->machptr[m->machno]));
	wrmsr(KernelGSbase, 0ull);

	r = rdmsr(Efer);
	r |= Sce;
	wrmsr(Efer, r);
	r = ((u64int)SSEL(SiU32CS, SsRPL3))<<48;
	r |= ((u64int)SSEL(SiCS, SsRPL0))<<32;
	wrmsr(Star, r);
	wrmsr(Lstar, PTR2UINT(syscallentry));
	wrmsr(Sfmask, If);
}

int
userureg(Ureg* ureg)
{
	return ureg->cs == SSEL(SiUCS, SsRPL3);
}
