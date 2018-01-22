/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "e.h"

void sqrt(int p2)
{
	static int af = 0;
	int nps;	/* point size for radical */
	double radscale = 0.95;

	if (ttype == DEVPOST)
		radscale = 1.05;
	nps = ps * radscale * eht[p2] / EM(1.0,ps) + 0.99;	/* kludgy */
	nps = max(EFFPS(nps), ps);
	yyval = p2;
	if (ttype == DEVCAT || ttype == DEVAPS)
		eht[yyval] = EM(1.2, nps);
	else if (ttype == DEVPOST)
		eht[yyval] = EM(1.15, nps);
	else		/* DEV202, DEVPOST */
		eht[yyval] = EM(1.15, nps);
	dprintf(".\tS%d <- sqrt S%d;b=%g, h=%g, nps=%d\n",
		yyval, p2, ebase[yyval], eht[yyval], nps);
	printf(".as %d \\|\n", yyval);
	nrwid(p2, ps, p2);
	if (af++ == 0)
		printf(".af 10 01\n");	/* make it two digits when it prints */
	printf(".nr 10 %.3fu*\\n(.su/10\n", 9.2*eht[p2]);	/* this nonsense */
			/* guesses point size corresponding to height of stuff */
	printf(".ds %d \\v'%gm'\\s(\\n(10", yyval, REL(ebase[p2],ps));
	if (ttype == DEVCAT || ttype == DEVAPS)
		printf("\\v'-.2m'\\(sr\\l'\\n(%du\\(rn'\\v'.2m'", p2);
	else		/* DEV202, DEVPOST so far */
		printf("\\(sr\\l'\\n(%du\\(rn'", p2);
	printf("\\s0\\v'%gm'\\h'-\\n(%du'\\^\\*(%d\n", REL(-ebase[p2],ps), p2, p2);
	lfont[yyval] = rfont[yyval] = ROM;
}
