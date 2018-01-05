/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "e.h"

void boverb(int p1, int p2)
{
	int treg;
	double h, b, d, d1, d2;
	extern double Overgap, Overwid, Overline;

	treg = salloc();
	yyval = p1;
	d = EM(Overgap, ps);
	h = eht[p1] + eht[p2] + d;
	b = eht[p2] - d;
	dprintf(".\tS%d <- %d over %d; b=%g, h=%g\n",
		yyval, p1, p2, b, h);
	nrwid(p1, ps, p1);
	nrwid(p2, ps, p2);
	printf(".nr %d \\n(%d\n", treg, p1);
	printf(".if \\n(%d>\\n(%d .nr %d \\n(%d\n", p2, treg, treg, p2);
	printf(".nr %d \\n(%d+%gm\n", treg, treg, Overwid);
	d2 = eht[p2]-ebase[p2]-d;	/* denom */
	printf(".ds %d \\v'%gm'\\h'\\n(%du-\\n(%du/2u'\\*(%d\\v'%gm'\\\n",
		yyval, REL(d2,ps), treg, p2, p2, REL(-d2,ps));
	d1 = 2 * d + ebase[p1];		/* num */
	printf("\\h'-\\n(%du-\\n(%du/2u'\\v'%gm'\\*(%d\\v'%gm'\\\n",
		p2, p1, REL(-d1,ps), p1, REL(d1,ps));
	printf("\\h'-\\n(%du-\\n(%du/2u+%gm'\\v'%gm'\\l'\\n(%du-%gm'\\h'%gm'\\v'%gm'\n",
		treg, p1, Overline, REL(-d,ps),
		treg, 2*Overline, Overline, REL(d,ps));
	ebase[yyval] = b;
	eht[yyval] = h;
	lfont[yyval] = rfont[yyval] = 0;
	sfree(p2);
	sfree(treg);
}
