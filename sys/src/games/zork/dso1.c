/* PRINCR- PRINT CONTENTS OF ROOM */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

void princr_(full, rm)
logical full;
integer rm;
{
    /* System generated locals */
    integer i__1, i__2;

    /* Local variables */
    integer i, j, k;

    j = 329;
/* 						!ASSUME SUPERBRIEF FORMAT. */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!LOOP ON OBJECTS */
	if (! qhere_(i, rm) || (objcts_1.oflag1[i - 1] & VISIBT + 
		NDSCBT) != VISIBT || i == advs_1.avehic[
		play_1.winner - 1]) {
	    goto L500;
	}
	if (! (full) && (findex_1.superf || findex_1.brieff && (
		rooms_1.rflag[play_1.here - 1] & RSEEN) != 0)) {
	    goto L200;
	}

/* DO LONG DESCRIPTION OF OBJECT. */

	k = objcts_1.odesco[i - 1];
/* 						!GET UNTOUCHED. */
	if (k == 0 || (objcts_1.oflag2[i - 1] & TCHBT) != 0) {
	    k = objcts_1.odesc1[i - 1];
	}
	rspeak_(k);
/* 						!DESCRIBE. */
	goto L500;
/* DO SHORT DESCRIPTION OF OBJECT. */

L200:
	rspsub_(j, objcts_1.odesc2[i - 1]);
/* 						!YOU CAN SEE IT. */
	j = 502;

L500:
	;
    }

/* NOW LOOP TO PRINT CONTENTS OF OBJECTS IN ROOM. */

    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!LOOP ON OBJECTS. */
	if (! qhere_(i, rm) || (objcts_1.oflag1[i - 1] & VISIBT + 
		NDSCBT) != VISIBT) {
	    goto L1000;
	}
	if ((objcts_1.oflag2[i - 1] & ACTRBT) != 0) {
	    i__2 = oactor_(i);
	    invent_(i__2);
	}
	if ((objcts_1.oflag1[i - 1] & TRANBT) == 0 && (
		objcts_1.oflag2[i - 1] & OPENBT) == 0 || qempty_(i))
		 {
	    goto L1000;
	}

/* OBJECT IS NOT EMPTY AND IS OPEN OR TRANSPARENT. */

	j = 573;
	if (i != oindex_1.tcase) {
	    goto L600;
	}
/* 						!TROPHY CASE? */
	j = 574;
	if ((findex_1.brieff || findex_1.superf) && ! (full)) {
	    goto L1000;
	}
L600:
	princo_(i, j);
/* 						!PRINT CONTENTS. */

L1000:
	;
    }
} /* princr_ */

/* INVENT- PRINT CONTENTS OF ADVENTURER */

/* DECLARATIONS */

void invent_(adv)
integer adv;
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    integer i, j;

    i = 575;
/* 						!FIRST LINE. */
    if (adv != aindex_1.player) {
	i = 576;
    }
/* 						!IF NOT ME. */
    i__1 = objcts_1.olnt;
    for (j = 1; j <= i__1; ++j) {
/* 						!LOOP */
	if (objcts_1.oadv[j - 1] != adv || (objcts_1.oflag1[j - 1] & 
		VISIBT) == 0) {
	    goto L10;
	}
	rspsub_(i, objcts_1.odesc2[advs_1.aobj[adv - 1] - 1]);
	i = 0;
	rspsub_(502, objcts_1.odesc2[j - 1]);
L10:
	;
    }

    if (i == 0) {
	goto L25;
    }
/* 						!ANY OBJECTS? */
    if (adv == aindex_1.player) {
	rspeak_(578);
    }
/* 						!NO, TELL HIM. */
    return;

L25:
    i__1 = objcts_1.olnt;
    for (j = 1; j <= i__1; ++j) {
/* 						!LOOP. */
	if (objcts_1.oadv[j - 1] != adv || (objcts_1.oflag1[j - 1] & 
		VISIBT) == 0 || (objcts_1.oflag1[j - 1] & 
		TRANBT) == 0 && (objcts_1.oflag2[j - 1] & 
		OPENBT) == 0) {
	    goto L100;
	}
	if (! qempty_(j)) {
	    princo_(j, 573);
	}
/* 						!IF NOT EMPTY, LIST. */
L100:
	;
    }
} /* invent_ */

/* PRINCO-	PRINT CONTENTS OF OBJECT */

/* DECLARATIONS */

void princo_(obj, desc)
integer obj;
integer desc;
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    integer i;

    rspsub_(desc, objcts_1.odesc2[obj - 1]);
/* 						!PRINT HEADER. */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!LOOP THRU. */
	if (objcts_1.ocan[i - 1] == obj) {
	    rspsub_(502, objcts_1.odesc2[i - 1]);
	}
/* L100: */
    }
} /* princo_ */
