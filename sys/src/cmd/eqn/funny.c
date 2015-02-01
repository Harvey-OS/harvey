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

extern int Funnyps;
extern double Funnyht, Funnybase;

void funny(int n)
{
	char *f = 0;

	yyval = salloc();
	switch (n) {
	case SUM:
		f = lookup(deftbl, "sum_def")->cval; break;
	case UNION:
		f = lookup(deftbl, "union_def")->cval; break;
	case INTER:	/* intersection */
		f = lookup(deftbl, "inter_def")->cval; break;
	case PROD:
		f = lookup(deftbl, "prod_def")->cval; break;
	default:
		ERROR "funny type %d in funny", n FATAL;
	}
	printf(".ds %d %s\n", yyval, f);
	eht[yyval] = EM(1.0, ps+Funnyps) - EM(Funnyht, ps);
	ebase[yyval] = EM(Funnybase, ps);
	dprintf(".\tS%d <- %s; h=%g b=%g\n", 
		yyval, f, eht[yyval], ebase[yyval]);
	lfont[yyval] = rfont[yyval] = ROM;
}
