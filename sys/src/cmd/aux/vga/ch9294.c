/*
 * Chrontel
 * CH9294 Dual Enhanced Graphics Clock Generator.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

typedef struct {
	char*	name[2];
	ulong	frequency[16];
} Pattern;

static Pattern patterns[] = {
	{ "e", "E",		/* Tseng */
	 50350000,  56644000,  65000000,  72000000,  80000000,  89800000,  63000000,  75000000,
	 VgaFreq0,  VgaFreq1,  31500000,  36000000,  40000000,  44900000,  50000000,  65000000,
	},
	{ "g", "G",		/* S3, IIT */
	 VgaFreq0,  VgaFreq1,  40000000,  72000000,  50000000,  77000000,  36000000,  44900000,
	130000000, 120000000,  80000000,  31500000, 110000000,  65000000,  75000000,  94500000,
	},
	{ "k", "K",		/* Avance Logic */
	 50350000,  56644000,  89800000,  72000000,  75000000,  65000000,  63000000,  80000000,
	 57272000,  85000000,  94000000,  96000000, 100000000, 108000000, 110000000,  77000000,
	},

	{ 0,
	},
};

static void
init(Vga* vga, Ctlr* ctlr)
{
	Pattern *pattern;
	char *p;
	int f, fmin, index, divisor, maxdivisor;

	if(ctlr->flag & Finit)
		return;

	if(vga->f[0] == 0)
		vga->f[0] = vga->mode->frequency;

	if((p = strchr(ctlr->name, '-')) == 0)
		error("%s: unknown pattern\n", ctlr->name);
	p++;

	for(pattern = patterns; pattern->name[0]; pattern++){
		if(strcmp(pattern->name[0], p) == 0)
			break;
		if(pattern->name[1] && strcmp(pattern->name[1], p) == 0)
			break;
	}
	if(pattern->name[0] == 0)
		error("%s: unknown pattern\n", ctlr->name);

	maxdivisor = 1;
	if(vga->ctlr && (vga->ctlr->flag & Hclkdiv))
		maxdivisor = 8;
	fmin = vga->f[0];
	for(index = 0; index < 16; index++){
		for(divisor = 1; divisor <= maxdivisor; divisor <<= 1){
			f = vga->f[0] - pattern->frequency[index]/divisor;
			if(f < 0)
				f = -f;
			if(f < fmin){
				/*vga->f = pattern->frequency[index];*/
				fmin = f;
				vga->d[0] = divisor;
				vga->i[0] = index;
			}
		}
	}

	if(fmin > (vga->f[0]*5)/100)
		error("%s: can't find frequency %ld\n", ctlr->name, vga->f[0]);
	ctlr->flag |= Finit;
}

Ctlr ch9294 = {
	"ch9294",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	0,				/* load */
	0,				/* dump */
};
