/* FINDXT- FIND EXIT FROM ROOM */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include <stdio.h>
#include "funcs.h"
#include "vars.h"

logical findxt_(dir, rm)
integer dir;
integer rm;
{
    /* System generated locals */
    logical ret_val;

    /* Local variables */
    integer i, xi;
    integer xxxflg;

    ret_val = TRUE_;
/* 						!ASSUME WINS. */
    xi = rooms_1.rexit[rm - 1];
/* 						!FIND FIRST ENTRY. */
    if (xi == 0) {
	goto L1000;
    }
/* 						!NO EXITS? */

L100:
    i = exits_1.travel[xi - 1];
/* 						!GET ENTRY. */
    curxt_1.xroom1 = i & xpars_1.xrmask;
/* mask to 16-bits to get rid of sign extension problems with 32-bit ints 
*/
    xxxflg = ~ xpars_1.xlflag & 65535;
    curxt_1.xtype = ((i & xxxflg) / xpars_1.xfshft & xpars_1.xfmask) + 1;
    switch (curxt_1.xtype) {
	case 1:  goto L110;
	case 2:  goto L120;
	case 3:  goto L130;
	case 4:  goto L130;
    }
/* 						!BRANCH ON ENTRY. */
    bug_(10, curxt_1.xtype);

L130:
    curxt_1.xobj = exits_1.travel[xi + 1] & xpars_1.xrmask;
    curxt_1.xactio = exits_1.travel[xi + 1] / xpars_1.xashft;
L120:
    curxt_1.xstrng = exits_1.travel[xi];
/* 						!DOOR/CEXIT/NEXIT - STRING. */
L110:
    xi += xpars_1.xelnt[curxt_1.xtype - 1];
/* 						!ADVANCE TO NEXT ENTRY. */
    if ((i & xpars_1.xdmask) == dir) {
	return ret_val;
    }
    if ((i & xpars_1.xlflag) == 0) {
	goto L100;
    }
L1000:
    ret_val = FALSE_;
/* 						!YES, LOSE. */
    return ret_val;
} /* findxt_ */

/* FWIM- FIND WHAT I MEAN */

/* DECLARATIONS */

integer fwim_(f1, f2, rm, con, adv, nocare)
integer f1;
integer f2;
integer rm;
integer con;
integer adv;
logical nocare;
{
    /* System generated locals */
    integer ret_val, i__1, i__2;

    /* Local variables */
    integer i, j;


/* OBJECTS */




    ret_val = 0;
/* 						!ASSUME NOTHING. */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!LOOP */
	if ((rm == 0 || objcts_1.oroom[i - 1] != rm) && (adv == 0 || 
		objcts_1.oadv[i - 1] != adv) && (con == 0 || objcts_1.ocan[
		i - 1] != con)) {
	    goto L1000;
	}

/* OBJECT IS ON LIST... IS IT A MATCH? */

	if ((objcts_1.oflag1[i - 1] & VISIBT) == 0) {
	    goto L1000;
	}
	if (~ (nocare) & (objcts_1.oflag1[i - 1] & TAKEBT) == 0 || (
		objcts_1.oflag1[i - 1] & f1) == 0 && (objcts_1.oflag2[i - 1] 
		& f2) == 0) {
	    goto L500;
	}
	if (ret_val == 0) {
	    goto L400;
	}
/* 						!ALREADY GOT SOMETHING? */
	ret_val = -ret_val;
/* 						!YES, AMBIGUOUS. */
	return ret_val;

L400:
	ret_val = i;
/* 						!NOTE MATCH. */

/* DOES OBJECT CONTAIN A MATCH? */

L500:
	if ((objcts_1.oflag2[i - 1] & OPENBT) == 0) {
	    goto L1000;
	}
	i__2 = objcts_1.olnt;
	for (j = 1; j <= i__2; ++j) {
/* 						!NO, SEARCH CONTENTS. */
	    if (objcts_1.ocan[j - 1] != i || (objcts_1.oflag1[j - 1] & 
		    VISIBT) == 0 || (objcts_1.oflag1[j - 1] & f1) ==
		     0 && (objcts_1.oflag2[j - 1] & f2) == 0) {
		goto L700;
	    }
	    if (ret_val == 0) {
		goto L600;
	    }
	    ret_val = -ret_val;
	    return ret_val;

L600:
	    ret_val = j;
L700:
	    ;
	}
L1000:
	;
    }
    return ret_val;
} /* fwim_ */

/* YESNO- OBTAIN YES/NO ANSWER */

/* CALLED BY- */

/* 	YES-IS-TRUE=YESNO(QUESTION,YES-STRING,NO-STRING) */

logical yesno_(q, y, n)
integer q;
integer y;
integer n;
{
    /* System generated locals */
    logical ret_val;

    /* Local variables */
    char ans[100];

L100:
    rspeak_(q);
/* 						!ASK */
    (void) fflush(stdout);
    (void) fgets(ans, sizeof ans, stdin);
    more_input();
/* 						!GET ANSWER */
    if (*ans == 'Y' || *ans == 'y') {
	goto L200;
    }
    if (*ans == 'N' || *ans == 'n') {
	goto L300;
    }
    rspeak_(6);
/* 						!SCOLD. */
    goto L100;

L200:
    ret_val = TRUE_;
/* 						!YES, */
    rspeak_(y);
/* 						!OUT WITH IT. */
    return ret_val;

L300:
    ret_val = FALSE_;
/* 						!NO, */
    rspeak_(n);
/* 						!LIKEWISE. */
    return ret_val;

} /* yesno_ */
