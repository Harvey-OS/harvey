#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"../port/error.h"
#include	"io.h"

static int iwf;

/*
 * Ask if the instruction at EPC could have cause this badvaddr
 */
int
tstbadvaddr(Ureg *ur, int read)
{
	int rn;
	ulong iw, off, ea;

	if(ur->cause & (1<<31))
		iw = ur->pc+4;
	else
		iw = ur->pc;

	if(seg(up, iw, 0) == 0)
		return 0;

	m->vaddrtst = iw;
	iw = *(ulong*)iw;
	m->vaddrtst = 0;

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
		if(!read)
			print("bogus: load causes store fault\n");
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
		if(read)
			print("bogus: store causes load fault\n");
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

	/* print("ea %lux 0x%lux(R%d) bv 0x%lux pc 0x%lux\n",
		ea, off, rn, ur->badvaddr, ur->pc); /**/

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

	/* print("fault: %s code %d va %lux pc %lux r31 %lux %lux\n",
		up->text, code, ur->badvaddr, ur->pc, ur->r31, tlbvirt());/**/

	if(fault(addr, read) == 0)
		return;

	if(tstbadvaddr(ur, read)) {
		/* print("spurious badvaddr 0x%lux in %s at pc 0x%lux\n",
				ur->badvaddr, up->text, ur->pc);/**/
		return;
	}

	if(user) {
		p = "store";
		if(read)
			p = "load";
		sprint(buf, "sys: trap: fault %s addr=0x%lux r31=0x%lux",
					p, ur->badvaddr, ur->r31);
		postnote(up, 1, buf, NDebug);
		return;
	}

	print("kernel %s vaddr=0x%lux\n", excname[code], ur->badvaddr);
	print("st=0x%lux pc=0x%lux sp=0x%lux\n", ur->status, ur->pc, ur->sp);
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
		postnote(up, 1, "sys: odd address", NDebug);
		error(Ebadarg);
	}
}
