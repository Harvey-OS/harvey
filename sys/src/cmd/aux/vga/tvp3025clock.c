#include <u.h>
#include <libc.h>

#include "vga.h"

#define SCALE(f)	((f)/1)		/* could be /10 */

static void
init(Vga *vga, Ctlr *ctlr)
{
	int f;
	ulong d, dmax, fmin, fvco, n, nmax, p;

	verbose("%s->init\n", ctlr->name);
	if(ctlr->flag & Finit)
		return;

	if(vga->f == 0)
		vga->f = vga->mode->frequency;

	/*
	 * Look for values of n, d and p that give
	 * the least error for
	 *	Fvco = Frequency*((n+2)*8)/(d+2)
	 *	Fpll = Fvco/2**p
	 * N and d are 7-bits, p is 2-bits. Constraints:
	 *	Frequency/(n+2) > 1MHz
	 *	110MHz <= Fvco <= 220MHz
	 *	n, d >= 1
	 * Should try to minimise n, d.
	 *
	 * There's nothing like brute force and ignorance.
	 */
	fmin = vga->f;
	vga->d = 6;
	vga->n = 5;
	vga->p = 2;
	dmax = (Frequency/1000000)-2;
	for(d = 1; d < dmax; d++){
		/*
		 * Calculate an upper bound on n
		 * to satisfy the condition
		 *	Fvco <= 220MHz
		 * This will hopefully prevent arithmetic
		 * overflow.
		 */
		nmax = ((220000000+Frequency)*(d+2))/(Frequency*8) - 2;
		for(n = 1; n < nmax; n++){
			fvco = SCALE(Frequency)*((n+2)*8)/(d+2);
			if(fvco < SCALE(110000000) || fvco > SCALE(220000000))
				continue;
			for(p = 1; p < 4; p++){
				f = SCALE(vga->f) - (fvco>>p);
				if(f < 0)
					f = -f;
				if(f < fmin){
					fmin = f;
					vga->d = d;
					vga->n = n;
					vga->p = p;
				}
			}
		}
	}

	ctlr->flag |= Finit;
}


static void
load(Vga *vga, Ctlr *ctlr)
{
	uchar crt5c;

	verbose("%s->load\n", ctlr->name);

	crt5c = vgaxi(Crtx, 0x5C);
	vgaxo(Crtx, 0x5C, crt5c & ~0x20);

	tvp3020xo(0x2C, 0x00);
	tvp3020xo(0x2D, vga->d);
	tvp3020xo(0x2D, vga->n);
	tvp3020xo(0x2D, 0x08|vga->p);

	vgaxo(Crtx, 0x5C, crt5c);

	vga->misc |= 0x0C;
	vgao(MiscW, vga->misc);

	ctlr->flag |= Fload;
}

Ctlr tvp3025clock = {
	"tvp3025clock",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	load,				/* load */
	0,				/* dump */
};
