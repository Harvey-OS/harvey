/* TAKE-- BASIC TAKE SEQUENCE */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

/* TAKE AN OBJECT (FOR VERBS TAKE, PUT, DROP, READ, ETC.) */

logical take_(flg)
logical flg;
{
    /* System generated locals */
    integer i__1;
    logical ret_val;

    /* Local variables */
    integer oa;
    integer x;

    ret_val = FALSE_;
/* 						!ASSUME LOSES. */
    oa = objcts_1.oactio[prsvec_1.prso - 1];
/* 						!GET OBJECT ACTION. */
    if (prsvec_1.prso <= star_1.strbit) {
	goto L100;
    }
/* 						!STAR? */
    ret_val = objact_();
/* 						!YES, LET IT HANDLE. */
    return ret_val;

L100:
    x = objcts_1.ocan[prsvec_1.prso - 1];
/* 						!INSIDE? */
    if (prsvec_1.prso != advs_1.avehic[play_1.winner - 1]) {
	goto L400;
    }
/* 						!HIS VEHICLE? */
    rspeak_(672);
/* 						!DUMMY. */
    return ret_val;

L400:
    if ((objcts_1.oflag1[prsvec_1.prso - 1] & TAKEBT) != 0) {
	goto L500;
    }
    if (! oappli_(oa, 0)) {
	i__1 = rnd_(5) + 552;
	rspeak_(i__1);
    }
    return ret_val;

/* OBJECT IS TAKEABLE AND IN POSITION TO BE TAKEN. */

L500:
    if (x != 0 || qhere_(prsvec_1.prso, play_1.here)) {
	goto L600;
    }
    if (objcts_1.oadv[prsvec_1.prso - 1] == play_1.winner) {
	rspeak_(557);
    }
/* 						!ALREADY GOT IT? */
    return ret_val;

L600:
    if (x != 0 && objcts_1.oadv[x - 1] == play_1.winner || weight_(0,
	    prsvec_1.prso, play_1.winner) + objcts_1.osize[prsvec_1.prso - 1]
	     <= state_1.mxload) {
	goto L700;
    }
    rspeak_(558);
/* 						!TOO MUCH WEIGHT. */
    return ret_val;

L700:
    ret_val = TRUE_;
/* 						!AT LAST. */
    if (oappli_(oa, 0)) {
	return ret_val;
    }
/* 						!DID IT HANDLE? */
    newsta_(prsvec_1.prso, 0, 0, 0, play_1.winner);
/* 						!TAKE OBJECT FOR WINNER. */
    objcts_1.oflag2[prsvec_1.prso - 1] |= TCHBT;
    scrupd_(objcts_1.ofval[prsvec_1.prso - 1]);
/* 						!UPDATE SCORE. */
    objcts_1.ofval[prsvec_1.prso - 1] = 0;
/* 						!CANT BE SCORED AGAIN. */
    if (flg) {
	rspeak_(559);
    }
/* 						!TELL TAKEN. */
    return ret_val;

} /* take_ */

/* DROP- DROP VERB PROCESSOR */

/* DECLARATIONS */

logical drop_(z)
logical z;
{
    /* System generated locals */
    logical ret_val;

    /* Local variables */
    logical f;
    integer i, x;

    ret_val = TRUE_;
/* 						!ASSUME WINS. */
    x = objcts_1.ocan[prsvec_1.prso - 1];
/* 						!GET CONTAINER. */
    if (x == 0) {
	goto L200;
    }
/* 						!IS IT INSIDE? */
    if (objcts_1.oadv[x - 1] != play_1.winner) {
	goto L1000;
    }
/* 						!IS HE CARRYING CON? */
    if ((objcts_1.oflag2[x - 1] & OPENBT) != 0) {
	goto L300;
    }
    rspsub_(525, objcts_1.odesc2[x - 1]);
/* 						!CANT REACH. */
    return ret_val;

L200:
    if (objcts_1.oadv[prsvec_1.prso - 1] != play_1.winner) {
	goto L1000;
    }
/* 						!IS HE CARRYING OBJ? */
L300:
    if (advs_1.avehic[play_1.winner - 1] == 0) {
	goto L400;
    }
/* 						!IS HE IN VEHICLE? */
    prsvec_1.prsi = advs_1.avehic[play_1.winner - 1];
/* 						!YES, */
    f = put_(1);
/* 						!DROP INTO VEHICLE. */
    prsvec_1.prsi = 0;
/* 						!DISARM PARSER. */
    return ret_val;
/* 						!DONE. */

L400:
    newsta_(prsvec_1.prso, 0, play_1.here, 0, 0);
/* 						!DROP INTO ROOM. */
    if (play_1.here == rindex_1.mtree) {
	newsta_(prsvec_1.prso, 0, rindex_1.fore3, 0, 0);
    }
    scrupd_(objcts_1.ofval[prsvec_1.prso - 1]);
/* 						!SCORE OBJECT. */
    objcts_1.ofval[prsvec_1.prso - 1] = 0;
/* 						!CANT BE SCORED AGAIN. */
    objcts_1.oflag2[prsvec_1.prso - 1] |= TCHBT;

    if (objact_()) {
	return ret_val;
    }
/* 						!DID IT HANDLE? */
    i = 0;
/* 						!ASSUME NOTHING TO SAY. */
    if (prsvec_1.prsa == vindex_1.dropw) {
	i = 528;
    }
    if (prsvec_1.prsa == vindex_1.throww) {
	i = 529;
    }
    if (i != 0 && play_1.here == rindex_1.mtree) {
	i = 659;
    }
    rspsub_(i, objcts_1.odesc2[prsvec_1.prso - 1]);
    return ret_val;

L1000:
    rspeak_(527);
/* 						!DONT HAVE IT. */
    return ret_val;

} /* drop_ */

/* PUT- PUT VERB PROCESSOR */

/* DECLARATIONS */

logical put_(flg)
logical flg;
{
    /* System generated locals */
    logical ret_val;

    /* Local variables */
    integer j;
    integer svi, svo;

    ret_val = FALSE_;
    if (prsvec_1.prso <= star_1.strbit && prsvec_1.prsi <= star_1.strbit) {
	goto L200;
    }
    if (! objact_()) {
	rspeak_(560);
    }
/* 						!STAR */
    ret_val = TRUE_;
    return ret_val;

L200:
    if ((objcts_1.oflag2[prsvec_1.prsi - 1] & OPENBT) != 0 || (
	    objcts_1.oflag1[prsvec_1.prsi - 1] & DOORBT + 
	    CONTBT) != 0 || (objcts_1.oflag2[prsvec_1.prsi - 1] & 
	    VEHBT) != 0) {
	goto L300;
    }
    rspeak_(561);
/* 						!CANT PUT IN THAT. */
    return ret_val;

L300:
    if ((objcts_1.oflag2[prsvec_1.prsi - 1] & OPENBT) != 0) {
	goto L400;
    }
/* 						!IS IT OPEN? */
    rspeak_(562);
/* 						!NO, JOKE */
    return ret_val;

L400:
    if (prsvec_1.prso != prsvec_1.prsi) {
	goto L500;
    }
/* 						!INTO ITSELF? */
    rspeak_(563);
/* 						!YES, JOKE. */
    return ret_val;

L500:
    if (objcts_1.ocan[prsvec_1.prso - 1] != prsvec_1.prsi) {
	goto L600;
    }
/* 						!ALREADY INSIDE. */
    rspsb2_(564, objcts_1.odesc2[prsvec_1.prso - 1], objcts_1.odesc2[
	    prsvec_1.prsi - 1]);
    ret_val = TRUE_;
    return ret_val;

L600:
    if (weight_(0, prsvec_1.prso, 0) + weight_(0, prsvec_1.prsi,
	     0) + objcts_1.osize[prsvec_1.prso - 1] <= objcts_1.ocapac[
	    prsvec_1.prsi - 1]) {
	goto L700;
    }
    rspeak_(565);
/* 						!THEN CANT DO IT. */
    return ret_val;

/* NOW SEE IF OBJECT (OR ITS CONTAINER) IS IN ROOM */

L700:
    j = prsvec_1.prso;
/* 						!START SEARCH. */
L725:
    if (qhere_(j, play_1.here)) {
	goto L750;
    }
/* 						!IS IT HERE? */
    j = objcts_1.ocan[j - 1];
    if (j != 0) {
	goto L725;
    }
/* 						!MORE TO DO? */
    goto L800;
/* 						!NO, SCH FAILS. */

L750:
    svo = prsvec_1.prso;
/* 						!SAVE PARSER. */
    svi = prsvec_1.prsi;
    prsvec_1.prsa = vindex_1.takew;
    prsvec_1.prsi = 0;
    if (! take_(0)) {
	return ret_val;
    }
/* 						!TAKE OBJECT. */
    prsvec_1.prsa = vindex_1.putw;
    prsvec_1.prso = svo;
    prsvec_1.prsi = svi;
    goto L1000;

/* NOW SEE IF OBJECT IS ON PERSON. */

L800:
    if (objcts_1.ocan[prsvec_1.prso - 1] == 0) {
	goto L1000;
    }
/* 						!INSIDE? */
    if ((objcts_1.oflag2[objcts_1.ocan[prsvec_1.prso - 1] - 1] & 
	    OPENBT) != 0) {
	goto L900;
    }
/* 						!OPEN? */
    rspsub_(566, objcts_1.odesc2[prsvec_1.prso - 1]);
/* 						!LOSE. */
    return ret_val;

L900:
    scrupd_(objcts_1.ofval[prsvec_1.prso - 1]);
/* 						!SCORE OBJECT. */
    objcts_1.ofval[prsvec_1.prso - 1] = 0;
    objcts_1.oflag2[prsvec_1.prso - 1] |= TCHBT;
    newsta_(prsvec_1.prso, 0, 0, 0, play_1.winner);
/* 						!TEMPORARILY ON WINNER. */

L1000:
    if (objact_()) {
	return ret_val;
    }
/* 						!NO, GIVE OBJECT A SHOT. */
    newsta_(prsvec_1.prso, 2, 0, prsvec_1.prsi, 0);
/* 						!CONTAINED INSIDE. */
    ret_val = TRUE_;
    return ret_val;

} /* put_ */

/* VALUAC- HANDLES VALUABLES/EVERYTHING */

void valuac_(v)
integer v;
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    logical f;
    integer i;
    logical f1;
    integer savep, saveh;

    f = TRUE_;
/* 						!ASSUME NO ACTIONS. */
    i = 579;
/* 						!ASSUME NOT LIT. */
    if (! lit_(play_1.here)) {
	goto L4000;
    }
/* 						!IF NOT LIT, PUNT. */
    i = 677;
/* 						!ASSUME WRONG VERB. */
    savep = prsvec_1.prso;
/* 						!SAVE PRSO. */
    saveh = play_1.here;
/* 						!SAVE HERE. */

/* L100: */
    if (prsvec_1.prsa != vindex_1.takew) {
	goto L1000;
    }
/* 						!TAKE EVERY/VALUA? */
    i__1 = objcts_1.olnt;
    for (prsvec_1.prso = 1; prsvec_1.prso <= i__1; ++prsvec_1.prso) {
/* 						!LOOP THRU OBJECTS. */
	if (! qhere_(prsvec_1.prso, play_1.here) || (objcts_1.oflag1[
		prsvec_1.prso - 1] & VISIBT) == 0 || (
		objcts_1.oflag2[prsvec_1.prso - 1] & ACTRBT) != 0 || 
		savep == v && objcts_1.otval[prsvec_1.prso - 1] <= 0) {
	    goto L500;
	}
	if ((objcts_1.oflag1[prsvec_1.prso - 1] & TAKEBT) == 0 && (
		objcts_1.oflag2[prsvec_1.prso - 1] & TRYBT) == 0) {
	    goto L500;
	}
	f = FALSE_;
	rspsub_(580, objcts_1.odesc2[prsvec_1.prso - 1]);
	f1 = take_(1);
	if (saveh != play_1.here) {
	    return;
	}
L500:
	;
    }
    goto L3000;

L1000:
    if (prsvec_1.prsa != vindex_1.dropw) {
	goto L2000;
    }
/* 						!DROP EVERY/VALUA? */
    i__1 = objcts_1.olnt;
    for (prsvec_1.prso = 1; prsvec_1.prso <= i__1; ++prsvec_1.prso) {
	if (objcts_1.oadv[prsvec_1.prso - 1] != play_1.winner || savep == v 
		&& objcts_1.otval[prsvec_1.prso - 1] <= 0) {
	    goto L1500;
	}
	f = FALSE_;
	rspsub_(580, objcts_1.odesc2[prsvec_1.prso - 1]);
	f1 = drop_(1);
	if (saveh != play_1.here) {
	    return;
	}
L1500:
	;
    }
    goto L3000;

L2000:
    if (prsvec_1.prsa != vindex_1.putw) {
	goto L3000;
    }
/* 						!PUT EVERY/VALUA? */
    i__1 = objcts_1.olnt;
    for (prsvec_1.prso = 1; prsvec_1.prso <= i__1; ++prsvec_1.prso) {
/* 						!LOOP THRU OBJECTS. */
	if (objcts_1.oadv[prsvec_1.prso - 1] != play_1.winner || 
		prsvec_1.prso == prsvec_1.prsi || savep == v && 
		objcts_1.otval[prsvec_1.prso - 1] <= 0 || (objcts_1.oflag1[
		prsvec_1.prso - 1] & VISIBT) == 0) {
	    goto L2500;
	}
	f = FALSE_;
	rspsub_(580, objcts_1.odesc2[prsvec_1.prso - 1]);
	f1 = put_(1);
	if (saveh != play_1.here) {
	    return;
	}
L2500:
	;
    }

L3000:
    i = 581;
    if (savep == v) {
	i = 582;
    }
/* 						!CHOOSE MESSAGE. */
L4000:
    if (f) {
	rspeak_(i);
    }
/* 						!IF NOTHING, REPORT. */
} /* valuac_ */
