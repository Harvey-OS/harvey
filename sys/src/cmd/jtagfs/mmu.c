#include <u.h>
#include <libc.h>
#include <bio.h>
#include "chain.h"
#include "debug.h"
#include "tap.h"
#include "lebo.h"
#include "bebo.h"
#include "jtag.h"
#include "icert.h"
#include "mmu.h"
#include "mpsse.h"
#include "/sys/src/9/kw/arm.h"

/* 
 *	Feroceon's scan chain 15 does not work, I suspect it has the multi-ice
 *	40 bits limit (even if it is only one core). Just pushing MCR/MRC and
 *	reading back seems to be more portable
 *	The only bad thing is that using chain 15 one can
 *	change things secondary effects (I cannot think of any, but there may be)
 */

char *
printmmuregs(MMURegs *mmuregs, char *s, int ssz)
{
	char *e, *te;

	te = s + ssz - 1;
	
	e = seprint(s, te, "cpid: %#8.8ux\n", mmuregs->cpid);
	e = seprint(e, te, "ct: %#8.8ux\n", mmuregs->ct);
	e = seprint(e, te, "control: %#8.8ux\n", mmuregs->control);
	e = seprint(e, te, "ttb: %#8.8ux\n", mmuregs->ttb);
	e = seprint(e, te, "dac: %#8.8ux\n", mmuregs->dac);
	e = seprint(e, te, "fsr: %#8.8ux\n", mmuregs->fsr);
	e = seprint(e, te, "far: %#8.8ux\n", mmuregs->far);
	e = seprint(e, te, "pid: %#8.8ux\n", mmuregs->pid);
	return e;
}

/* Chain 15 does not work properly and is not portable, use MCR/MRC */

static int
jtagmrc(JMedium *jmed, u32int *data, uchar op1, uchar op2, uchar crn, uchar crm)
{
	int res;
	u32int d;

	res = armfastexec(jmed, ARMMRC(CpSC, op1, 0, crn, crm, op2));
	if(res < 0)
		return -1;

	setinst(jmed, InRestart, 0);
	res = icewaitdebug(jmed);
	if(res < 0)
		return -1;
	res = setchain(jmed, ChCommit, 1);
	if(res < 0)
		sysfatal("setchain %r");
	armgetexec(jmed, 1, &d, ARMSTMIA|0x0001);

	*data = d;
	dprint(Dmmu, "MCR data %#8.8ux \n", d);
	return 0;
}

static int
jtagmcr(JMedium *jmed, u32int data, uchar op1, uchar op2, uchar crn, uchar crm)
{
	int res;
	dprint(Dmem, "MCR data %#8.8ux\n",
				data);
	/* load data in r0 */
	res = armsetexec(jmed, 1, &data, ARMLDMIA|0x0001);
	if(res < 0)
		return -1;

	res = armfastexec(jmed, ARMMCR(CpSC, op1, 0, crn, crm, op2));
	if(res < 0)
		return -1;

	setinst(jmed, InRestart, 0);
	res = icewaitdebug(jmed);
	if(res < 0)
		return -1;
	return 0;
}


int
mmurdregs(JMedium *jmed, MMURegs *regs)
{
	int res;
	res = setchain(jmed, ChCommit, 1);
	if(res < 0)
		sysfatal("setchain %r");
	res = jtagmrc(jmed, &regs->cpid, 0, CpIDid, C(CpID), C(0));
	if(res < 0)
		return -1;
	res = jtagmrc(jmed, &regs->control, 0, 0,  C(CpCONTROL), C(0));
	if(res < 0)
		return -1;
	res = jtagmrc(jmed, &regs->ttb, 0, 0,  C(CpTTB), C(0));
	if(res < 0)
		return -1;
	res = jtagmrc(jmed, &regs->dac, 0, 0,  C(CpDAC), C(0));
	if(res < 0)
		return -1;
	res = jtagmrc(jmed, &regs->fsr, 0, 0,  C(CpFSR), C(0));
	if(res < 0)
		return -1;
	res = jtagmrc(jmed, &regs->far, 0, 0,  C(CpFAR), C(0));
	if(res < 0)
		return -1;
	res = jtagmrc(jmed, &regs->ct, 0, CpIDct, C(CpID), C(0));
	if(res < 0)
		return -1;
	res = jtagmrc(jmed, &regs->pid, 0, 0,  C(CpPID), C(0));
	if(res < 0)
		return -1;

	return 0;
}


