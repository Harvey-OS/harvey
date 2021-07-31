#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"../port/error.h"
#include	"io.h"

/*
 * Ask if the instruction at EPC could have cause this badvaddr
 *	this is a fixup for some early R4400 chips which report
 *	random BADVADDR contents.
 */

ulong*
reg(Ureg *ur, int regno)
{
	ulong *l;

	switch(regno) {
	case 31: return &ur->r31;
	case 30: return &ur->r30;
	case 29: return &ur->sp;
	default:
		l = &ur->r1;
		return &l[regno-1];
	}
}

int
tstbadvaddr(Ureg *ur)
{
	int rn;
	ulong iw, off, ea;

	iw = ur->pc;
	if(ur->cause & (1<<31))
		iw += 4;

	if(seg(u->p, iw, 0) == 0)
		return 0;

	iw = *(ulong*)iw;

/*	print("iw: %lux\n", iw);	/**/

	switch((iw>>26) & 0x3f) {
	default:
		return 1;
	case 0x20:	/* LB */
	case 0x24:	/* LBU */
			/* LD */
	case 0x35:
	case 0x36:
	case 0x37:	/* LDCz */
	case 0x1A:	/* LDL */
	case 0x1B:	/* LDR */
	case 0x21:	/* LH */
	case 0x25:	/* LHU */
	case 0x30:	/* LL */
	case 0x34:	/* LLD */
	case 0x23:	/* LW */
	case 0x31:
	case 0x32:	/* LWCz possible 0x33 */
	case 0x27:	/* LWU */
	case 0x22:	/* LWL */
	case 0x26:	/* LWR */
		break;

	case 0x28:	/* SB */
	case 0x38:	/* SC */
	case 0x3C:	/* SCD */
	case 0x3D:
	case 0x3E:
	case 0x3F:	/* SDCz */
	case 0x2C:	/* SDL */
	case 0x2D:	/* SDR */
	case 0x29:	/* SH */
	case 0x2B:	/* SW */
	case 0x39:
	case 0x3A:	/* SWCz */
	case 0x2A:	/* SWL */
	case 0x2E:	/* SWR */
		break;
	}

	off = iw & 0xffff;
	if(off & 0x8000)
		off |= ~0xffff;

	rn = (iw>>21) & 0x1f;
	ea = *reg(ur, rn);
	if(rn == 0)
		ea = 0;
	ea += off;

/*	print("ea %lux 0x%lux(R%d) bv 0x%lux pc 0x%lux\n", ea, off, rn, ur->badvaddr, ur->pc); /**/

	if(ur->badvaddr == ea)
		return 0;

	return 1;
}

/*
 *  find out fault address and type of access.
 *  Call common fault handler.
 */
void
faultmips(Ureg *ur, int user, int code)
{
	int read;
	ulong addr;
	extern char *excname[];
	char *p, buf[ERRLEN];

	addr = ur->badvaddr;

	addr &= ~(BY2PG-1);
	read = !(code==CTLBM || code==CTLBS);

/*	print("fault: %s code %d va %lux pc %lux r31 %lux %lux\n",
		u->p->text, code, ur->badvaddr, ur->pc, ur->r31, tlbvirt()); /**/

	if(fault(addr, read) == 0)
		return;

	if(tstbadvaddr(ur))
		return;

	if(user) {
		p = "store";
		if(read)
			p = "load";
		sprint(buf, "sys: trap: fault %s addr=0x%lux r31=0x%lux",
					p, ur->badvaddr, ur->r31);
		postnote(u->p, 1, buf, NDebug);
		return;
	}

	print("kernel %s VADDR=0x%lux\n", excname[code], ur->badvaddr);
	print("IW at pc %.8lux\n", *((ulong*)ur->pc));
	dumpregs(ur);
	panic("fault");
}

/*
 * called in sysfile.c
 */
void
evenaddr(ulong addr)
{
	if(addr & 3){
		postnote(u->p, 1, "sys: odd address", NDebug);
		error(Ebadarg);
	}
}
