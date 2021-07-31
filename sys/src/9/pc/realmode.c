#include	"u.h"
#include	"tos.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"

/*
 * Back the processor into real mode to run a BIOS call,
 * then return.  This must be used carefully, since it 
 * completely disables hardware interrupts (e.g., the i8259)
 * while running.  It is *not* using VM86 mode. 
 * Maybe that's really the right answer, but real mode
 * is fine for now.  We don't expect to use this very much --
 * just for VGA and APM.
 */
#define realmoderegs (*(Ureg*)RMUADDR)

#define LORMBUF (RMBUF-KZERO)

static Ureg rmu;
static Lock rmlock;

void
realmode(Ureg *ureg)
{
	int s;
	ulong cr3;
	extern void realmode0(void);	/* in l.s */
	extern void i8259off(void), i8259on(void);

	if(getconf("*norealmode"))
		return;

	lock(&rmlock);
	realmoderegs = *ureg;

	/* copy l.s so that it can be run from 16-bit mode */
	memmove((void*)RMCODE, (void*)KTZERO, 0x1000);

	s = splhi();
	m->pdb[PDX(0)] = m->pdb[PDX(KZERO)];	/* identity map low */
	cr3 = getcr3();
	putcr3(PADDR(m->pdb));
	i8259off();
	realmode0();
	if(m->tss){
		/*
		 * Called from memory.c before initialization of mmu.
		 * Don't turn interrupts on before the kernel is ready!
		 */
		i8259on();
	}
	m->pdb[PDX(0)] = 0;	/* remove low mapping */
	putcr3(cr3);
	splx(s);
	*ureg = realmoderegs;
	unlock(&rmlock);
}

static long
rtrapread(Chan*, void *a, long n, vlong off)
{
	if(off < 0)
		error("badarg");
	if(n+off > sizeof rmu)
		n = sizeof rmu - off;
	if(n <= 0)
		return 0;
	memmove(a, (char*)&rmu+off, n);
	return n;
}

static long
rtrapwrite(Chan*, void *a, long n, vlong off)
{
	if(off || n != sizeof rmu)
		error("write a Ureg");
	memmove(&rmu, a, sizeof rmu);
	/*
	 * Sanity check
	 */
	if(rmu.trap == 0x10){	/* VBE */
		rmu.es = (LORMBUF>>4)&0xF000;
		rmu.di = LORMBUF&0xFFFF;
	}else
		error("invalid trap arguments");
	realmode(&rmu);
	return n;
}

static long
rmemrw(int isr, void *a, long n, vlong off)
{
	if(off >= 1024*1024 || off+n >= 1024*1024)
		return 0;
	if(off < 0 || n < 0)
		error("bad offset/count");
	if(isr)
		memmove(a, KADDR((ulong)off), n);
	else{
		/* writes are more restricted */
		if(LORMBUF <= off && off < LORMBUF+BY2PG
		&& off+n <= LORMBUF+BY2PG)
			{}
		else
			error("bad offset/count in write");
		memmove(KADDR((ulong)off), a, n);
	}
	return n;
}

static long
rmemread(Chan*, void *a, long n, vlong off)
{
	return rmemrw(1, a, n, off);
}

static long
rmemwrite(Chan*, void *a, long n, vlong off)
{
	return rmemrw(0, a, n, off);
}

void
realmodelink(void)
{
	addarchfile("realmode", 0660, rtrapread, rtrapwrite);
	addarchfile("realmodemem", 0660, rmemread, rmemwrite);
}

