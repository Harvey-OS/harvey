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
	ulong	ea;			/* effective address */
	ushort	ssw;			/* special status word */
	ushort	wb3s;			/* write back 3 status */
	ushort	wb2s;			/* write back 2 status */
	ushort	wb1s;			/* write back 1 status */
	ulong	fa;			/* fault address */
	ulong	wb3a;			/* write back 3 address */
	ulong	wb3d;			/* write back 3 data */
	ulong	wb2a;			/* write back 2 address */
	ulong	wb2d;			/* write back 2 data */
	ulong	wb1a;			/* write back 1 address */
	ulong	wb1d;			/* write back 3 data */
	ulong	pd1;			/* push data lw1 */
	ulong	pd2;			/* push data lw2 */
	ulong	pd3;			/* push data lw3 */
};

static	uchar	tsz[4] = { 4, 1, 2, 0 };

/*
 * SSW bits
 */
#define	READ	0x0100
#define	TM	0x0007

/*
 * Writeback bits
 */
#define	WBVALID	(1<<7)
#define	WBSIZE	(3<<5)
#define	WBSZSH	5
#define	WBTT	(3<<3)
#define	WBTM	(7<<0)

int	wbfault(ulong, int, ushort, int, ulong);
ulong	normalize(ulong, ulong, ulong);

extern struct{
	ulong	addr;
	ulong	baddr;
	Lock;
}kmapalloc;

void
fault68040(Ureg *ur, FFrame *f)
{
	ulong addr, badvaddr;
	int user, read, insyscall;
	char buf[ERRLEN];

	if(u == 0){
		dumpregs(ur);
		print("ssw=%ux ea=%lux fa=%lux\n", f->ssw, f->ea, f->fa);
		panic("fault u==0 pc=%lux fa=%lux ssw=%lux", ur->pc, f->fa, f->ssw);
	}

	insyscall = u->p->insyscall;
	u->p->insyscall = 1;
	addr = f->fa;
	addr &= VAMASK;
	badvaddr = addr;
	addr &= ~(BY2PG-1);
	read = 0;
	if(f->ssw & READ)
		read = 1;
	user = !(ur->sr&SUPER);
	if(user)
		u->dbgreg = ur;
	if((f->ssw&TM) == 0)
		panic("cache push pc %lux addr %lux ssw %lux\n", ur->pc, f->fa, f->ssw);
/* print("fault pc=%lux addr=%lux read %d\n", ur->pc, badvaddr, read); /**/

	/*
	 *  original fault plus 3 writeback buffers
	 */
	if(fault(addr, read) < 0
	|| wbfault(f->wb1a, user, f->wb1s, 1, normalize(f->wb1s, f->wb1a, f->wb1d)) < 0
	|| wbfault(f->wb2a, user, f->wb2s, 2, f->wb2d) < 0
	|| wbfault(f->wb3a, user, f->wb3s, 3, f->wb3d) < 0){
		if(user){
			sprint(buf, "sys: trap: fault %s addr=0x%lux",
				read? "read" : "write", badvaddr);
			postnote(u->p, 1, buf, NDebug);
			notify(ur);
			return;
		}
		dumpregs(ur);
print("wb1a %lux wb1s %lux wb1d %lux\n", f->wb1a, f->wb1s, f->wb1d);
print("wb2a %lux wb2s %lux wb2d %lux\n", f->wb2a, f->wb2s, f->wb2d);
print("wb3a %lux wb3s %lux wb3d %lux\n", f->wb3a, f->wb3s, f->wb3d);
		panic("fault: 0x%lux 0x%lux", badvaddr, addr);
	}
	u->p->insyscall = insyscall;
	if(user)
		notify(ur);
}

/*
 *  get relevant data into the right hand side of the long
 */
ulong
normalize(ulong stat, ulong addr, ulong val)
{
	int thecase, size;

	size = (stat&WBSIZE) >> WBSZSH;
	thecase = (size<<2) | (addr&3);
	switch(thecase){
		/* long write */
	case 0:		return val;
	case 1:		return (val<<8) | ((val>>24)&0xff);
	case 2:		return (val<<16) | ((val>>16)&0xffff);
	case 3:		return (val<<24) | ((val>>8)&0xffffff);
		/* byte write */
	case 4:		return val>>24;
	case 5:		return val>>16;
	case 6:		return val>>8;
	case 7:		return val;
		/* short write */
	case 8:		return val>>16;
	case 9:		return val>>8;
	case 10:	return val;
	case 11:	return (val<<8) | ((val>>24)&0xff);
	}
}

/*
 * Check fault status before doing writeback
 */
int
wbfault(ulong fa, int user, ushort wbs, int wbn, ulong d)
{
	int s;
	ulong addr;

	if(!(wbs & WBVALID))
		return 0;
	if(wbs & WBTT)
		panic("writeback%d status %ux", wbn, wbs);
	addr = fa & ~(BY2PG-1);
	s = tsz[(wbs&WBSIZE) >> WBSZSH];
	if(user || !kernaddr(addr)){
		if(fault(fa, 0) < 0){
print("wbfault: %lux, 0\n", fa);
			return -1;
		}
		if(addr != ((fa+s-1)&~(BY2PG-1)))
			if(fault(fa+s-1, 0) < 0){
print("wbfault: %lux, %lux\n", fa, fa+s-1);
				return -1;
			}
	}

	switch(s){
	case 4:
		*(ulong*)fa = d;
		break;
	case 1:
		*(uchar*)fa = d;
		break;
	case 2:
		*(ushort*)fa = d;
		break;
	default:
		panic("writeback%d %d\n", wbn, s);
	}
	return 0;
}
