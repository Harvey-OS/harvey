/* SYNMCH--	SYNTAX MATCHER */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"
#include "parse.h"

static void unpack_ P((integer, integer *));
static integer gwim_ P((integer, integer, integer));
static logical syneql_ P((integer, integer, integer, integer, integer));
static logical takeit_ P((integer, integer));

/* THIS ROUTINE DETAILS ON BIT 4 OF PRSFLG */

logical synmch_()
{
    /* Initialized data */

/*   THE FOLLOWING DATA STATEMENT WAS ORIGINALLY: */

/* 	DATA R50MIN/1RA/ */

    const integer r50min = 1600;

    /* System generated locals */
    integer i__1;
    logical ret_val;

    /* Local variables */
    integer j;
    integer newj;
    integer drive, limit, qprep, sprep, dforce;

    ret_val = FALSE_;
    j = pv_1.act;
/* 						!SET UP PTR TO SYNTAX. */
    drive = 0;
/* 						!NO DEFAULT. */
    dforce = 0;
/* 						!NO FORCED DEFAULT. */
    qprep = orphs_1.oflag & orphs_1.oprep;
L100:
    j += 2;
/* 						!FIND START OF SYNTAX. */
    if (vvoc[j - 1] <= 0 || vvoc[j - 1] >= r50min) {
	goto L100;
    }
    limit = j + vvoc[j - 1] + 1;
/* 						!COMPUTE LIMIT. */
    ++j;
/* 						!ADVANCE TO NEXT. */

L200:
    unpack_(j, &newj);
/* 						!UNPACK SYNTAX. */
    sprep = syntax_1.dobj & VPMASK;
    if (! syneql_(pv_1.p1, pv_1.o1, syntax_1.dobj, syntax_1.dfl1, 
	    syntax_1.dfl2)) {
	goto L1000;
    }
    sprep = syntax_1.iobj & VPMASK;
    if (syneql_(pv_1.p2, pv_1.o2, syntax_1.iobj, syntax_1.ifl1, 
	    syntax_1.ifl2)) {
	goto L6000;
    }

/* SYNTAX MATCH FAILS, TRY NEXT ONE. */

    if (pv_1.o2 != 0) {
	goto L3000;
    } else {
	goto L500;
    }
/* 						!IF O2=0, SET DFLT. */
L1000:
    if (pv_1.o1 != 0) {
	goto L3000;
    } else {
	goto L500;
    }
/* 						!IF O1=0, SET DFLT. */
L500:
    if (qprep == 0 || qprep == sprep) {
	dforce = j;
    }
/* 						!IF PREP MCH. */
    if ((syntax_1.vflag & SDRIV) != 0) {
	drive = j;
    }
L3000:
    j = newj;
    if (j < limit) {
	goto L200;
    }
/* 						!MORE TO DO? */
/* SYNMCH, PAGE 2 */

/* MATCH HAS FAILED.  IF DEFAULT SYNTAX EXISTS, TRY TO SNARF */
/* ORPHANS OR GWIMS, OR MAKE NEW ORPHANS. */

    if (drive == 0) {
	drive = dforce;
    }
/* 						!NO DRIVER? USE FORCE. */
    if (drive == 0) {
	goto L10000;
    }
/* 						!ANY DRIVER? */
    unpack_(drive, &dforce);
/* 						!UNPACK DFLT SYNTAX. */

/* TRY TO FILL DIRECT OBJECT SLOT IF THAT WAS THE PROBLEM. */

    if ((syntax_1.vflag & SDIR) == 0 || pv_1.o1 != 0) {
	goto L4000;
    }

/* FIRST TRY TO SNARF ORPHAN OBJECT. */

    pv_1.o1 = orphs_1.oflag & orphs_1.oslot;
    if (pv_1.o1 == 0) {
	goto L3500;
    }
/* 						!ANY ORPHAN? */
    if (syneql_(pv_1.p1, pv_1.o1, syntax_1.dobj, syntax_1.dfl1, 
	    syntax_1.dfl2)) {
	goto L4000;
    }

/* ORPHAN FAILS, TRY GWIM. */

L3500:
    pv_1.o1 = gwim_(syntax_1.dobj, syntax_1.dfw1, syntax_1.dfw2);
/* 						!GET GWIM. */
    if (pv_1.o1 > 0) {
	goto L4000;
    }
/* 						!TEST RESULT. */
    i__1 = syntax_1.dobj & VPMASK;
    orphan_(- 1, pv_1.act, 0, i__1, 0);
    rspeak_(623);
    return ret_val;

/* TRY TO FILL INDIRECT OBJECT SLOT IF THAT WAS THE PROBLEM. */

L4000:
    if ((syntax_1.vflag & SIND) == 0 || pv_1.o2 != 0) {
	goto L6000;
    }
    pv_1.o2 = gwim_(syntax_1.iobj, syntax_1.ifw1, syntax_1.ifw2);
/* 						!GWIM. */
    if (pv_1.o2 > 0) {
	goto L6000;
    }
    if (pv_1.o1 == 0) {
	pv_1.o1 = orphs_1.oflag & orphs_1.oslot;
    }
    i__1 = syntax_1.dobj & VPMASK;
    orphan_(- 1, pv_1.act, pv_1.o1, i__1, 0);
    rspeak_(624);
    return ret_val;

/* TOTAL CHOMP */

L10000:
    rspeak_(601);
/* 						!CANT DO ANYTHING. */
    return ret_val;
/* SYNMCH, PAGE 3 */

/* NOW TRY TO TAKE INDIVIDUAL OBJECTS AND */
/* IN GENERAL CLEAN UP THE PARSE VECTOR. */

L6000:
    if ((syntax_1.vflag & SFLIP) == 0) {
	goto L5000;
    }
    j = pv_1.o1;
/* 						!YES. */
    pv_1.o1 = pv_1.o2;
    pv_1.o2 = j;

L5000:
    prsvec_1.prsa = syntax_1.vflag & SVMASK;
    prsvec_1.prso = pv_1.o1;
/* 						!GET DIR OBJ. */
    prsvec_1.prsi = pv_1.o2;
/* 						!GET IND OBJ. */
    if (! takeit_(prsvec_1.prso, syntax_1.dobj)) {
	return ret_val;
    }
/* 						!TRY TAKE. */
    if (! takeit_(prsvec_1.prsi, syntax_1.iobj)) {
	return ret_val;
    }
/* 						!TRY TAKE. */
    ret_val = TRUE_;
    return ret_val;

} /* synmch_ */

/* UNPACK-	UNPACK SYNTAX SPECIFICATION, ADV POINTER */

/* DECLARATIONS */

static void unpack_(oldj, j)
integer oldj;
integer *j;
{
    /* Local variables */
    integer i;

    for (i = 1; i <= 11; ++i) {
/* 						!CLEAR SYNTAX. */
	syn[i - 1] = 0;
/* L10: */
    }

    syntax_1.vflag = vvoc[oldj - 1];
    *j = oldj + 1;
    if ((syntax_1.vflag & SDIR) == 0) {
	return;
    }
    syntax_1.dfl1 = -1;
/* 						!ASSUME STD. */
    syntax_1.dfl2 = -1;
    if ((syntax_1.vflag & SSTD) == 0) {
	goto L100;
    }
    syntax_1.dfw1 = -1;
/* 						!YES. */
    syntax_1.dfw2 = -1;
    syntax_1.dobj = VABIT + VRBIT + VFBIT;
    goto L200;

L100:
    syntax_1.dobj = vvoc[*j - 1];
/* 						!NOT STD. */
    syntax_1.dfw1 = vvoc[*j];
    syntax_1.dfw2 = vvoc[*j + 1];
    *j += 3;
    if ((syntax_1.dobj & VEBIT) == 0) {
	goto L200;
    }
    syntax_1.dfl1 = syntax_1.dfw1;
/* 						!YES. */
    syntax_1.dfl2 = syntax_1.dfw2;

L200:
    if ((syntax_1.vflag & SIND) == 0) {
	return;
    }
    syntax_1.ifl1 = -1;
/* 						!ASSUME STD. */
    syntax_1.ifl2 = -1;
    syntax_1.iobj = vvoc[*j - 1];
    syntax_1.ifw1 = vvoc[*j];
    syntax_1.ifw2 = vvoc[*j + 1];
    *j += 3;
    if ((syntax_1.iobj & VEBIT) == 0) {
	return;
    }
    syntax_1.ifl1 = syntax_1.ifw1;
/* 						!YES. */
    syntax_1.ifl2 = syntax_1.ifw2;
} /* unpack_ */

/* SYNEQL-	TEST FOR SYNTAX EQUALITY */

/* DECLARATIONS */

static logical syneql_(prep, obj, sprep, sfl1, sfl2)
integer prep;
integer obj;
integer sprep;
integer sfl1;
integer sfl2;
{
    /* System generated locals */
    logical ret_val;

    if (obj == 0) {
	goto L100;
    }
/* 						!ANY OBJECT? */
    ret_val = prep == (sprep & VPMASK) && (sfl1 & objcts_1.oflag1[
	    obj - 1] | sfl2 & objcts_1.oflag2[obj - 1]) != 0;
    return ret_val;

L100:
    ret_val = prep == 0 && sfl1 == 0 && sfl2 == 0;
    return ret_val;

} /* syneql_ */

/* TAKEIT-	PARSER BASED TAKE OF OBJECT */

/* DECLARATIONS */

static logical takeit_(obj, sflag)
integer obj;
integer sflag;
{
    /* System generated locals */
    logical ret_val;

    /* Local variables */
    integer x;
    integer odo2;

/* TAKEIT, PAGE 2 */

    ret_val = FALSE_;
/* 						!ASSUME LOSES. */
    if (obj == 0 || obj > star_1.strbit) {
	goto L4000;
    }
/* 						!NULL/STARS WIN. */
    odo2 = objcts_1.odesc2[obj - 1];
/* 						!GET DESC. */
    x = objcts_1.ocan[obj - 1];
/* 						!GET CONTAINER. */
    if (x == 0 || (sflag & VFBIT) == 0) {
	goto L500;
    }
    if ((objcts_1.oflag2[x - 1] & OPENBT) != 0) {
	goto L500;
    }
    rspsub_(566, odo2);
/* 						!CANT REACH. */
    return ret_val;

L500:
    if ((sflag & VRBIT) == 0) {
	goto L1000;
    }
    if ((sflag & VTBIT) == 0) {
	goto L2000;
    }

/* SHOULD BE IN ROOM (VRBIT NE 0) AND CAN BE TAKEN (VTBIT NE 0) */

    if (schlst_(0, 0, play_1.here, 0, 0, obj) <= 0) {
	goto L4000;
    }
/* 						!IF NOT, OK. */

/* ITS IN THE ROOM AND CAN BE TAKEN. */

    if ((objcts_1.oflag1[obj - 1] & TAKEBT) != 0 && (
	    objcts_1.oflag2[obj - 1] & TRYBT) == 0) {
	goto L3000;
    }

/* NOT TAKEABLE.  IF WE CARE, FAIL. */

    if ((sflag & VCBIT) == 0) {
	goto L4000;
    }
    rspsub_(445, odo2);
    return ret_val;

/* 1000--	IT SHOULD NOT BE IN THE ROOM. */
/* 2000--	IT CANT BE TAKEN. */

L2000:
    if ((sflag & VCBIT) == 0) {
	goto L4000;
    }
L1000:
    if (schlst_(0, 0, play_1.here, 0, 0, obj) <= 0) {
	goto L4000;
    }
    rspsub_(665, odo2);
    return ret_val;
/* TAKEIT, PAGE 3 */

/* OBJECT IS IN THE ROOM, CAN BE TAKEN BY THE PARSER, */
/* AND IS TAKEABLE IN GENERAL.  IT IS NOT A STAR. */
/* TAKING IT SHOULD NOT HAVE SIDE AFFECTS. */
/* IF IT IS INSIDE SOMETHING, THE CONTAINER IS OPEN. */
/* THE FOLLOWING CODE IS LIFTED FROM SUBROUTINE TAKE. */

L3000:
    if (obj != advs_1.avehic[play_1.winner - 1]) {
	goto L3500;
    }
/* 						!TAKE VEHICLE? */
    rspeak_(672);
    return ret_val;

L3500:
    if (x != 0 && objcts_1.oadv[x - 1] == play_1.winner || weight_(0, obj,
	     play_1.winner) + objcts_1.osize[obj - 1] <= state_1.mxload) {
	goto L3700;
    }
    rspeak_(558);
/* 						!TOO BIG. */
    return ret_val;

L3700:
    newsta_(obj, 559, 0, 0, play_1.winner);
/* 						!DO TAKE. */
    objcts_1.oflag2[obj - 1] |= TCHBT;
    scrupd_(objcts_1.ofval[obj - 1]);
    objcts_1.ofval[obj - 1] = 0;

L4000:
    ret_val = TRUE_;
/* 						!SUCCESS. */
    return ret_val;

} /* takeit_ */

/* GWIM- GET WHAT I MEAN IN AMBIGOUS SITUATIONS */

/* DECLARATIONS */

static integer gwim_(sflag, sfw1, sfw2)
integer sflag;
integer sfw1;
integer sfw2;
{
    /* System generated locals */
    integer ret_val;

    /* Local variables */
    integer av;
    integer nobj, robj;
    logical nocare;

/* GWIM, PAGE 2 */

    ret_val = -1;
/* 						!ASSUME LOSE. */
    av = advs_1.avehic[play_1.winner - 1];
    nobj = 0;
    nocare = (sflag & VCBIT) == 0;

/* FIRST SEARCH ADVENTURER */

    if ((sflag & VABIT) != 0) {
	nobj = fwim_(sfw1, sfw2, 0, 0, play_1.winner, nocare);
    }
    if ((sflag & VRBIT) != 0) {
	goto L100;
    }
L50:
    ret_val = nobj;
    return ret_val;

/* ALSO SEARCH ROOM */

L100:
    robj = fwim_(sfw1, sfw2, play_1.here, 0, 0, nocare);
    if (robj < 0) {
	goto L500;
    } else if (robj == 0) {
	goto L50;
    } else {
	goto L200;
    }
/* 						!TEST RESULT. */

/* ROBJ > 0 */

L200:
    if (av == 0 || robj == av || (objcts_1.oflag2[robj - 1] & FINDBT)
	     != 0) {
	goto L300;
    }
    if (objcts_1.ocan[robj - 1] != av) {
	goto L50;
    }
/* 						!UNREACHABLE? TRY NOBJ */
L300:
    if (nobj != 0) {
	return ret_val;
    }
/* 						!IF AMBIGUOUS, RETURN. */
    if (! takeit_(robj, sflag)) {
	return ret_val;
    }
/* 						!IF UNTAKEABLE, RETURN */
    ret_val = robj;
L500:
    return ret_val;

} /* gwim_ */
