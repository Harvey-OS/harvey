/* ENCRYP--	ENCRYPT PASSWORD */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include <stdio.h>
#include "funcs.h"
#include "vars.h"

void encryp_(inw, outw)
const char *inw;
char *outw;
{
    /* Initialized data */

    const char *keyw = "ECORMS";

    integer i, j, uinw[6], usum;
    char ukeyw[1*6];
    integer uinws, ukeyws;

    /* Parameter adjustments */
    --outw;
    --inw;

    /* Function Body */

    uinws = 0;
/* 						!UNBIASED INW SUM. */
    ukeyws = 0;
/* 						!UNBIASED KEYW SUM. */
    j = 1;
/* 						!POINTER IN KEYWORD. */
    for (i = 1; i <= 6; ++i) {
/* 						!UNBIAS, COMPUTE SUMS. */
	ukeyw[i - 1] = (keyw[i - 1] - 64);
	if (inw[j] <= '@') {
	    j = 1;
	}
	uinw[i - 1] = inw[j] - 64;
	ukeyws += ukeyw[i - 1];
	uinws += uinw[i - 1];
	++j;
/* L100: */
    }

    usum = uinws % 8 + (ukeyws % 8 << 3);
/* 						!COMPUTE MASK. */
    for (i = 1; i <= 6; ++i) {
	j = (uinw[i - 1] ^ ukeyw[i - 1] ^ usum) & 31;
	usum = (usum + 1) % 32;
	if (j > 26) {
	    j %= 26;
	}
	outw[i] = (max(1,j) + 64);
/* L200: */
    }
} /* encryp_ */

/* CPGOTO--	MOVE TO NEXT STATE IN PUZZLE ROOM */

/* DECLARATIONS */

void cpgoto_(st)
integer st;
{
    /* System generated locals */
    integer i__1, i__2;

    /* Local variables */
    integer i;

    rooms_1.rflag[rindex_1.cpuzz - 1] &= ~ RSEEN;
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!RELOCATE OBJECTS. */
	if (objcts_1.oroom[i - 1] == rindex_1.cpuzz && (objcts_1.oflag2[i - 1]
		 & ACTRBT + VILLBT) == 0) {
	    i__2 = findex_1.cphere * hyper_1.hfactr;
	    newsta_(i, 0, i__2, 0, 0);
	}
	if (objcts_1.oroom[i - 1] == st * hyper_1.hfactr) {
	    newsta_(i, 0, rindex_1.cpuzz, 0, 0);
	}
/* L100: */
    }
    findex_1.cphere = st;
} /* cpgoto_ */

/* CPINFO--	DESCRIBE PUZZLE ROOM */

/* DECLARATIONS */

void cpinfo_(rmk, st)
integer rmk;
integer st;
{
    /* Initialized data */

    static const integer dgmoft[8] = { -9,-8,-7,-1,1,7,8,9 };
    const char *pict = "SSS M";

    /* Local variables */
    integer i, j, k, l;
    char dgm[1*8];

    rspeak_(rmk);
    for (i = 1; i <= 8; ++i) {
	j = dgmoft[i - 1];
	dgm[i - 1] = pict[puzzle_1.cpvec[st + j - 1] + 3];
/* 						!GET PICTURE ELEMENT. */
	if (abs(j) == 1 || abs(j) == 8) {
	    goto L100;
	}
	k = 8;
	if (j < 0) {
	    k = -8;
	}
/* 						!GET ORTHO DIR. */
	l = j - k;
	if (puzzle_1.cpvec[st + k - 1] != 0 && puzzle_1.cpvec[st + l - 1] !=
		 0) {
	    dgm[i - 1] = '?';
	}
L100:
	;
    }

    more_output(NULL);
    printf("       |%c%c %c%c %c%c|\n", dgm[0], dgm[0], dgm[1], dgm[1],
					dgm[2], dgm[2]);
    more_output(NULL);
    printf(" West  |%c%c .. %c%c| East\n", dgm[3], dgm[3], dgm[4], dgm[4]);
    more_output(NULL);
    printf("       |%c%c %c%c %c%c|\n", dgm[5], dgm[5], dgm[6], dgm[6],
					dgm[7], dgm[7]);

    if (st == 10) {
	rspeak_(870);
    }
/* 						!AT HOLE? */
    if (st == 37) {
	rspeak_(871);
    }
/* 						!AT NICHE? */
    i = 872;
/* 						!DOOR OPEN? */
    if (findex_1.cpoutf) {
	i = 873;
    }
    if (st == 52) {
	rspeak_(i);
    }
/* 						!AT DOOR? */
    if (puzzle_1.cpvec[st] == -2) {
	rspeak_(874);
    }
/* 						!EAST LADDER? */
    if (puzzle_1.cpvec[st - 2] == -3) {
	rspeak_(875);
    }
/* 						!WEST LADDER? */
} /* cpinfo_ */
