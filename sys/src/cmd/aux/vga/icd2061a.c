/*
 * IC Designs ICD2061A Dual Programmable Graphics Clock Generator.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

enum {
	Prescale	= 2,			/* P counter prescale (default) */
	NIndex		= 14,			/* number of index field values */
};

/*
 * For an index value of x, the appropriate VCO range
 * is >= index[x] && <= index[x+1]. The higher index is
 * prefered if VCO is on a boundary.
 */
static ulong index[NIndex] = {
	 50000000,
	 51000000,
	 53200000,
	 58500000,
	 60700000,
	 64400000,
	 66800000,
	 73500000,
	 75600000,
	 80900000,
	 83200000,
	 91500000,
	100000000,
	120000000,
};

static void
init(Vga* vga, Ctlr* ctlr)
{
	int f;
	ulong d, dmax, fmin, n;

	if(ctlr->flag & Finit)
		return;

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;

	if(vga->mode->z > 8)
		error("depth %d not supported\n", vga->mode->z);

	/*
	 * Post-VCO divisor. Constraint:
	 * 	50MHz <= vga->f <= 120MHz
	 */
	for(vga->p[0] = 0; vga->f[0] <= 50000000; vga->p[0]++)
		vga->f[0] <<= 1;

	/*
	 * Determine index.
	 */
	for(vga->i[0] = NIndex-1; vga->f[0] < index[vga->i[0]] && vga->i[0]; vga->i[0]--)
		;

	/*
	 * Denominator. Constraints:
	 *	200KHz <= RefFreq/d <= 1MHz
	 * and
	 *	3 <= d <= 129
	 *
	 * Numerator. Constraint:
	 *	4 <= n <= 130
	 */
	d = RefFreq/1000000 > 3 ? RefFreq/1000000: 3;
	dmax = RefFreq/200000 < 129 ? RefFreq/200000: 129;

	/*
	 * Now look for values of p and q that give
	 * the least error for
	 *	vga->f = (Prescale*RefFreq*n/d);
	 */
	vga->d[0] = d;
	vga->n[0] = 4;
	for(fmin = vga->f[0]; d <= dmax; d++){
		for(n = 4; n <= 130; n++){
			f = vga->f[0] - (Prescale*RefFreq*n/d);
			if(f < 0)
				f = -f;
			if(f < fmin){
				fmin = f;
				vga->d[0] = d;
				vga->n[0] = n;
			}
		}
	}

	/*
	 * The serial word to be loaded into the icd2061a is
	 *	(2<<21)|(vga->i<<17)|((vga->n)<<10)|(vga->p<<7)|vga->d
	 * Always select ICD2061A REG2.
	 */
	vga->f[0] = (Prescale*RefFreq*vga->n[0]/vga->d[0]);
	vga->d[0] -= 2;
	vga->n[0] -= 3;

	ctlr->flag |= Finit;
}

Ctlr icd2061a = {
	"icd2061a",
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	0,				/* load */
	0,				/* dump */
};
