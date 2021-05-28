/* virtex 5 dependencies */
#include "include.h"
#include "qtm.h"

uvlong myhz = 400000000;	/* fixed 400MHz */
uchar mymac[Eaddrlen] = { 0x00, 0x0A, 0x35, 0x01, 0xE1, 0x48 };

void
clrmchk(void)
{
	putmcsr(~0);			/* clear machine check causes */
	sync();
	isync();
	putesr(0);			/* clears machine check */
}

static vlong
myprobe(uintptr addr)
{
	vlong v;

	iprint("myprobe: addr %#lux\n", addr);
	v = qtmprobeaddr(addr);
	if (v < 0)
		iprint("myprobe: failed for %#lux\n", addr);
	flushwrbufs();
	qtmerrtestaddr(addr);
	return v;
}

/*
 * size by watching for bus errors (machine checks), but avoid
 * tlb faults for unmapped memory beyond the maximum we expect.
 * furthermore, write entire aligned cachelines to avoid macfail
 * errors if we are using qtm.
 */
uintptr
memsize(void)
{
	uintptr sz;
#ifdef AMBITIOUS		/* this works on vanilla systems */
	int fault;

	/* try powers of two */
	fault = 0;
	if (securemem) {
		qrp->err = 0;
		coherence();
	}
	for (sz = 64*MB; sz != 0 && sz < MAXMEM; sz <<= 1)
		if (myprobe(sz) < 0) {
			fault = 1;
			break;
		}

	/* special handling for maximum size */
	if (sz >= MAXMEM && !fault && myprobe(MEMTOP(sz) - 2*DCACHELINESZ) < 0)
		sz >>= 1;

	if (securemem) {
		qrp->err = 0;		/* in case we perturbed qtm */
		coherence();
	}
#else
	/* the vanilla ddr2 system used to have 512MB, but now only 256MB */
	sz = 256*MB;
#endif
	return securemem? MEMTOP(sz): sz;
}
