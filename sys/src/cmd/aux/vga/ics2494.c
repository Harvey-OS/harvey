/*
 * Integrated Circuit Systems, Inc.
 * ICS2494[A] Dual Video/Memory Clock Generator.
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
	{ "237", "304",
	 50350000,  56644000,  65000000,  72000000,  80000000,  89800000,  63000000,  75000000,
	 25175000,  28322000,  31500000,  36000000,  40000000,  44900000,  50000000,  65000000,
	},

	{ "324", 0,
	 50350000,  56644000,  65000000,  72000000,  80000000,  89800000,  63000000,  75000000,
	 83078000,  93463000, 100000000, 104000000, 108000000, 120000000, 130000000, 134700000,
	},

	{ 0,
	},
};

static void
init(Vga* vga, Ctlr* ctlr)
{
	Pattern *pattern;
	char *p;
	int f, index, divisor, maxdivisor;

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
	for(index = 0; index < 16; index++){
		for(divisor = 1; divisor <= maxdivisor; divisor <<= 1){
			f = vga->f[0] - pattern->frequency[index]/divisor;
			if(f < 0)
				f = -f;
			if(f < 1000000){
				/*vga->f = pattern->frequency[index];*/
				vga->d[0] = divisor;
				vga->i[0] = index;

				ctlr->flag |= Finit;
				return;
			}
		}
	}
	error("%s: can't find frequency %ld\n", ctlr->name, vga->f[0]);
}

Ctlr ics2494 = {
	"ics2494",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	0,				/* load */
	0,				/* dump */
};

Ctlr ics2494a = {
	"ics2494a",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	0,				/* load */
	0,				/* dump */
};
