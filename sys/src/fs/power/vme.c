#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "io.h"

Vmedevice *vmevec[256];

static void
vmedevinit(void)
{
	Vmedevice *vp;

	for (vp = vmedevtab; vp->init; vp++) {
		if (vp->vector > 0xFF || vmevec[vp->vector]) {
			print("%s: vme vector reused/too big 0x%lux\n", vp->name, vp->vector);
			continue;
		}
		if ((*vp->init)(vp)) {
			print("%s at #%lux: cannot init\n", vp->name, vp->address);
			continue;
		}
		vmevec[vp->vector] = vp;
	}
}

/*
 *  reset the vme bus
 */
void
vmereset(void)
{
	int noforce;

	if(ioid >= IO3R1)
		noforce = 1;
	else
		noforce = 0;
	MODEREG->resetforce = (1<<1) | noforce;
	delay(140);
	MODEREG->resetforce = noforce;
}

/*
 *  We have to program both the IO2 board to generate interrupts
 *  and the SBCC on CPU 0 to accept them.
 */
void
vmeinit(void)
{
	long i;
	int maxlevel;

	print("vmeinit\n");
	ioid = *IOID;
	if(ioid >= IO3R1)
		maxlevel = 8;
	else
		maxlevel = 8;

	vmereset();
	MODEREG->masterslave = (SLAVE<<4) | MASTER;

	/*
	 *  tell IO2 to sent all interrupts to CPU 0's SBCC
	 */
	for(i=0; i<maxlevel; i++)
		INTVECREG->i[i].vec = 0<<8;

	/*
	 *  Tell CPU 0's SBCC to map all interrupts from the IO2 to MIPS level 5
	 *
	 *	0x01		level 0
	 *	0x02		level 1
	 *	0x04		level 2
	 *	0x08		level 4
	 *	0x10		level 5
	 */
	SBCCREG->flevel = 0x10;

	/*
	 *  Tell CPU 0's SBCC to enable all interrupts from the IO2.
	 *
	 *  The SBCC 16 bit registers are read/written as ulong, but only
	 *  bits 23-16 and 7-0 are meaningful.
	 */
	SBCCREG->fintenable |= 0xff;	  /* allow all interrupts on the IO2 */
	SBCCREG->idintenable |= 0x800000; /* allow interrupts from the IO2 */

	/*
	 *  Enable all interrupts on the IO2.  If IO3, run in compatibility mode.
	 */
	*IO2SETMASK = 0xff000000;

	vmedevinit();
}

int
vmeintr(long vec)
{
	if(vmevec[vec]) {
		(*vmevec[vec]->intr)(vmevec[vec]);
		return 1;
	}
	return 0;
}
