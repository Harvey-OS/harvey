/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * code to keep track of registers
 */

#include "defs.h"
#include "fns.h"

/*
 * translate a name to a magic register offset
 */
Reglist *
rname(char *name)
{
	Reglist *rp;

	for(rp = mach->reglist; rp->rname; rp++)
		if(strcmp(name, rp->rname) == 0)
			return rp;
	return 0;
}

static uint64_t
getreg(Map *map, Reglist *rp)
{
	uint64_t v;
	uint32_t w;
	uint16_t s;
	int ret;

	v = 0;
	ret = 0;
	switch(rp->rformat) {
	case 'x':
		ret = get2(map, rp->roffs, &s);
		v = s;
		break;
	case 'f':
	case 'X':
		ret = get4(map, rp->roffs, &w);
		v = w;
		break;
	case 'F':
	case 'W':
	case 'Y':
		ret = get8(map, rp->roffs, &v);
		break;
	default:
		werrstr("can't retrieve register %s", rp->rname);
		error("%r");
	}
	if(ret < 0) {
		werrstr("Register %s: %r", rp->rname);
		error("%r");
	}
	return v;
}

uint64_t
rget(Map *map, char *name)
{
	Reglist *rp;

	rp = rname(name);
	if(!rp)
		error("invalid register name");
	return getreg(map, rp);
}

void
rput(Map *map, char *name, int64_t v)
{
	Reglist *rp;
	int ret;

	rp = rname(name);
	if(!rp)
		error("invalid register name");
	if(rp->rflags & RRDONLY)
		error("register is read-only");
	switch(rp->rformat) {
	case 'x':
		ret = put2(map, rp->roffs, (uint16_t)v);
		break;
	case 'X':
	case 'f':
	case 'F':
		ret = put4(map, rp->roffs, (int32_t)v);
		break;
	case 'Y':
		ret = put8(map, rp->roffs, v);
		break;
	default:
		ret = -1;
	}
	if(ret < 0)
		error("can't write register");
}
/*
 * print the registers
 */
void
printregs(int c)
{
	Reglist *rp;
	int i;
	uint64_t v;

	for(i = 1, rp = mach->reglist; rp->rname; rp++, i++) {
		if((rp->rflags & RFLT)) {
			if(c != 'R')
				continue;
			if(rp->rformat == '8' || rp->rformat == '3')
				continue;
		}
		v = getreg(cormap, rp);
		if(rp->rformat == 'Y')
			dprint("%-8s %-20#llux", rp->rname, v);
		else
			dprint("%-8s %-12#lux", rp->rname, (uint32_t)v);
		if((i % 3) == 0) {
			dprint("\n");
			i = 0;
		}
	}
	if(i != 1)
		dprint("\n");
	dprint("%s\n", machdata->excep(cormap, rget));
	printpc();
}
