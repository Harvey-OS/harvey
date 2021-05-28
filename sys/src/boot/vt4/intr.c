/* Xilink XPS interrupt controller */

#include "include.h"

enum {
	/* mer bits */
	Merme	= 1<<0,		/* master enable */
	Merhie	= 1<<1,		/* hw intr enable */

	Maxintrs = 8,		/* from 32; size reduction */
};

typedef struct {
	ulong	isr;		/* status */
	ulong	ipr;		/* pending (ro) */
	ulong	ier;		/* enable */
	ulong	iar;		/* acknowledge (wo) */
	ulong	sieb;		/* set ie bits; avoid */
	ulong	cieb;		/* clear ie bits; avoid */
	ulong	ivr;		/* vector; silly */
	ulong	mer;		/* master enable */
} Intregs;

typedef struct {
	ulong	bit;
	int	(*func)(ulong);
} Intr;

Intregs *irp = (Intregs *)Intctlr;
Intr intrs[Maxintrs];
Intr *nextintr;

static uvlong extintrs;

/* called from trap to poll for external interrupts */
void
intr(Ureg *)
{
	int handled;
	Intr *ip;

	extintrs++;
	dcflush(PTR2UINT(&extintrs), sizeof extintrs);	/* seems needed */
	handled = 0;
	for (ip = intrs; ip->bit != 0; ip++)
		handled |= ip->func(ip->bit);
	if (!handled)
		print("interrupt with no handler\n");
}

void
intrinit(void)
{
	intrack(~0);			/* clear dregs */
	putesr(0);			/* clears machine check */
	coherence();

	irp->mer = Merme | Merhie;
	irp->ier = 0;			/* clear any pending spurious intrs */
	coherence();
	nextintr = intrs;		/* touches sram, not dram */
	nextintr->bit = 0;
	coherence();

	intrack(~0);			/* clear dregs */
	putesr(0);			/* clears machine check */
	coherence();
}

/* register func as the interrupt-service routine for bit */
void
intrenable(ulong bit, int (*func)(ulong))
{
	Intr *ip;

	if (func == nil)
		return;
	assert(nextintr < intrs + nelem(intrs));
	assert(bit);
	for (ip = intrs; ip->bit != 0; ip++)
		if (bit == ip->bit) {
			iprint("handler for intr bit %#lux already "
				"registered\n", bit);
			return;
		}
	nextintr->bit = bit;
	nextintr->func = func;
	nextintr++;
	coherence();
	irp->ier |= bit;
	coherence();
}

void
intrack(ulong bit)
{
	if (bit) {
		irp->iar = bit;
		coherence();
	}
}
