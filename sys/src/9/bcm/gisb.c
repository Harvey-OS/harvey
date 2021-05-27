/*
 * from 9front
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "ureg.h"

/*
 * GISB arbiter registers
 */
static u32int *regs = (u32int*)(VIRTIO + 0x400000);

enum {
	ArbMasterMask	= 0x004/4,
	ArbTimer	= 0x008/4,
		TimerFreq	= 216000000,	// 216MHz

	ArbErrCapClear	= 0x7e4/4,
	ArbErrCapAddrHi	= 0x7e8/4,
	ArbErrCapAddr	= 0x7ec/4,
	ArbErrCapData	= 0x7f0/4,
	ArbErrCapStatus	= 0x7f4/4,
		CapStatusTimeout	= 1<<12,
		CapStatusAbort		= 1<<11,
		CapStatusStrobe		= 15<<2,
		CapStatusWrite		= 1<<1,
		CapStatusValid		= 1<<0,
	ArbErrCapMaster	= 0x7f8/4,

	ArbIntrSts	= 0x3000/4,
	ArbIntrSet	= 0x3004/4,
	ArbIntrClr	= 0x3008/4,

	ArbCpuMaskSet	= 0x3010/4,
};

static int
arberror(Ureg*)
{
	u32int status, intr;
	u32int master, data;
	uvlong addr;

	status = regs[ArbErrCapStatus];
	if((status & CapStatusValid) == 0)
		return 0;
	intr = regs[ArbIntrSts];
	master = regs[ArbErrCapMaster];
	addr = regs[ArbErrCapAddr];
	addr |= (uvlong)regs[ArbErrCapAddrHi]<<32;
	data = regs[ArbErrCapData];
	if(intr)
		regs[ArbIntrClr] = intr;
	regs[ArbErrCapClear] = CapStatusValid;

	iprint("cpu%d: GISB arbiter error: %s%s %s bus addr %llux data %.8ux, "
		"master %.8ux, status %.8ux, intr %.8ux\n",
		m->machno,
		(status & CapStatusTimeout) ? "timeout" : "",
		(status & CapStatusAbort) ? "abort" : "",
		(status & CapStatusWrite) ? "writing" : "reading",
		addr, data,
		master, status, intr);

	return 1;
}

static void
arbclock(void)
{
	arberror(nil);
}

void
gisblink(void)
{
	extern int (*buserror)(Ureg*);	// trap.c

	regs[ArbErrCapClear] = CapStatusValid;
	regs[ArbIntrClr] = -1;

	addclock0link(arbclock, 100);

	//buserror = arberror;
}
