/*
 * Chrontel
 * CH9294 Dual Enhanced Graphics Clock Generator.
 */
#include <u.h>
#include <libc.h>

#include "vga.h"

typedef struct {
	char*	name[2];
	ulong	frequency[16];
} Pattern;

static Pattern patterns[] = {
	{ "g", "G",
	 25175000,  28322000,  40000000,  72000000,  50000000,  77000000,  36000000,  44900000,
	130000000, 120000000,  80000000,  31500000, 110000000,  65000000,  75000000,  94500000,
	},

	{ 0,
	},
};

static void
init(Vga *vga, Ctlr *ctlr)
{
	Pattern *pattern;
	char *p;
	int f, index;

	verbose("%s->init\n", ctlr->name);
	if(ctlr->flag & Finit)
		return;

	if(vga->f == 0)
		vga->f = vga->mode->frequency;

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

	for(index = 0; index < 16; index++){
		f = vga->f - pattern->frequency[index];
		if(f < 0)
			f = -f;
		if(f < 1000000){
			vga->i = index;

			ctlr->flag |= Finit;
			return;
		}
	}
	error("%s: can't find frequency %ld\n", ctlr->name, vga->f);
}

Ctlr ch9294 = {
	"ch9294",			/* name */
	0,				/* snarf */
	0,				/* options */
	init,				/* init */
	0,				/* load */
	0,				/* dump */
};
