/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "e.h"

startcol(int type) /* mark start of column in lp[] array */
{
	int oct = ct;

	lp[ct++] = type;
	lp[ct++] = 0; /* count, to come */
	lp[ct++] = 0; /* separation, to come */
	return oct;
}

void column(int oct, int sep) /* remember end of column that started at lp[oct] */
{
	int i, type;

	lp[oct + 1] = ct - oct - 3;
	lp[oct + 2] = sep;
	type = lp[oct];
	if(dbg) {
		printf(".\t%d column of", type);
		for(i = oct + 3; i < ct; i++)
			printf(" S%d", lp[i]);
		printf(", rows=%d, sep=%d\n", lp[oct + 1], lp[oct + 2]);
	}
}

void matrix(int oct) /* matrix is list of columns */
{
	int nrow, ncol, i, j, k, val[100];
	double b, hb;
	char *space;
	extern char *Matspace;

	space = Matspace;   /* between columns of matrix */
	nrow = lp[oct + 1]; /* disaster if rows inconsistent */
			    /* also assumes just columns */
			    /* fix when add other things */
	ncol = 0;
	for(i = oct + 1; i < ct; i += lp[i] + 3) {
		ncol++;
		dprintf(".\tcolct=%d\n", lp[i]);
	}
	for(k = 1; k <= nrow; k++) {
		hb = b = 0;
		j = oct + k + 2;
		for(i = 0; i < ncol; i++) {
			hb = max(hb, eht[lp[j]] - ebase[lp[j]]);
			b = max(b, ebase[lp[j]]);
			j += nrow + 3;
		}
		dprintf(".\trow %d: b=%g, hb=%g\n", k, b, hb);
		j = oct + k + 2;
		for(i = 0; i < ncol; i++) {
			ebase[lp[j]] = b;
			eht[lp[j]] = b + hb;
			j += nrow + 3;
		}
	}
	j = oct;
	for(i = 0; i < ncol; i++) {
		pile(j);
		val[i] = yyval;
		j += nrow + 3;
	}
	yyval = salloc();
	eht[yyval] = eht[val[0]];
	ebase[yyval] = ebase[val[0]];
	lfont[yyval] = rfont[yyval] = 0;
	dprintf(".\tmatrix S%d: r=%d, c=%d, h=%g, b=%g\n",
		yyval, nrow, ncol, eht[yyval], ebase[yyval]);
	printf(".ds %d \"", yyval);
	for(i = 0; i < ncol; i++) {
		printf("\\*(%d%s", val[i], i == ncol - 1 ? "" : space);
		sfree(val[i]);
	}
	printf("\n");
}
