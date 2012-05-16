#include	"u.h"
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
 * just for BIOS INT 13 disk i/o.
 */

void realmode0(void);		/* in realmode0.s */
void realmodeintrinst(void);	/* in realmode0.s */
void realmodeend(void);		/* in realmode0.s */

extern ushort rmseg;		/* in realmode0.s */

static Ureg rmu;
static QLock rmlock;
static int beenhere;

void
realmode(Ureg *ureg)
{
	int s, sz;
	ulong cr3;
	uchar *ip;

	qlock(&rmlock);
	if (!beenhere)
		iprint("into bios in real mode...");
	*(Ureg *)RMUADDR = *ureg;

	/*
	 * in pxe-loaded bootstraps, the l.s real-mode code is already
	 * below 64K, but for pbs-loaded bootstraps, we need to copy it there.
	 */
	ip = (void *)realmodeintrinst;		/* the INT instruction */
	ip[1] = ureg->trap;			/* insert INT number */
	coherence();
	if ((uintptr)KTZERO == KZERO+PXEBASE)	/* pxe-loaded? */
		rmseg = 0;			/* into JMPFAR instr. */
	else {
		/* copy l.s so that it can be run from 16-bit mode */
		sz = (char *)realmodeend - (char *)KTZERO;
		if (sz > RMSIZE)
			panic("real mode code %d bytes > %d", sz, RMSIZE);
		rmseg = (RMCODE - KZERO) >> 4;	/* into JMPFAR instr. */
		memmove((void*)RMCODE, (void*)KTZERO, sz);
	}
	coherence();

	s = splhi();
	m->pdb[PDX(0)] = m->pdb[PDX(KZERO)];	/* identity map low */
	cr3 = getcr3();
	putcr3(PADDR(m->pdb));
	if (arch)
		arch->introff();
	else
		i8259off();

	realmode0();
	splhi();				/* who knows what the bios did */

	if(m->tss){
		/*
		 * Called from memory.c before initialization of mmu.
		 * Don't turn interrupts on before the kernel is ready!
		 */
		if (arch)
			arch->intron();
		else
			i8259on();
	}
	m->pdb[PDX(0)] = 0;	/* remove low mapping */
	putcr3(cr3);
	splx(s);

	*ureg = *(Ureg *)RMUADDR;
	if (!beenhere) {
		beenhere = 1;
		iprint("and back\n");
	}
	qunlock(&rmlock);
}
