#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

#define SCALE(f)	((f)/10)		/* could be /10 */

static void
init(Vga* vga, Ctlr* ctlr)
{
	int f, k;
	ulong fmin, fvco, m, n, p, q;
	double z;

	if(ctlr->flag & Finit)
		return;

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;
	vga->misc &= ~0x0C;
	if(vga->f[0] == VgaFreq0){
		/* nothing to do */;
	}
	else if(vga->f[0] == VgaFreq1)
		vga->misc |= 0x04;
	else
		vga->misc |= 0x0C;

	/*
	 * Look for values of n, d and p that give
	 * the least error for
	 *	Fvco = 8*RefFreq*(65-m)/(65-n)
	 *	Fpll = Fvco/2**p
	 * N and m are 6 bits, p is 2 bits. Constraints:
	 *	110MHz <= Fvco <= 250MHz
	 *	40 <= n <= 62
	 *	 1 <= m <= 62
	 *	 0 <= p <=  3
	 * Should try to minimise n, m.
	 *
	 * There's nothing like brute force and ignorance.
	 */
	fmin = vga->f[0];
	vga->m[0] = 0x15;
	vga->n[0] = 0x18;
	vga->p[0] = 3;
	for(m = 62; m > 0; m--){
		for(n = 62; n >= 40; n--){
			fvco = 8*SCALE(RefFreq)*(65-m)/(65-n);
			if(fvco < SCALE(110000000) || fvco > SCALE(250000000))
				continue;
			for(p = 0; p < 4; p++){
				f = SCALE(vga->f[0]) - (fvco>>p);
				if(f < 0)
					f = -f;
				if(f < fmin){
					fmin = f;
					vga->m[0] = m;
					vga->n[0] = n;
					vga->p[0] = p;
				}
			}
		}
	}

	/*
	 * Now the loop clock:
	 * 	m is fixed;
	 *	calculate n;
	 *	set z to the lower bound (110MHz) and calculate p and q.
	 */
	vga->m[1] = 61;
	if(ctlr->flag & Uenhanced)
		k = 64/8;
	else
		k = 8/8;
	n = 65 - 4*k;
	fvco = (8*RefFreq*(65-vga->m[0]))/(65-vga->n[0]);
	vga->f[1] = fvco;
	z = 110.0*(65-n)/(4*(fvco/1000000.0)*k);
	if(z <= 16){
		for(p = 0; p < 4; p++){
			if(1<<(p+1) > z)
				break;
		}
		q = 0;
	}
	else{
		p = 3;
		q = (z - 16)/16 + 1;
	}
	vga->n[1] = n;
	vga->p[1] = p;
	vga->q[1] = q;

	ctlr->flag |= Finit;
}

Ctlr tvp3026clock = {
	"tvp3026clock",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	0,				/* load */
	0,				/* dump */
};
