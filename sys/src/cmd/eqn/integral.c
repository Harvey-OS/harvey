/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "e.h"
#include "y.tab.h"

extern int Intps;
extern double Intht, Intbase, Int1h, Int1v, Int2h, Int2v;

void integral(int p, int p1, int p2)
{
	if (p1 != 0)
		printf(".ds %d \\h'%gm'\\v'%gm'\\*(%d\\v'%gm'\n", p1, -Int1h, Int1v, p1, -Int1v);
	if (p2 != 0)
		printf(".ds %d \\v'%gm'\\h'%gm'\\*(%d\\v'%gm'\n", p2, -Int2v, Int2h, p2, Int2v);
	if (p1 != 0 && p2 != 0)
		shift2(p, p1, p2);
	else if (p1 != 0)
		bshiftb(p, SUB, p1);
	else if (p2 != 0)
		bshiftb(p, SUP, p2);
	dprintf(".\tintegral: S%d; h=%g b=%g\n", p, eht[p], ebase[p]);
	lfont[p] = ROM;
}

void setintegral(void)
{
	yyval = salloc();
	printf(".ds %d %s\n", yyval, lookup(deftbl, "int_def")->cval);
	eht[yyval] = EM(Intht, ps+Intps);
	ebase[yyval] = EM(Intbase, ps);
	lfont[yyval] = rfont[yyval] = ROM;
}
