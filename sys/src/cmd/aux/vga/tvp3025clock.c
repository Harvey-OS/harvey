#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

#define SCALE(f)	((f)/1)		/* could be /10 */

static void
init(Vga* vga, Ctlr* ctlr)
{
	int f;
	ulong d, dmax, fmin, fvco, n, nmax, p;

	if(ctlr->flag & Finit)
		return;

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;

	/*
	 * Look for values of n, d and p that give
	 * the least error for
	 *	Fvco = RefFreq*((n+2)*8)/(d+2)
	 *	Fpll = Fvco/2**p
	 * N and d are 7-bits, p is 2-bits. Constraints:
	 *	RefFreq/(n+2) > 1MHz
	 *	110MHz <= Fvco <= 220MHz
	 *	n, d >= 1
	 * Should try to minimise n, d.
	 *
	 * There's nothing like brute force and ignorance.
	 */
	fmin = vga->f[0];
	vga->d[0] = 6;
	vga->n[0] = 5;
	vga->p[0] = 2;
	dmax = (RefFreq/1000000)-2;
	for(d = 1; d < dmax; d++){
		/*
		 * Calculate an upper bound on n
		 * to satisfy the condition
		 *	Fvco <= 220MHz
		 * This will hopefully prevent arithmetic
		 * overflow.
		 */
		nmax = ((220000000+RefFreq)*(d+2))/(RefFreq*8) - 2;
		for(n = 1; n < nmax; n++){
			fvco = SCALE(RefFreq)*((n+2)*8)/(d+2);
			if(fvco < SCALE(110000000) || fvco > SCALE(220000000))
				continue;
			for(p = 1; p < 4; p++){
				f = SCALE(vga->f[0]) - (fvco>>p);
				if(f < 0)
					f = -f;
				if(f < fmin){
					fmin = f;
					vga->d[0] = d;
					vga->n[0] = n;
					vga->p[0] = p;
				}
			}
		}
	}

	ctlr->flag |= Finit;
}

Ctlr tvp3025clock = {
	"tvp3025clock",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	0,				/* load */
	0,				/* dump */
};
