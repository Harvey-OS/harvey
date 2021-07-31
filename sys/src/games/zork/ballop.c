/* BALLOP-	BALLOON FUNCTION */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

logical ballop_(arg)
integer arg;
{
    /* System generated locals */
    logical ret_val;

    ret_val = TRUE_;
/* 						!ASSUME WINS. */
    if (arg != 2) {
	goto L200;
    }
/* 						!READOUT? */
    if (prsvec_1.prsa != vindex_1.lookw) {
	goto L10;
    }
/* 						!ONLY PROCESS LOOK. */
    if (findex_1.binff != 0) {
	goto L50;
    }
/* 						!INFLATED? */
    rspeak_(543);
/* 						!NO. */
    goto L100;
L50:
    rspsub_(544, objcts_1.odesc2[findex_1.binff - 1]);
/* 						!YES. */
L100:
    if (findex_1.btief != 0) {
	rspeak_(545);
    }
/* 						!HOOKED? */
    return ret_val;

L200:
    if (arg != 1) {
	goto L500;
    }
/* 						!READIN? */
    if (prsvec_1.prsa != vindex_1.walkw) {
	goto L300;
    }
/* 						!WALK? */
    if (findxt_(prsvec_1.prso, play_1.here)) {
	goto L250;
    }
/* 						!VALID EXIT? */
    rspeak_(546);
/* 						!NO, JOKE. */
    return ret_val;

L250:
    if (findex_1.btief == 0) {
	goto L275;
    }
/* 						!TIED UP? */
    rspeak_(547);
/* 						!YES, JOKE. */
    return ret_val;

L275:
    if (curxt_1.xtype != xpars_1.xnorm) {
	goto L10;
    }
/* 						!NORMAL EXIT? */
    if ((rooms_1.rflag[curxt_1.xroom1 - 1] & RMUNG) == 0) {
	state_1.bloc = curxt_1.xroom1;
    }
L10:
    ret_val = FALSE_;
    return ret_val;

L300:
    if (prsvec_1.prsa != vindex_1.takew || prsvec_1.prso != findex_1.binff) {
	goto L350;
    }
    rspsub_(548, objcts_1.odesc2[findex_1.binff - 1]);
/* 						!RECEP CONT TOO HOT. */
    return ret_val;

L350:
    if (prsvec_1.prsa != vindex_1.putw || prsvec_1.prsi != oindex_1.recep || 
	    qempty_(oindex_1.recep)) {
	goto L10;
    }
    rspeak_(549);
    return ret_val;

L500:
    if (prsvec_1.prsa != vindex_1.unboaw || (rooms_1.rflag[play_1.here - 1] & 
	    RLAND) == 0) {
	goto L600;
    }
    if (findex_1.binff != 0) {
	cevent_1.ctick[cindex_1.cevbal - 1] = 3;
    }
/* 						!HE GOT OUT, START BALLOON. */
    goto L10;

L600:
    if (prsvec_1.prsa != vindex_1.burnw || objcts_1.ocan[prsvec_1.prso - 1] !=
	     oindex_1.recep) {
	goto L700;
    }
    rspsub_(550, objcts_1.odesc2[prsvec_1.prso - 1]);
/* 						!LIGHT FIRE IN RECEP. */
    cevent_1.ctick[cindex_1.cevbrn - 1] = objcts_1.osize[prsvec_1.prso - 1] * 
	    20;
    objcts_1.oflag1[prsvec_1.prso - 1] |= ONBT + FLAMBT + 
	    LITEBT & ~ (TAKEBT + READBT);
    if (findex_1.binff != 0) {
	return ret_val;
    }
    if (! findex_1.blabf) {
	newsta_(oindex_1.blabe, 0, 0, oindex_1.ballo, 0);
    }
    findex_1.blabf = TRUE_;
    findex_1.binff = prsvec_1.prso;
    cevent_1.ctick[cindex_1.cevbal - 1] = 3;
    rspeak_(551);
    return ret_val;

L700:
    if (prsvec_1.prsa == vindex_1.unboaw && findex_1.binff != 0 && (
	    rooms_1.rflag[play_1.here - 1] & RLAND) != 0) {
	cevent_1.ctick[cindex_1.cevbal - 1] = 3;
    }
    goto L10;

} /* ballop_ */
