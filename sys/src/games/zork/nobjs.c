/* NOBJS-	NEW OBJECTS PROCESSOR */
/* 	OBJECTS IN THIS MODULE CANNOT CALL RMINFO, JIGSUP, */
/* 	MAJOR VERBS, OR OTHER NON-RESIDENT SUBROUTINES */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

static logical mirpan_ P((integer, logical));

logical nobjs_(ri, arg)
integer ri;
integer arg;
{
    /* System generated locals */
    integer i__1, i__2;
    logical ret_val;

    /* Local variables */
    logical f;
    integer target;
    integer i;
    integer j;
    integer av, wl;
    integer nxt, odi2 = 0, odo2 = 0;

    if (prsvec_1.prso != 0) {
	odo2 = objcts_1.odesc2[prsvec_1.prso - 1];
    }
    if (prsvec_1.prsi != 0) {
	odi2 = objcts_1.odesc2[prsvec_1.prsi - 1];
    }
    av = advs_1.avehic[play_1.winner - 1];
    ret_val = TRUE_;

    switch (ri - 31) {
	case 1:  goto L1000;
	case 2:  goto L2000;
	case 3:  goto L3000;
	case 4:  goto L4000;
	case 5:  goto L5000;
	case 6:  goto L6000;
	case 7:  goto L7000;
	case 8:  goto L8000;
	case 9:  goto L9000;
	case 10:  goto L10000;
	case 11:  goto L11000;
	case 12:  goto L12000;
	case 13:  goto L13000;
	case 14:  goto L14000;
	case 15:  goto L15000;
	case 16:  goto L16000;
	case 17:  goto L17000;
	case 18:  goto L18000;
	case 19:  goto L19000;
	case 20:  goto L20000;
	case 21:  goto L21000;
    }
    bug_(6, ri);

/* RETURN HERE TO DECLARE FALSE RESULT */

L10:
    ret_val = FALSE_;
    return ret_val;

/* O32--	BILLS */

L1000:
    if (prsvec_1.prsa != vindex_1.eatw) {
	goto L1100;
    }
/* 						!EAT? */
    rspeak_(639);
/* 						!JOKE. */
    return ret_val;

L1100:
    if (prsvec_1.prsa == vindex_1.burnw) {
	rspeak_(640);
    }
/* 						!BURN?  JOKE. */
    goto L10;
/* 						!LET IT BE HANDLED. */
/* NOBJS, PAGE 3 */

/* O33--	SCREEN OF LIGHT */

L2000:
    target = oindex_1.scol;
/* 						!TARGET IS SCOL. */
L2100:
    if (prsvec_1.prso != target) {
	goto L2400;
    }
/* 						!PRSO EQ TARGET? */
    if (prsvec_1.prsa != vindex_1.pushw && prsvec_1.prsa != vindex_1.movew && 
	    prsvec_1.prsa != vindex_1.takew && prsvec_1.prsa != vindex_1.rubw)
	     {
	goto L2200;
    }
    rspeak_(673);
/* 						!HAND PASSES THRU. */
    return ret_val;

L2200:
    if (prsvec_1.prsa != vindex_1.killw && prsvec_1.prsa != vindex_1.attacw &&
	     prsvec_1.prsa != vindex_1.mungw) {
	goto L2400;
    }
    rspsub_(674, odi2);
/* 						!PASSES THRU. */
    return ret_val;

L2400:
    if (prsvec_1.prsa != vindex_1.throww || prsvec_1.prsi != target) {
	goto L10;
    }
    if (play_1.here == rindex_1.bkbox) {
	goto L2600;
    }
/* 						!THRU SCOL? */
    newsta_(prsvec_1.prso, 0, rindex_1.bkbox, 0, 0);
/* 						!NO, THRU WALL. */
    rspsub_(675, odo2);
/* 						!ENDS UP IN BOX ROOM. */
    cevent_1.ctick[cindex_1.cevscl - 1] = 0;
/* 						!CANCEL ALARM. */
    screen_1.scolrm = 0;
/* 						!RESET SCOL ROOM. */
    return ret_val;

L2600:
    if (screen_1.scolrm == 0) {
	goto L2900;
    }
/* 						!TRIED TO GO THRU? */
    newsta_(prsvec_1.prso, 0, screen_1.scolrm, 0, 0);
/* 						!SUCCESS. */
    rspsub_(676, odo2);
/* 						!ENDS UP SOMEWHERE. */
    cevent_1.ctick[cindex_1.cevscl - 1] = 0;
/* 						!CANCEL ALARM. */
    screen_1.scolrm = 0;
/* 						!RESET SCOL ROOM. */
    return ret_val;

L2900:
    rspeak_(213);
/* 						!CANT DO IT. */
    return ret_val;
/* NOBJS, PAGE 4 */

/* O34--	GNOME OF ZURICH */

L3000:
    if (prsvec_1.prsa != vindex_1.givew && prsvec_1.prsa != vindex_1.throww) {

	goto L3200;
    }
    if (objcts_1.otval[prsvec_1.prso - 1] != 0) {
	goto L3100;
    }
/* 						!THROW A TREASURE? */
    newsta_(prsvec_1.prso, 641, 0, 0, 0);
/* 						!NO, GO POP. */
    return ret_val;

L3100:
    newsta_(prsvec_1.prso, 0, 0, 0, 0);
/* 						!YES, BYE BYE TREASURE. */
    rspsub_(642, odo2);
    newsta_(oindex_1.zgnom, 0, 0, 0, 0);
/* 						!BYE BYE GNOME. */
    cevent_1.ctick[cindex_1.cevzgo - 1] = 0;
/* 						!CANCEL EXIT. */
    f = moveto_(rindex_1.bkent, play_1.winner);
/* 						!NOW IN BANK ENTRANCE. */
    return ret_val;

L3200:
    if (prsvec_1.prsa != vindex_1.attacw && prsvec_1.prsa != vindex_1.killw &&
	     prsvec_1.prsa != vindex_1.mungw) {
	goto L3300;
    }
    newsta_(oindex_1.zgnom, 643, 0, 0, 0);
/* 						!VANISH GNOME. */
    cevent_1.ctick[cindex_1.cevzgo - 1] = 0;
/* 						!CANCEL EXIT. */
    return ret_val;

L3300:
    rspeak_(644);
/* 						!GNOME IS IMPATIENT. */
    return ret_val;

/* O35--	EGG */

L4000:
    if (prsvec_1.prsa != vindex_1.openw || prsvec_1.prso != oindex_1.egg) {
	goto L4500;
    }
    if (! ((objcts_1.oflag2[oindex_1.egg - 1] & OPENBT) != 0)) {
	goto L4100;
    }
/* 						!OPEN ALREADY? */
    rspeak_(649);
/* 						!YES. */
    return ret_val;

L4100:
    if (prsvec_1.prsi != 0) {
	goto L4200;
    }
/* 						!WITH SOMETHING? */
    rspeak_(650);
/* 						!NO, CANT. */
    return ret_val;

L4200:
    if (prsvec_1.prsi != oindex_1.hands) {
	goto L4300;
    }
/* 						!WITH HANDS? */
    rspeak_(651);
/* 						!NOT RECOMMENDED. */
    return ret_val;

L4300:
    i = 652;
/* 						!MUNG MESSAGE. */
    if ((objcts_1.oflag1[prsvec_1.prsi - 1] & TOOLBT) != 0 || (
	    objcts_1.oflag2[prsvec_1.prsi - 1] & WEAPBT) != 0) {
	goto L4600;
    }
    i = 653;
/* 						!NOVELTY 1. */
    if ((objcts_1.oflag2[prsvec_1.prso - 1] & FITEBT) != 0) {
	i = 654;
    }
    objcts_1.oflag2[prsvec_1.prso - 1] |= FITEBT;
    rspsub_(i, odi2);
    return ret_val;

L4500:
    if (prsvec_1.prsa != vindex_1.openw && prsvec_1.prsa != vindex_1.mungw) {
	goto L4800;
    }
    i = 655;
/* 						!YOU BLEW IT. */
L4600:
    newsta_(oindex_1.begg, i, objcts_1.oroom[oindex_1.egg - 1], 
	    objcts_1.ocan[oindex_1.egg - 1], objcts_1.oadv[oindex_1.egg - 1])
	    ;
    newsta_(oindex_1.egg, 0, 0, 0, 0);
/* 						!VANISH EGG. */
    objcts_1.otval[oindex_1.begg - 1] = 2;
/* 						!BAD EGG HAS VALUE. */
    if (objcts_1.ocan[oindex_1.canar - 1] != oindex_1.egg) {
	goto L4700;
    }
/* 						!WAS CANARY INSIDE? */
    rspeak_(objcts_1.odesco[oindex_1.bcana - 1]);
/* 						!YES, DESCRIBE RESULT. */
    objcts_1.otval[oindex_1.bcana - 1] = 1;
    return ret_val;

L4700:
    newsta_(oindex_1.bcana, 0, 0, 0, 0);
/* 						!NO, VANISH IT. */
    return ret_val;

L4800:
    if (prsvec_1.prsa != vindex_1.dropw || play_1.here != rindex_1.mtree) {
	goto L10;
    }
    newsta_(oindex_1.begg, 658, rindex_1.fore3, 0, 0);
/* 						!DROPPED EGG. */
    newsta_(oindex_1.egg, 0, 0, 0, 0);
    objcts_1.otval[oindex_1.begg - 1] = 2;
    if (objcts_1.ocan[oindex_1.canar - 1] != oindex_1.egg) {
	goto L4700;
    }
    objcts_1.otval[oindex_1.bcana - 1] = 1;
/* 						!BAD CANARY. */
    return ret_val;
/* NOBJS, PAGE 5 */

/* O36--	CANARIES, GOOD AND BAD */

L5000:
    if (prsvec_1.prsa != vindex_1.windw) {
	goto L10;
    }
/* 						!WIND EM UP? */
    if (prsvec_1.prso == oindex_1.canar) {
	goto L5100;
    }
/* 						!RIGHT ONE? */
    rspeak_(645);
/* 						!NO, BAD NEWS. */
    return ret_val;

L5100:
    if (! findex_1.singsf && (play_1.here == rindex_1.mtree || play_1.here >= 
	    rindex_1.fore1 && play_1.here < rindex_1.clear)) {
	goto L5200;
    }
    rspeak_(646);
/* 						!NO, MEDIOCRE NEWS. */
    return ret_val;

L5200:
    findex_1.singsf = TRUE_;
/* 						!SANG SONG. */
    i = play_1.here;
    if (i == rindex_1.mtree) {
	i = rindex_1.fore3;
    }
/* 						!PLACE BAUBLE. */
    newsta_(oindex_1.baubl, 647, i, 0, 0);
    return ret_val;

/* O37--	WHITE CLIFFS */

L6000:
    if (prsvec_1.prsa != vindex_1.clmbw && prsvec_1.prsa != vindex_1.clmbuw &&
	     prsvec_1.prsa != vindex_1.clmbdw) {
	goto L10;
    }
    rspeak_(648);
/* 						!OH YEAH? */
    return ret_val;

/* O38--	WALL */

L7000:
    if ((i__1 = play_1.here - findex_1.mloc, abs(i__1)) != 1 || mrhere_(
	    play_1.here) != 0 || prsvec_1.prsa != vindex_1.pushw) {
	goto L7100;
    }
    rspeak_(860);
/* 						!PUSHED MIRROR WALL. */
    return ret_val;

L7100:
    if ((rooms_1.rflag[play_1.here - 1] & RNWALL) == 0) {
	goto L10;
    }
    rspeak_(662);
/* 						!NO WALL. */
    return ret_val;
/* NOBJS, PAGE 6 */

/* O39--	SONG BIRD GLOBAL */

L8000:
    if (prsvec_1.prsa != vindex_1.findw) {
	goto L8100;
    }
/* 						!FIND? */
    rspeak_(666);
    return ret_val;

L8100:
    if (prsvec_1.prsa != vindex_1.examiw) {
	goto L10;
    }
/* 						!EXAMINE? */
    rspeak_(667);
    return ret_val;

/* O40--	PUZZLE/SCOL WALLS */

L9000:
    if (play_1.here != rindex_1.cpuzz) {
	goto L9500;
    }
/* 						!PUZZLE WALLS? */
    if (prsvec_1.prsa != vindex_1.pushw) {
	goto L10;
    }
/* 						!PUSH? */
    for (i = 1; i <= 8; i += 2) {
/* 						!LOCATE WALL. */
	if (prsvec_1.prso == puzzle_1.cpwl[i - 1]) {
	    goto L9200;
	}
/* L9100: */
    }
    bug_(80, prsvec_1.prso);
/* 						!WHAT? */

L9200:
    j = puzzle_1.cpwl[i];
/* 						!GET DIRECTIONAL OFFSET. */
    nxt = findex_1.cphere + j;
/* 						!GET NEXT STATE. */
    wl = puzzle_1.cpvec[nxt - 1];
/* 						!GET C(NEXT STATE). */
    switch (wl + 4) {
	case 1:  goto L9300;
	case 2:  goto L9300;
	case 3:  goto L9300;
	case 4:  goto L9250;
	case 5:  goto L9350;
    }
/* 						!PROCESS. */

L9250:
    rspeak_(876);
/* 						!CLEAR CORRIDOR. */
    return ret_val;

L9300:
    if (puzzle_1.cpvec[nxt + j - 1] == 0) {
	goto L9400;
    }
/* 						!MOVABLE, ROOM TO MOVE? */
L9350:
    rspeak_(877);
/* 						!IMMOVABLE, NO ROOM. */
    return ret_val;

L9400:
    i = 878;
/* 						!ASSUME FIRST PUSH. */
    if (findex_1.cpushf) {
	i = 879;
    }
/* 						!NOT? */
    findex_1.cpushf = TRUE_;
    puzzle_1.cpvec[nxt + j - 1] = wl;
/* 						!MOVE WALL. */
    puzzle_1.cpvec[nxt - 1] = 0;
/* 						!VACATE NEXT STATE. */
    cpgoto_(nxt);
/* 						!ONWARD. */
    cpinfo_(i, nxt);
/* 						!DESCRIBE. */
    princr_(1, play_1.here);
/* 						!PRINT ROOMS CONTENTS. */
    rooms_1.rflag[play_1.here - 1] |= RSEEN;
    return ret_val;

L9500:
    if (play_1.here != screen_1.scolac) {
	goto L9700;
    }
/* 						!IN SCOL ACTIVE ROOM? */
    for (i = 1; i <= 12; i += 3) {
	target = screen_1.scolwl[i];
/* 						!ASSUME TARGET. */
	if (screen_1.scolwl[i - 1] == play_1.here) {
	    goto L2100;
	}
/* 						!TREAT IF FOUND. */
/* L9600: */
    }

L9700:
    if (play_1.here != rindex_1.bkbox) {
	goto L10;
    }
/* 						!IN BOX ROOM? */
    target = oindex_1.wnort;
    goto L2100;
/* NOBJS, PAGE 7 */

/* O41--	SHORT POLE */

L10000:
    if (prsvec_1.prsa != vindex_1.raisew) {
	goto L10100;
    }
/* 						!LIFT? */
    i = 749;
/* 						!ASSUME UP. */
    if (findex_1.poleuf == 2) {
	i = 750;
    }
/* 						!ALREADY UP? */
    rspeak_(i);
    findex_1.poleuf = 2;
/* 						!POLE IS RAISED. */
    return ret_val;

L10100:
    if (prsvec_1.prsa != vindex_1.lowerw && prsvec_1.prsa != vindex_1.pushw) {

	goto L10;
    }
    if (findex_1.poleuf != 0) {
	goto L10200;
    }
/* 						!ALREADY LOWERED? */
    rspeak_(751);
/* 						!CANT DO IT. */
    return ret_val;

L10200:
    if (findex_1.mdir % 180 != 0) {
	goto L10300;
    }
/* 						!MIRROR N-S? */
    findex_1.poleuf = 0;
/* 						!YES, LOWER INTO */
    rspeak_(752);
/* 						!CHANNEL. */
    return ret_val;

L10300:
    if (findex_1.mdir != 270 || findex_1.mloc != rindex_1.mrb) {
	goto L10400;
    }
    findex_1.poleuf = 0;
/* 						!LOWER INTO HOLE. */
    rspeak_(753);
    return ret_val;

L10400:
    i__1 = findex_1.poleuf + 753;
    rspeak_(i__1);
/* 						!POLEUF = 1 OR 2. */
    findex_1.poleuf = 1;
/* 						!NOW ON FLOOR. */
    return ret_val;

/* O42--	MIRROR SWITCH */

L11000:
    if (prsvec_1.prsa != vindex_1.pushw) {
	goto L10;
    }
/* 						!PUSH? */
    if (findex_1.mrpshf) {
	goto L11300;
    }
/* 						!ALREADY PUSHED? */
    rspeak_(756);
/* 						!BUTTON GOES IN. */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!BLOCKED? */
	if (qhere_(i, rindex_1.mreye) && i != oindex_1.rbeam) {
	    goto L11200;
	}
/* L11100: */
    }
    rspeak_(757);
/* 						!NOTHING IN BEAM. */
    return ret_val;

L11200:
    cevent_1.cflag[cindex_1.cevmrs - 1] = TRUE_;
/* 						!MIRROR OPENS. */
    cevent_1.ctick[cindex_1.cevmrs - 1] = 7;
    findex_1.mrpshf = TRUE_;
    findex_1.mropnf = TRUE_;
    return ret_val;

L11300:
    rspeak_(758);
/* 						!MIRROR ALREADYOPEN. */
    return ret_val;
/* NOBJS, PAGE 8 */

/* O43--	BEAM FUNCTION */

L12000:
    if (prsvec_1.prsa != vindex_1.takew || prsvec_1.prso != oindex_1.rbeam) {
	goto L12100;
    }
    rspeak_(759);
/* 						!TAKE BEAM, JOKE. */
    return ret_val;

L12100:
    i = prsvec_1.prso;
/* 						!ASSUME BLK WITH DIROBJ. */
    if (prsvec_1.prsa == vindex_1.putw && prsvec_1.prsi == oindex_1.rbeam) {
	goto L12200;
    }
    if (prsvec_1.prsa != vindex_1.mungw || prsvec_1.prso != oindex_1.rbeam || 
	    prsvec_1.prsi == 0) {
	goto L10;
    }
    i = prsvec_1.prsi;
L12200:
    if (objcts_1.oadv[i - 1] != play_1.winner) {
	goto L12300;
    }
/* 						!CARRYING? */
    newsta_(i, 0, play_1.here, 0, 0);
/* 						!DROP OBJ. */
    rspsub_(760, objcts_1.odesc2[i - 1]);
    return ret_val;

L12300:
    j = 761;
/* 						!ASSUME NOT IN ROOM. */
    if (qhere_(j, play_1.here)) {
	i = 762;
    }
/* 						!IN ROOM? */
    rspsub_(j, objcts_1.odesc2[i - 1]);
/* 						!DESCRIBE. */
    return ret_val;

/* O44--	BRONZE DOOR */

L13000:
    if (play_1.here == rindex_1.ncell || findex_1.lcell == 4 && (play_1.here 
	    == rindex_1.cell || play_1.here == rindex_1.scorr)) {
	goto L13100;
    }
    rspeak_(763);
/* 						!DOOR NOT THERE. */
    return ret_val;

L13100:
    if (! opncls_(oindex_1.odoor, 764, 765)) {
	goto L10;
    }
/* 						!OPEN/CLOSE? */
    if (play_1.here == rindex_1.ncell && (objcts_1.oflag2[oindex_1.odoor - 1] 
	    & OPENBT) != 0) {
	rspeak_(766);
    }
    return ret_val;

/* O45--	QUIZ DOOR */

L14000:
    if (prsvec_1.prsa != vindex_1.openw && prsvec_1.prsa != vindex_1.closew) {

	goto L14100;
    }
    rspeak_(767);
/* 						!DOOR WONT MOVE. */
    return ret_val;

L14100:
    if (prsvec_1.prsa != vindex_1.knockw) {
	goto L10;
    }
/* 						!KNOCK? */
    if (findex_1.inqstf) {
	goto L14200;
    }
/* 						!TRIED IT ALREADY? */
    findex_1.inqstf = TRUE_;
/* 						!START INQUISITION. */
    cevent_1.cflag[cindex_1.cevinq - 1] = TRUE_;
    cevent_1.ctick[cindex_1.cevinq - 1] = 2;
    findex_1.quesno = rnd_(8);
/* 						!SELECT QUESTION. */
    findex_1.nqatt = 0;
    findex_1.corrct = 0;
    rspeak_(768);
/* 						!ANNOUNCE RULES. */
    rspeak_(769);
    i__1 = findex_1.quesno + 770;
    rspeak_(i__1);
/* 						!ASK QUESTION. */
    return ret_val;

L14200:
    rspeak_(798);
/* 						!NO REPLY. */
    return ret_val;

/* O46--	LOCKED DOOR */

L15000:
    if (prsvec_1.prsa != vindex_1.openw) {
	goto L10;
    }
/* 						!OPEN? */
    rspeak_(778);
/* 						!CANT. */
    return ret_val;

/* O47--	CELL DOOR */

L16000:
    ret_val = opncls_(oindex_1.cdoor, 779, 780);
/* 						!OPEN/CLOSE? */
    return ret_val;
/* NOBJS, PAGE 9 */

/* O48--	DIALBUTTON */

L17000:
    if (prsvec_1.prsa != vindex_1.pushw) {
	goto L10;
    }
/* 						!PUSH? */
    rspeak_(809);
/* 						!CLICK. */
    if ((objcts_1.oflag2[oindex_1.cdoor - 1] & OPENBT) != 0) {
	rspeak_(810);
    }
/* 						!CLOSE CELL DOOR. */

    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!RELOCATE OLD TO HYPER. */
	if (objcts_1.oroom[i - 1] == rindex_1.cell && (objcts_1.oflag1[i - 1] 
		& DOORBT) == 0) {
	    i__2 = findex_1.lcell * hyper_1.hfactr;
	    newsta_(i, 0, i__2, 0, 0);
	}
	if (objcts_1.oroom[i - 1] == findex_1.pnumb * hyper_1.hfactr) {
	    newsta_(i, 0, rindex_1.cell, 0, 0);
	}
/* L17100: */
    }

    objcts_1.oflag2[oindex_1.odoor - 1] &= ~ OPENBT;
    objcts_1.oflag2[oindex_1.cdoor - 1] &= ~ OPENBT;
    objcts_1.oflag1[oindex_1.odoor - 1] &= ~ VISIBT;
    if (findex_1.pnumb == 4) {
	objcts_1.oflag1[oindex_1.odoor - 1] |= VISIBT;
    }

    if (advs_1.aroom[aindex_1.player - 1] != rindex_1.cell) {
	goto L17400;
    }
/* 						!PLAYER IN CELL? */
    if (findex_1.lcell != 4) {
	goto L17200;
    }
/* 						!IN RIGHT CELL? */
    objcts_1.oflag1[oindex_1.odoor - 1] |= VISIBT;
    f = moveto_(rindex_1.ncell, aindex_1.player);
/* 						!YES, MOVETO NCELL. */
    goto L17400;
L17200:
    f = moveto_(rindex_1.pcell, aindex_1.player);
/* 						!NO, MOVETO PCELL. */

L17400:
    findex_1.lcell = findex_1.pnumb;
    return ret_val;
/* NOBJS, PAGE 10 */

/* O49--	DIAL INDICATOR */

L18000:
    if (prsvec_1.prsa != vindex_1.spinw) {
	goto L18100;
    }
/* 						!SPIN? */
    findex_1.pnumb = rnd_(8) + 1;
/* 						!WHEE */
/* 						! */
    i__1 = findex_1.pnumb + 712;
    rspsub_(797, i__1);
    return ret_val;

L18100:
    if (prsvec_1.prsa != vindex_1.movew && prsvec_1.prsa != vindex_1.putw && 
	    prsvec_1.prsa != vindex_1.trntow) {
	goto L10;
    }
    if (prsvec_1.prsi != 0) {
	goto L18200;
    }
/* 						!TURN DIAL TO X? */
    rspeak_(806);
/* 						!MUST SPECIFY. */
    return ret_val;

L18200:
    if (prsvec_1.prsi >= oindex_1.num1 && prsvec_1.prsi <= oindex_1.num8) {
	goto L18300;
    }
    rspeak_(807);
/* 						!MUST BE DIGIT. */
    return ret_val;

L18300:
    findex_1.pnumb = prsvec_1.prsi - oindex_1.num1 + 1;
/* 						!SET UP NEW. */
    i__1 = findex_1.pnumb + 712;
    rspsub_(808, i__1);
    return ret_val;

/* O50--	GLOBAL MIRROR */

L19000:
    ret_val = mirpan_(832, 0);
    return ret_val;

/* O51--	GLOBAL PANEL */

L20000:
    if (play_1.here != rindex_1.fdoor) {
	goto L20100;
    }
/* 						!AT FRONT DOOR? */
    if (prsvec_1.prsa != vindex_1.openw && prsvec_1.prsa != vindex_1.closew) {

	goto L10;
    }
    rspeak_(843);
/* 						!PANEL IN DOOR, NOGO. */
    return ret_val;

L20100:
    ret_val = mirpan_(838, 1);
    return ret_val;

/* O52--	PUZZLE ROOM SLIT */

L21000:
    if (prsvec_1.prsa != vindex_1.putw || prsvec_1.prsi != oindex_1.cslit) {
	goto L10;
    }
    if (prsvec_1.prso != oindex_1.gcard) {
	goto L21100;
    }
/* 						!PUT CARD IN SLIT? */
    newsta_(prsvec_1.prso, 863, 0, 0, 0);
/* 						!KILL CARD. */
    findex_1.cpoutf = TRUE_;
/* 						!OPEN DOOR. */
    objcts_1.oflag1[oindex_1.stldr - 1] &= ~ VISIBT;
    return ret_val;

L21100:
    if ((objcts_1.oflag1[prsvec_1.prso - 1] & VICTBT) == 0 && (
	    objcts_1.oflag2[prsvec_1.prso - 1] & VILLBT) == 0) {
	goto L21200;
    }
    i__1 = rnd_(5) + 552;
    rspeak_(i__1);
/* 						!JOKE FOR VILL, VICT. */
    return ret_val;

L21200:
    newsta_(prsvec_1.prso, 0, 0, 0, 0);
/* 						!KILL OBJECT. */
    rspsub_(864, odo2);
/* 						!DESCRIBE. */
    return ret_val;

} /* nobjs_ */

/* MIRPAN--	PROCESSOR FOR GLOBAL MIRROR/PANEL */

/* DECLARATIONS */

static logical mirpan_(st, pnf)
integer st;
logical pnf;
{
    /* System generated locals */
    integer i__1;
    logical ret_val;

    /* Local variables */
    integer num;
    integer mrbf;

    ret_val = TRUE_;
    num = mrhere_(play_1.here);
/* 						!GET MIRROR NUM. */
    if (num != 0) {
	goto L100;
    }
/* 						!ANY HERE? */
    rspeak_(st);
/* 						!NO, LOSE. */
    return ret_val;

L100:
    mrbf = 0;
/* 						!ASSUME MIRROR OK. */
    if (num == 1 && ! findex_1.mr1f || num == 2 && ! findex_1.mr2f) {
	mrbf = 1;
    }
    if (prsvec_1.prsa != vindex_1.movew && prsvec_1.prsa != vindex_1.openw) {
	goto L200;
    }
    i__1 = st + 1;
    rspeak_(i__1);
/* 						!CANT OPEN OR MOVE. */
    return ret_val;

L200:
    if (pnf || prsvec_1.prsa != vindex_1.lookiw && prsvec_1.prsa != 
	    vindex_1.examiw && prsvec_1.prsa != vindex_1.lookw) {
	goto L300;
    }
    i__1 = mrbf + 844;
    rspeak_(i__1);
/* 						!LOOK IN MIRROR. */
    return ret_val;

L300:
    if (prsvec_1.prsa != vindex_1.mungw) {
	goto L400;
    }
/* 						!BREAK? */
    i__1 = st + 2 + mrbf;
    rspeak_(i__1);
/* 						!DO IT. */
    if (num == 1 && ! (pnf)) {
	findex_1.mr1f = FALSE_;
    }
    if (num == 2 && ! (pnf)) {
	findex_1.mr2f = FALSE_;
    }
    return ret_val;

L400:
    if (pnf || mrbf == 0) {
	goto L500;
    }
/* 						!BROKEN MIRROR? */
    rspeak_(846);
    return ret_val;

L500:
    if (prsvec_1.prsa != vindex_1.pushw) {
	goto L600;
    }
/* 						!PUSH? */
    i__1 = st + 3 + num;
    rspeak_(i__1);
    return ret_val;

L600:
    ret_val = FALSE_;
/* 						!CANT HANDLE IT. */
    return ret_val;

} /* mirpan_ */
