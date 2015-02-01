/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "a.h"

/*
 * 14. Three-part titles.
 */
void
r_lt(int argc, Rune **argv)
{
	Rune *p;
	
	if(argc < 2)
		nr(L(".lt"), evalscale(L("6.5i"), 'm'));
	else{
		if(argc > 2)
			warn("too many arguments for .lt");
		p = argv[1];
		if(p[0] == '-')
			nr(L(".lt"), getnr(L(".lt"))-evalscale(p+1, 'm'));
		else if(p[0] == '+')
			nr(L(".lt"), getnr(L(".lt"))+evalscale(p+1, 'm'));
		else
			nr(L(".lt"), evalscale(p, 'm'));
	}
}

void
t14init(void)
{
	addreq(L("tl"), r_warn, -1);
	addreq(L("pc"), r_nop, -1);	/* page number char */
	addreq(L("lt"), r_lt, -1);
}

