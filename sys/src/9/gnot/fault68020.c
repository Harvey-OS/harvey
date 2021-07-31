#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"ureg.h"
#include	"../port/error.h"

#define	FORMAT(ur)	((((ur)->vo)>>12)&0xF)
#define	OFFSET(ur)	(((ur)->vo)&0xFFF)


struct FFrame
{
	ushort	ireg0;			/* internal register */
	ushort	ssw;			/* special status word */
	ushort	ipsc;			/* instr. pipe stage c */
	ushort	ipsb;			/* instr. pipe stage b */
	ulong	addr;			/* data cycle fault address */
	ushort	ireg1;			/* internal register */
	ushort	ireg2;			/* internal register */
	ulong	dob;			/* data output buffer */
	ushort	ireg3[4];		/* more stuff */
	ulong	baddr;			/* stage b address */
	ushort	ireg4[26];		/* more more stuff */
};

/*
 * SSW bits
 */
#define	RW	0x0040		/* read/write for data cycle */
#define	FC	0x8000		/* fault on stage C of instruction pipe */
#define	FB	0x4000		/* fault on stage B of instruction pipe */
#define	RC	0x2000		/* rerun flag for stage C of instruction pipe */
#define	RB	0x1000		/* rerun flag for stage B of instruction pipe */
#define	DF	0x0100		/* fault/rerun flag for data cycle */
#define	RM	0x0080		/* read-modify-write on data cycle */
#define	READ	0x0040
#define	WRITE	0x0000
#define	SIZ	0x0030		/* size code for data cycle */
#define	FC2	0x0004		/* address space for data cycle */
#define	FC1	0x0002
#define	FC0	0x0001

void
fault68020(Ureg *ur, FFrame *f)
{
	ulong addr, badvaddr;
	int user, read, insyscall;
	char buf[ERRLEN];

	if(u == 0){
		dumpregs(ur);
		panic("fault u==0 pc=%lux", ur->pc);
	}
	insyscall = u->p->insyscall;
	u->p->insyscall = 1;
	addr = 0;	/* set */
	if(f->ssw & DF)
		addr = f->addr;
	else if(FORMAT(ur) == 0xA){
		if(f->ssw & FC)
			addr = ur->pc+2;
		else if(f->ssw & FB)
			addr = ur->pc+4;
		else
			panic("prefetch pagefault");
	}else if(FORMAT(ur) == 0xB){
		if(f->ssw & FC)
			addr = f->baddr-2;
		else if(f->ssw & FB)
			addr = f->baddr;
		else
			panic("prefetch pagefault");
	}else
		panic("prefetch format");

	addr &= VAMASK;
	badvaddr = addr;
	addr &= ~(BY2PG-1);
	user = !(ur->sr&SUPER);
	if(user)
		u->dbgreg = ur;

	if(f->ssw & DF)
		read = (f->ssw&READ) && !(f->ssw&RM);
	else
		read = f->ssw&(FB|FC);

/* print("fault pc=%lux addr=%lux read %d\n", ur->pc, badvaddr, read); /**/

	if(fault(addr, read) < 0){
		if(user){
			sprint(buf, "sys: trap: fault %s addr=0x%lux",
				read? "read" : "write", badvaddr);
			postnote(u->p, 1, buf, NDebug);
			notify(ur);
			return;
		}
		dumpregs(ur);
		panic("fault: 0x%lux", badvaddr);
	}
	u->p->insyscall = insyscall;
}
