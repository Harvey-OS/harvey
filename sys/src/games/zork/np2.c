/* GETOBJ--	FIND OBJ DESCRIBED BY ADJ, NAME PAIR */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"
#include "parse.h"

static logical thisit_ P((integer, integer, integer, integer));

/* THIS ROUTINE DETAILS ON BIT 3 OF PRSFLG */

integer getobj_(oidx, aidx, spcobj)
integer oidx;
integer aidx;
integer spcobj;
{
    /* System generated locals */
    integer ret_val, i__1;

    /* Local variables */
    integer i, av;
    integer obj;
    integer nobj;
    logical chomp;

/* GETOBJ, PAGE 2 */

    chomp = FALSE_;
    av = advs_1.avehic[play_1.winner - 1];
    obj = 0;
/* 						!ASSUME DARK. */
    if (! lit_(play_1.here)) {
	goto L200;
    }
/* 						!LIT? */

    obj = schlst_(oidx, aidx, play_1.here, 0, 0, spcobj);
/* 						!SEARCH ROOM. */
    if (obj < 0) {
	goto L1000;
    } else if (obj == 0) {
	goto L200;
    } else {
	goto L100;
    }
/* 						!TEST RESULT. */
L100:
    if (av == 0 || av == obj || (objcts_1.oflag2[obj - 1] & FINDBT) 
	    != 0) {
	goto L200;
    }
    if (objcts_1.ocan[obj - 1] == av) {
	goto L200;
    }
/* 						!TEST IF REACHABLE. */
    chomp = TRUE_;
/* 						!PROBABLY NOT. */

L200:
    if (av == 0) {
	goto L400;
    }
/* 						!IN VEHICLE? */
    nobj = schlst_(oidx, aidx, 0, av, 0, spcobj);
/* 						!SEARCH VEHICLE. */
    if (nobj < 0) {
	goto L1100;
    } else if (nobj == 0) {
	goto L400;
    } else {
	goto L300;
    }
/* 						!TEST RESULT. */
L300:
    chomp = FALSE_;
/* 						!REACHABLE. */
    if (obj == nobj) {
	goto L400;
    }
/* 						!SAME AS BEFORE? */
    if (obj != 0) {
	nobj = -nobj;
    }
/* 						!AMB RESULT? */
    obj = nobj;

L400:
    nobj = schlst_(oidx, aidx, 0, 0, play_1.winner, spcobj);
/* 						!SEARCH ADVENTURER. */
    if (nobj < 0) {
	goto L1100;
    } else if (nobj == 0) {
	goto L600;
    } else {
	goto L500;
    }
/* 						!TEST RESULT */
L500:
    if (obj != 0) {
	nobj = -nobj;
    }
/* 						!AMB RESULT? */
L1100:
    obj = nobj;
/* 						!RETURN NEW OBJECT. */
L600:
    if (chomp) {
	obj = -10000;
    }
/* 						!UNREACHABLE. */
L1000:
    ret_val = obj;

    if (ret_val != 0) {
	goto L1500;
    }
/* 						!GOT SOMETHING? */
    i__1 = objcts_1.olnt;
    for (i = star_1.strbit + 1; i <= i__1; ++i) {
/* 						!NO, SEARCH GLOBALS. */
	if (! thisit_(oidx, aidx, i, spcobj)) {
	    goto L1200;
	}
	if (! ghere_(i, play_1.here)) {
	    goto L1200;
	}
/* 						!CAN IT BE HERE? */
	if (ret_val != 0) {
	    ret_val = -i;
	}
/* 						!AMB MATCH? */
	if (ret_val == 0) {
	    ret_val = i;
	}
L1200:
	;
    }

L1500:
/* 						!END OF SEARCH. */
    return ret_val;
} /* getobj_ */

/* SCHLST--	SEARCH FOR OBJECT */

/* DECLARATIONS */

integer schlst_(oidx, aidx, rm, cn, ad, spcobj)
integer oidx;
integer aidx;
integer rm;
integer cn;
integer ad;
integer spcobj;
{
    /* System generated locals */
    integer ret_val, i__1, i__2;

    /* Local variables */
    integer i, j, x;

    ret_val = 0;
/* 						!NO RESULT. */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!SEARCH OBJECTS. */
	if ((objcts_1.oflag1[i - 1] & VISIBT) == 0 || (rm == 0 || ! 
		qhere_(i, rm)) && (cn == 0 || objcts_1.ocan[i - 1] != cn) 
		&& (ad == 0 || objcts_1.oadv[i - 1] != ad)) {
	    goto L1000;
	}
	if (! thisit_(oidx, aidx, i, spcobj)) {
	    goto L200;
	}
	if (ret_val != 0) {
	    goto L2000;
	}
/* 						!GOT ONE ALREADY? */
	ret_val = i;
/* 						!NO. */

/* IF OPEN OR TRANSPARENT, SEARCH THE OBJECT ITSELF. */

L200:
	if ((objcts_1.oflag1[i - 1] & TRANBT) == 0 && (
		objcts_1.oflag2[i - 1] & OPENBT) == 0) {
	    goto L1000;
	}

/* SEARCH IS CONDUCTED IN REVERSE.  ALL OBJECTS ARE CHECKED TO */
/* SEE IF THEY ARE AT SOME LEVEL OF CONTAINMENT INSIDE OBJECT 'I'. */
/* IF THEY ARE AT LEVEL 1, OR IF ALL LINKS IN THE CONTAINMENT */
/* CHAIN ARE OPEN, VISIBLE, AND HAVE SEARCHME SET, THEY CAN QUALIFY */

/* AS A POTENTIAL MATCH. */

	i__2 = objcts_1.olnt;
	for (j = 1; j <= i__2; ++j) {
/* 						!SEARCH OBJECTS. */
	    if ((objcts_1.oflag1[j - 1] & VISIBT) == 0 || ! thisit_(
		    oidx, aidx, j, spcobj)) {
		goto L500;
	    }
	    x = objcts_1.ocan[j - 1];
/* 						!GET CONTAINER. */
L300:
	    if (x == i) {
		goto L400;
	    }
/* 						!INSIDE TARGET? */
	    if (x == 0) {
		goto L500;
	    }
/* 						!INSIDE ANYTHING? */
	    if ((objcts_1.oflag1[x - 1] & VISIBT) == 0 || (
		    objcts_1.oflag1[x - 1] & TRANBT) == 0 && (
		    objcts_1.oflag2[x - 1] & OPENBT) == 0 || (
		    objcts_1.oflag2[x - 1] & SCHBT) == 0) {
		goto L500;
	    }
	    x = objcts_1.ocan[x - 1];
/* 						!GO ANOTHER LEVEL. */
	    goto L300;

L400:
	    if (ret_val != 0) {
		goto L2000;
	    }
/* 						!ALREADY GOT ONE? */
	    ret_val = j;
/* 						!NO. */
L500:
	    ;
	}

L1000:
	;
    }
    return ret_val;

L2000:
    ret_val = -ret_val;
/* 						!AMB RETURN. */
    return ret_val;

} /* schlst_ */

/* THISIT--	VALIDATE OBJECT VS DESCRIPTION */

/* DECLARATIONS */

static logical thisit_(oidx, aidx, obj, spcobj)
integer oidx;
integer aidx;
integer obj;
integer spcobj;
{
    /* Initialized data */

/*    THE FOLLOWING DATA STATEMENT USED RADIX-50 NOTATION (R50MIN/1RA/) */

/*       IN RADIX-50 NOTATION, AN "A" IN THE FIRST POSITION IS */
/*       ENCODED AS 1*40*40 = 1600. */

    const integer r50min = 1600;

    /* System generated locals */
    logical ret_val;

    /* Local variables */
    integer i;

    ret_val = FALSE_;
/* 						!ASSUME NO MATCH. */
    if (spcobj != 0 && obj == spcobj) {
	goto L500;
    }

/* CHECK FOR OBJECT NAMES */

    i = oidx + 1;
L100:
    ++i;
    if (ovoc[i - 1] <= 0 || ovoc[i - 1] >= r50min) {
	return ret_val;
    }
/* 						!IF DONE, LOSE. */
    if (ovoc[i - 1] != obj) {
	goto L100;
    }
/* 						!IF FAIL, CONT. */

    if (aidx == 0) {
	goto L500;
    }
/* 						!ANY ADJ? */
    i = aidx + 1;
L200:
    ++i;
    if (avoc[i - 1] <= 0 || avoc[i - 1] >= r50min) {
	return ret_val;
    }
/* 						!IF DONE, LOSE. */
    if (avoc[i - 1] != obj) {
	goto L200;
    }
/* 						!IF FAIL, CONT. */

L500:
    ret_val = TRUE_;
    return ret_val;
} /* thisit_ */
