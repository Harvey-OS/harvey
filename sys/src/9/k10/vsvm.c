/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
typedef uint64_t Sd;
typedef uint16_t Ss;
typedef struct Tss Tss;

struct Gd {
	Sd	sd;
	uint64_t	hi;
};

struct Tss {
	uint32_t	_0_;
	uint32_t	rsp0[2];
	uint32_t	rsp1[2];
	uint32_t	rsp2[2];
	uint32_t	_28_[2];
	uint32_t	ist[14];
	uint16_t	_92_[5];
	uint16_t	iomap;
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
//static int ngdt64 = 10;

static Gd idt64[Nidt];
//static Gd acidt64[Nidt];	/* NIX application core IDT */

static Sd
mksd(uint64_t base, uint64_t limit, uint64_t bits, uint64_t* upper)
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
mkgd(Gd* gd, uint64_t offset, Ss ss, uint64_t bits, int ist)
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
idtinit(Gd *gd, uintptr_t offset)
{
	int ist, v;
	uint64_t dpl;

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
tssrsp0(uintptr_t sp)
{
	Tss *tss;

	tss = m->tss;
	tss->rsp0[0] = sp;
	tss->rsp0[1] = sp>>32;
}

static void
tssinit(uintptr_t sp)
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

void acsyscallentry(void)
{
	panic("acsyscallentry");
}

void
vsvminit(int size, int nixtype)
{
	Sd *sd;
	uint64_t r;
	if(m->machno == 0){
		idtinit(idt64, PTR2UINT(idthandlers));
		//idtinit(acidt64, PTR2UINT(acidthandlers));
	}
hi("set m->gdt\n");
	m->gdt = m->vsvm;
	memmove(m->gdt, gdt64, sizeof(gdt64));
	m->tss = &m->vsvm[ROUNDUP(sizeof(gdt64), 16)];

	sd = &((Sd*)m->gdt)[SiTSS];
	*sd = mksd(PTR2UINT(m->tss), sizeof(Tss)-1, SdP|SdDPL0|SdaTSS, sd+1);
hi("tssinti\n");
	tssinit(m->stack+size);
hi("call gdtput...");
hi("SKIPPING gdtput for now until we figure out what we want. \n");
	//gdtput(sizeof(gdt64)-1, PTR2UINT(m->gdt), SSEL(SiCS, SsTIGDT|SsRPL0));
hi("gdtput\n");
#if 0 // NO ACs YET
	if(nixtype != NIXAC)
#endif
		idtput(sizeof(idt64)-1, PTR2UINT(idt64));
#if 0
	else
		idtput(sizeof(acidt64)-1, PTR2UINT(acidt64));
#endif
	trput(SSEL(SiTSS, SsTIGDT|SsRPL0));

hi("idtput done\n");
hi("write fsbase, then gsbase, then etc.\n");
	wrmsr(FSbase, 0ull);
	wrmsr(GSbase, PTR2UINT(&sys->machptr[m->machno]));
	wrmsr(KernelGSbase, 0ull);
hi("done that\n");

	r = rdmsr(Efer);
	r |= Sce;
	wrmsr(Efer, r);
	r = ((uint64_t)SSEL(SiU32CS, SsRPL3))<<48;
	r |= ((uint64_t)SSEL(SiCS, SsRPL0))<<32;
hi("wirte Star!\n");
	wrmsr(Star, r);
hi("wirte Lstar!\n");
	if(nixtype != NIXAC)
		wrmsr(Lstar, PTR2UINT(syscallentry));
	else
		wrmsr(Lstar, PTR2UINT(acsyscallentry));
hi("wirte Sfmask!\n");
	wrmsr(Sfmask, If);
hi("vsvminit done!\n");
}

int
userureg(Ureg* ureg)
{
	return ureg->cs == SSEL(SiUCS, SsRPL3);
}
