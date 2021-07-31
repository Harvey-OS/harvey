/* SOBJS-	SIMPLE OBJECTS PROCESSOR */
/* 	OBJECTS IN THIS MODULE CANNOT CALL RMINFO, JIGSUP, */
/* 	MAJOR VERBS, OR OTHER NON-RESIDENT SUBROUTINES */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

logical sobjs_(ri, arg)
integer ri;
integer arg;
{
    /* System generated locals */
    integer i__1;
    logical ret_val;

    /* Local variables */
    logical f;
    integer i;
    integer mroom;
    integer av;
    integer odi2 = 0, odo2 = 0;

    if (prsvec_1.prso > 220) {
	goto L5;
    }
    if (prsvec_1.prso != 0) {
	odo2 = objcts_1.odesc2[prsvec_1.prso - 1];
    }
L5:
    if (prsvec_1.prsi != 0) {
	odi2 = objcts_1.odesc2[prsvec_1.prsi - 1];
    }
    av = advs_1.avehic[play_1.winner - 1];
    ret_val = TRUE_;

    switch (ri) {
	case 1:  goto L1000;
	case 2:  goto L3000;
	case 3:  goto L4000;
	case 4:  goto L6000;
	case 5:  goto L7000;
	case 6:  goto L8000;
	case 7:  goto L9000;
	case 8:  goto L13000;
	case 9:  goto L14000;
	case 10:  goto L16000;
	case 11:  goto L17000;
	case 12:  goto L21000;
	case 13:  goto L23000;
	case 14:  goto L24000;
	case 15:  goto L27000;
	case 16:  goto L28000;
	case 17:  goto L29000;
	case 18:  goto L30000;
	case 19:  goto L31000;
	case 20:  goto L33000;
	case 21:  goto L34000;
	case 22:  goto L36000;
	case 23:  goto L37000;
	case 24:  goto L38000;
	case 25:  goto L41000;
	case 26:  goto L42000;
	case 27:  goto L43000;
	case 28:  goto L44000;
	case 29:  goto L46000;
	case 30:  goto L53000;
	case 31:  goto L56000;
    }
    bug_(6, ri);

/* RETURN HERE TO DECLARE FALSE RESULT */

L10:
    ret_val = FALSE_;
    return ret_val;
/* SOBJS, PAGE 3 */

/* O1--	GUNK FUNCTION */

L1000:
    if (objcts_1.ocan[oindex_1.gunk - 1] == 0) {
	goto L10;
    }
/* 						!NOT INSIDE? F */
    newsta_(oindex_1.gunk, 122, 0, 0, 0);
/* 						!FALLS APART. */
    return ret_val;

/* O2--	TROPHY CASE */

L3000:
    if (prsvec_1.prsa != vindex_1.takew) {
	goto L10;
    }
/* 						!TAKE? */
    rspeak_(128);
/* 						!CANT. */
    return ret_val;

/* O3--	BOTTLE FUNCTION */

L4000:
    if (prsvec_1.prsa != vindex_1.throww) {
	goto L4100;
    }
/* 						!THROW? */
    newsta_(prsvec_1.prso, 129, 0, 0, 0);
/* 						!BREAKS. */
    return ret_val;

L4100:
    if (prsvec_1.prsa != vindex_1.mungw) {
	goto L10;
    }
/* 						!MUNG? */
    newsta_(prsvec_1.prso, 131, 0, 0, 0);
/* 						!BREAKS. */
    return ret_val;
/* SOBJS, PAGE 4 */

/* O4--	ROPE FUNCTION */

L6000:
    if (play_1.here == rindex_1.dome) {
	goto L6100;
    }
/* 						!IN DOME? */
    findex_1.domef = FALSE_;
/* 						!NO, */
    if (prsvec_1.prsa != vindex_1.untiew) {
	goto L6050;
    }
/* 						!UNTIE? */
    rspeak_(134);
/* 						!CANT */
    return ret_val;

L6050:
    if (prsvec_1.prsa != vindex_1.tiew) {
	goto L10;
    }
/* 						!TIE? */
    rspeak_(135);
/* 						!CANT TIE */
    return ret_val;

L6100:
    if (prsvec_1.prsa != vindex_1.tiew || prsvec_1.prsi != oindex_1.raili) {
	goto L6200;
    }
    if (findex_1.domef) {
	goto L6150;
    }
/* 						!ALREADY TIED? */
    findex_1.domef = TRUE_;
/* 						!NO, TIE IT. */
    objcts_1.oflag1[oindex_1.rope - 1] |= NDSCBT;
    objcts_1.oflag2[oindex_1.rope - 1] |= CLMBBT;
    newsta_(oindex_1.rope, 137, rindex_1.dome, 0, 0);
    return ret_val;

L6150:
    rspeak_(136);
/* 						!DUMMY. */
    return ret_val;

L6200:
    if (prsvec_1.prsa != vindex_1.untiew) {
	goto L6300;
    }
/* 						!UNTIE? */
    if (findex_1.domef) {
	goto L6250;
    }
/* 						!TIED? */
    rspeak_(134);
/* 						!NO, DUMMY. */
    return ret_val;

L6250:
    findex_1.domef = FALSE_;
/* 						!YES, UNTIE IT. */
    objcts_1.oflag1[oindex_1.rope - 1] &= ~ NDSCBT;
    objcts_1.oflag2[oindex_1.rope - 1] &= ~ CLMBBT;
    rspeak_(139);
    return ret_val;

L6300:
    if (findex_1.domef || prsvec_1.prsa != vindex_1.dropw) {
	goto L6400;
    }
/* 						!DROP & UNTIED? */
    newsta_(oindex_1.rope, 140, rindex_1.mtorc, 0, 0);
/* 						!YES, DROP. */
    return ret_val;

L6400:
    if (prsvec_1.prsa != vindex_1.takew || ! findex_1.domef) {
	goto L10;
    }
    rspeak_(141);
/* 						!TAKE & TIED. */
    return ret_val;

/* O5--	SWORD FUNCTION */

L7000:
    if (prsvec_1.prsa == vindex_1.takew && play_1.winner == aindex_1.player) {

	hack_1.swdact = TRUE_;
    }
    goto L10;

/* O6--	LANTERN */

L8000:
    if (prsvec_1.prsa != vindex_1.throww) {
	goto L8100;
    }
/* 						!THROW? */
    newsta_(oindex_1.lamp, 0, 0, 0, 0);
/* 						!KILL LAMP, */
    newsta_(oindex_1.blamp, 142, play_1.here, 0, 0);
/* 						!REPLACE WITH BROKEN. */
    return ret_val;

L8100:
    if (prsvec_1.prsa == vindex_1.trnonw) {
	cevent_1.cflag[cindex_1.cevlnt - 1] = TRUE_;
    }
    if (prsvec_1.prsa == vindex_1.trnofw) {
	cevent_1.cflag[cindex_1.cevlnt - 1] = FALSE_;
    }
    goto L10;

/* O7--	RUG FUNCTION */

L9000:
    if (prsvec_1.prsa != vindex_1.raisew) {
	goto L9100;
    }
/* 						!RAISE? */
    rspeak_(143);
/* 						!CANT */
    return ret_val;

L9100:
    if (prsvec_1.prsa != vindex_1.takew) {
	goto L9200;
    }
/* 						!TAKE? */
    rspeak_(144);
/* 						!CANT */
    return ret_val;

L9200:
    if (prsvec_1.prsa != vindex_1.movew) {
	goto L9300;
    }
/* 						!MOVE? */
    i__1 = findex_1.orrug + 145;
    rspeak_(i__1);
    findex_1.orrug = 1;
    objcts_1.oflag1[oindex_1.door - 1] |= VISIBT;
    return ret_val;

L9300:
    if (prsvec_1.prsa != vindex_1.lookuw || findex_1.orrug != 0 || (
	    objcts_1.oflag2[oindex_1.door - 1] & OPENBT) != 0) {
	goto L10;
    }
    rspeak_(345);
    return ret_val;
/* SOBJS, PAGE 5 */

/* O8--	SKELETON */

L13000:
    i = robrm_(play_1.here, 100, rindex_1.lld2, 0, 0) + robadv_(
	    play_1.winner, rindex_1.lld2, 0, 0);
    if (i != 0) {
	rspeak_(162);
    }
/* 						!IF ROBBED, SAY SO. */
    return ret_val;

/* O9--	MIRROR */

L14000:
    if (findex_1.mirrmf || prsvec_1.prsa != vindex_1.rubw) {
	goto L14500;
    }
    mroom = play_1.here ^ 1;
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!INTERCHANGE OBJS. */
	if (objcts_1.oroom[i - 1] == play_1.here) {
	    objcts_1.oroom[i - 1] = -1;
	}
	if (objcts_1.oroom[i - 1] == mroom) {
	    objcts_1.oroom[i - 1] = play_1.here;
	}
	if (objcts_1.oroom[i - 1] == -1) {
	    objcts_1.oroom[i - 1] = mroom;
	}
/* L14100: */
    }
    f = moveto_(mroom, play_1.winner);
    rspeak_(163);
/* 						!SHAKE WORLD. */
    return ret_val;

L14500:
    if (prsvec_1.prsa != vindex_1.lookw && prsvec_1.prsa != vindex_1.lookiw &&
	     prsvec_1.prsa != vindex_1.examiw) {
	goto L14600;
    }
    i = 164;
/* 						!MIRROR OK. */
    if (findex_1.mirrmf) {
	i = 165;
    }
/* 						!MIRROR DEAD. */
    rspeak_(i);
    return ret_val;

L14600:
    if (prsvec_1.prsa != vindex_1.takew) {
	goto L14700;
    }
/* 						!TAKE? */
    rspeak_(166);
/* 						!JOKE. */
    return ret_val;

L14700:
    if (prsvec_1.prsa != vindex_1.mungw && prsvec_1.prsa != vindex_1.throww) {

	goto L10;
    }
    i = 167;
/* 						!MIRROR BREAKS. */
    if (findex_1.mirrmf) {
	i = 168;
    }
/* 						!MIRROR ALREADY BROKEN. */
    findex_1.mirrmf = TRUE_;
    findex_1.badlkf = TRUE_;
    rspeak_(i);
    return ret_val;
/* SOBJS, PAGE 6 */

/* O10--	DUMBWAITER */

L16000:
    if (prsvec_1.prsa != vindex_1.raisew) {
	goto L16100;
    }
/* 						!RAISE? */
    if (findex_1.cagetf) {
	goto L16400;
    }
/* 						!ALREADY AT TOP? */
    newsta_(oindex_1.tbask, 175, rindex_1.tshaf, 0, 0);
/* 						!NO, RAISE BASKET. */
    newsta_(oindex_1.fbask, 0, rindex_1.bshaf, 0, 0);
    findex_1.cagetf = TRUE_;
/* 						!AT TOP. */
    return ret_val;

L16100:
    if (prsvec_1.prsa != vindex_1.lowerw) {
	goto L16200;
    }
/* 						!LOWER? */
    if (! findex_1.cagetf) {
	goto L16400;
    }
/* 						!ALREADY AT BOTTOM? */
    newsta_(oindex_1.tbask, 176, rindex_1.bshaf, 0, 0);
/* 						!NO, LOWER BASKET. */
    newsta_(oindex_1.fbask, 0, rindex_1.tshaf, 0, 0);
    findex_1.cagetf = FALSE_;
    if (! lit_(play_1.here)) {
	rspeak_(406);
    }
/* 						!IF DARK, DIE. */
    return ret_val;

L16200:
    if (prsvec_1.prso != oindex_1.fbask && prsvec_1.prsi != oindex_1.fbask) {
	goto L16300;
    }
    rspeak_(130);
/* 						!WRONG BASKET. */
    return ret_val;

L16300:
    if (prsvec_1.prsa != vindex_1.takew) {
	goto L10;
    }
/* 						!TAKE? */
    rspeak_(177);
/* 						!JOKE. */
    return ret_val;

L16400:
    i__1 = rnd_(3) + 125;
    rspeak_(i__1);
/* 						!DUMMY. */
    return ret_val;

/* O11--	GHOST FUNCTION */

L17000:
    i = 178;
/* 						!ASSUME DIRECT. */
    if (prsvec_1.prso != oindex_1.ghost) {
	i = 179;
    }
/* 						!IF NOT, INDIRECT. */
    rspeak_(i);
    return ret_val;
/* 						!SPEAK AND EXIT. */
/* SOBJS, PAGE 7 */

/* O12--	TUBE */

L21000:
    if (prsvec_1.prsa != vindex_1.putw || prsvec_1.prsi != oindex_1.tube) {
	goto L10;
    }
    rspeak_(186);
/* 						!CANT PUT BACK IN. */
    return ret_val;

/* O13--	CHALICE */

L23000:
    if (prsvec_1.prsa != vindex_1.takew || objcts_1.ocan[prsvec_1.prso - 1] !=
	     0 || objcts_1.oroom[prsvec_1.prso - 1] != rindex_1.treas || 
	    objcts_1.oroom[oindex_1.thief - 1] != rindex_1.treas || (
	    objcts_1.oflag2[oindex_1.thief - 1] & FITEBT) == 0 || ! 
	    hack_1.thfact) {
	goto L10;
    }
    rspeak_(204);
/* 						!CANT TAKE. */
    return ret_val;

/* O14--	PAINTING */

L24000:
    if (prsvec_1.prsa != vindex_1.mungw) {
	goto L10;
    }
/* 						!MUNG? */
    rspeak_(205);
/* 						!DESTROY PAINTING. */
    objcts_1.ofval[prsvec_1.prso - 1] = 0;
    objcts_1.otval[prsvec_1.prso - 1] = 0;
    objcts_1.odesc1[prsvec_1.prso - 1] = 207;
    objcts_1.odesc2[prsvec_1.prso - 1] = 206;
    return ret_val;
/* SOBJS, PAGE 8 */

/* O15--	BOLT */

L27000:
    if (prsvec_1.prsa != vindex_1.turnw) {
	goto L10;
    }
/* 						!TURN BOLT? */
    if (prsvec_1.prsi != oindex_1.wrenc) {
	goto L27500;
    }
/* 						!WITH WRENCH? */
    if (findex_1.gatef) {
	goto L27100;
    }
/* 						!PROPER BUTTON PUSHED? */
    rspeak_(210);
/* 						!NO, LOSE. */
    return ret_val;

L27100:
    if (findex_1.lwtidf) {
	goto L27200;
    }
/* 						!LOW TIDE NOW? */
    findex_1.lwtidf = TRUE_;
/* 						!NO, EMPTY DAM. */
    rspeak_(211);
    objcts_1.oflag2[oindex_1.coffi - 1] &= ~ SCRDBT;
    objcts_1.oflag1[oindex_1.trunk - 1] |= VISIBT;
    rooms_1.rflag[rindex_1.reser - 1] = (rooms_1.rflag[rindex_1.reser - 1] | 
	    RLAND) & ~ (RWATER + RSEEN);
    return ret_val;

L27200:
    findex_1.lwtidf = FALSE_;
/* 						!YES, FILL DAM. */
    rspeak_(212);
    if (qhere_(oindex_1.trunk, rindex_1.reser)) {
	objcts_1.oflag1[oindex_1.trunk - 1] &= ~ VISIBT;
    }
    rooms_1.rflag[rindex_1.reser - 1] = (rooms_1.rflag[rindex_1.reser - 1] | 
	    RWATER) & ~ RLAND;
    return ret_val;

L27500:
    rspsub_(299, odi2);
/* 						!NOT WITH THAT. */
    return ret_val;

/* O16--	GRATING */

L28000:
    if (prsvec_1.prsa != vindex_1.openw && prsvec_1.prsa != vindex_1.closew) {

	goto L10;
    }
    if (findex_1.grunlf) {
	goto L28200;
    }
/* 						!UNLOCKED? */
    rspeak_(214);
/* 						!NO, LOCKED. */
    return ret_val;

L28200:
    i = 215;
/* 						!UNLOCKED, VIEW FRM CLR. */
    if (play_1.here != rindex_1.clear) {
	i = 216;
    }
/* 						!VIEW FROM BELOW. */
    ret_val = opncls_(oindex_1.grate, i, 885);
/* 						!OPEN/CLOSE. */
    rooms_1.rflag[rindex_1.mgrat - 1] &= ~ RLIGHT;
    if ((objcts_1.oflag2[oindex_1.grate - 1] & OPENBT) != 0) {
	rooms_1.rflag[rindex_1.mgrat - 1] |= RLIGHT;
    }
    if (! lit_(play_1.here)) {
	rspeak_(406);
    }
/* 						!IF DARK, DIE. */
    return ret_val;

/* O17--	TRAP DOOR */

L29000:
    if (play_1.here != rindex_1.lroom) {
	goto L29100;
    }
/* 						!FROM LIVING ROOM? */
    ret_val = opncls_(oindex_1.door, 218, 219);
/* 						!OPEN/CLOSE. */
    return ret_val;

L29100:
    if (play_1.here != rindex_1.cella) {
	goto L10;
    }
/* 						!FROM CELLAR? */
    if (prsvec_1.prsa != vindex_1.openw || (objcts_1.oflag2[oindex_1.door - 1]
	     & OPENBT) != 0) {
	goto L29200;
    }
    rspeak_(220);
/* 						!CANT OPEN CLOSED DOOR. */
    return ret_val;

L29200:
    ret_val = opncls_(oindex_1.door, 0, 22);
/* 						!NORMAL OPEN/CLOSE. */
    return ret_val;

/* O18--	DURABLE DOOR */

L30000:
    i = 0;
/* 						!ASSUME NO APPL. */
    if (prsvec_1.prsa == vindex_1.openw) {
	i = 221;
    }
/* 						!OPEN? */
    if (prsvec_1.prsa == vindex_1.burnw) {
	i = 222;
    }
/* 						!BURN? */
    if (prsvec_1.prsa == vindex_1.mungw) {
	i = rnd_(3) + 223;
    }
/* 						!MUNG? */
    if (i == 0) {
	goto L10;
    }
    rspeak_(i);
    return ret_val;

/* O19--	MASTER SWITCH */

L31000:
    if (prsvec_1.prsa != vindex_1.turnw) {
	goto L10;
    }
/* 						!TURN? */
    if (prsvec_1.prsi != oindex_1.screw) {
	goto L31500;
    }
/* 						!WITH SCREWDRIVER? */
    if ((objcts_1.oflag2[oindex_1.machi - 1] & OPENBT) != 0) {
	goto L31600;
    }
/* 						!LID UP? */
    rspeak_(226);
/* 						!NO, ACTIVATE. */
    if (objcts_1.ocan[oindex_1.coal - 1] != oindex_1.machi) {
	goto L31400;
    }
/* 						!COAL INSIDE? */
    newsta_(oindex_1.coal, 0, 0, 0, 0);
/* 						!KILL COAL, */
    newsta_(oindex_1.diamo, 0, 0, oindex_1.machi, 0);
/* 						!REPLACE WITH DIAMOND. */
    return ret_val;

L31400:
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!KILL NONCOAL OBJECTS. */
	if (objcts_1.ocan[i - 1] != oindex_1.machi) {
	    goto L31450;
	}
/* 						!INSIDE MACHINE? */
	newsta_(i, 0, 0, 0, 0);
/* 						!KILL OBJECT AND CONTENTS. */
	newsta_(oindex_1.gunk, 0, 0, oindex_1.machi, 0);
/* 						!REDUCE TO GUNK. */
L31450:
	;
    }
    return ret_val;

L31500:
    rspsub_(300, odi2);
/* 						!CANT TURN WITH THAT. */
    return ret_val;

L31600:
    rspeak_(227);
/* 						!LID IS UP. */
    return ret_val;
/* SOBJS, PAGE 9 */

/* O20--	LEAK */

L33000:
    if (prsvec_1.prso != oindex_1.leak || prsvec_1.prsa != vindex_1.plugw || 
	    findex_1.rvmnt <= 0) {
	goto L10;
    }
    if (prsvec_1.prsi != oindex_1.putty) {
	goto L33100;
    }
/* 						!WITH PUTTY? */
    findex_1.rvmnt = -1;
/* 						!DISABLE LEAK. */
    cevent_1.ctick[cindex_1.cevmnt - 1] = 0;
    rspeak_(577);
    return ret_val;

L33100:
    rspsub_(301, odi2);
/* 						!CANT WITH THAT. */
    return ret_val;

/* O21--	DROWNING BUTTONS */

L34000:
    if (prsvec_1.prsa != vindex_1.pushw) {
	goto L10;
    }
/* 						!PUSH? */
    switch (prsvec_1.prso - oindex_1.rbutt + 1) {
	case 1:  goto L34100;
	case 2:  goto L34200;
	case 3:  goto L34300;
	case 4:  goto L34400;
    }
    goto L10;
/* 						!NOT A BUTTON. */

L34100:
    rooms_1.rflag[play_1.here - 1] ^= RLIGHT;
    i = 230;
    if ((rooms_1.rflag[play_1.here - 1] & RLIGHT) != 0) {
	i = 231;
    }
    rspeak_(i);
    return ret_val;

L34200:
    findex_1.gatef = TRUE_;
/* 						!RELEASE GATE. */
    rspeak_(232);
    return ret_val;

L34300:
    findex_1.gatef = FALSE_;
/* 						!INTERLOCK GATE. */
    rspeak_(232);
    return ret_val;

L34400:
    if (findex_1.rvmnt != 0) {
	goto L34500;
    }
/* 						!LEAK ALREADY STARTED? */
    rspeak_(233);
/* 						!NO, START LEAK. */
    findex_1.rvmnt = 1;
    cevent_1.ctick[cindex_1.cevmnt - 1] = -1;
    return ret_val;

L34500:
    rspeak_(234);
/* 						!BUTTON JAMMED. */
    return ret_val;

/* O22--	INFLATABLE BOAT */

L36000:
    if (prsvec_1.prsa != vindex_1.inflaw) {
	goto L10;
    }
/* 						!INFLATE? */
    if (objcts_1.oroom[oindex_1.iboat - 1] != 0) {
	goto L36100;
    }
/* 						!IN ROOM? */
    rspeak_(235);
/* 						!NO, JOKE. */
    return ret_val;

L36100:
    if (prsvec_1.prsi != oindex_1.pump) {
	goto L36200;
    }
/* 						!WITH PUMP? */
    newsta_(oindex_1.iboat, 0, 0, 0, 0);
/* 						!KILL DEFL BOAT, */
    newsta_(oindex_1.rboat, 236, play_1.here, 0, 0);
/* 						!REPL WITH INF. */
    findex_1.deflaf = FALSE_;
    return ret_val;

L36200:
    i = 237;
/* 						!JOKES. */
    if (prsvec_1.prsi != oindex_1.lungs) {
	i = 303;
    }
    rspsub_(i, odi2);
    return ret_val;

/* O23--	DEFLATED BOAT */

L37000:
    if (prsvec_1.prsa != vindex_1.inflaw) {
	goto L37100;
    }
/* 						!INFLATE? */
    rspeak_(238);
/* 						!JOKE. */
    return ret_val;

L37100:
    if (prsvec_1.prsa != vindex_1.plugw) {
	goto L10;
    }
/* 						!PLUG? */
    if (prsvec_1.prsi != oindex_1.putty) {
	goto L33100;
    }
/* 						!WITH PUTTY? */
    newsta_(oindex_1.iboat, 239, objcts_1.oroom[oindex_1.dboat - 1], 
	    objcts_1.ocan[oindex_1.dboat - 1], objcts_1.oadv[oindex_1.dboat 
	    - 1]);
    newsta_(oindex_1.dboat, 0, 0, 0, 0);
/* 						!KILL DEFL BOAT, REPL. */
    return ret_val;
/* SOBJS, PAGE 10 */

/* O24--	RUBBER BOAT */

L38000:
    if (arg != 0) {
	goto L10;
    }
/* 						!DISMISS READIN, OUT. */
    if (prsvec_1.prsa != vindex_1.boardw || objcts_1.oadv[oindex_1.stick - 1] 
	    != play_1.winner) {
	goto L38100;
    }
    newsta_(oindex_1.rboat, 0, 0, 0, 0);
/* 						!KILL INFL BOAT, */
    newsta_(oindex_1.dboat, 240, play_1.here, 0, 0);
/* 						!REPL WITH DEAD. */
    findex_1.deflaf = TRUE_;
    return ret_val;

L38100:
    if (prsvec_1.prsa != vindex_1.inflaw) {
	goto L38200;
    }
/* 						!INFLATE? */
    rspeak_(367);
/* 						!YES, JOKE. */
    return ret_val;

L38200:
    if (prsvec_1.prsa != vindex_1.deflaw) {
	goto L10;
    }
/* 						!DEFLATE? */
    if (av == oindex_1.rboat) {
	goto L38300;
    }
/* 						!IN BOAT? */
    if (objcts_1.oroom[oindex_1.rboat - 1] == 0) {
	goto L38400;
    }
/* 						!ON GROUND? */
    newsta_(oindex_1.rboat, 0, 0, 0, 0);
/* 						!KILL INFL BOAT, */
    newsta_(oindex_1.iboat, 241, play_1.here, 0, 0);
/* 						!REPL WITH DEFL. */
    findex_1.deflaf = TRUE_;
    return ret_val;

L38300:
    rspeak_(242);
/* 						!IN BOAT. */
    return ret_val;

L38400:
    rspeak_(243);
/* 						!NOT ON GROUND. */
    return ret_val;

/* O25--	BRAIDED ROPE */

L41000:
    if (prsvec_1.prsa != vindex_1.tiew || prsvec_1.prso != oindex_1.brope || 
	    prsvec_1.prsi != oindex_1.hook1 && prsvec_1.prsi != 
	    oindex_1.hook2) {
	goto L41500;
    }
    findex_1.btief = prsvec_1.prsi;
/* 						!RECORD LOCATION. */
    cevent_1.cflag[cindex_1.cevbal - 1] = FALSE_;
/* 						!STALL ASCENT. */
    rspeak_(248);
    return ret_val;

L41500:
    if (prsvec_1.prsa != vindex_1.untiew || prsvec_1.prso != oindex_1.brope) {

	goto L10;
    }
    if (findex_1.btief != 0) {
	goto L41600;
    }
/* 						!TIED UP? */
    rspeak_(249);
/* 						!NO, JOKE. */
    return ret_val;

L41600:
    rspeak_(250);
    findex_1.btief = 0;
/* 						!UNTIE. */
    cevent_1.ctick[cindex_1.cevbal - 1] = 3;
/* 						!RESTART CLOCK. */
    cevent_1.cflag[cindex_1.cevbal - 1] = TRUE_;
    return ret_val;

/* O26--	SAFE */

L42000:
    i = 0;
/* 						!ASSUME UNPROCESSED. */
    if (prsvec_1.prsa == vindex_1.takew) {
	i = 251;
    }
/* 						!TAKE? */
    if (prsvec_1.prsa == vindex_1.openw && findex_1.safef) {
	i = 253;
    }
/* 						!OPEN AFTER BLAST? */
    if (prsvec_1.prsa == vindex_1.openw && ! findex_1.safef) {
	i = 254;
    }
/* 						!OPEN BEFORE BLAST? */
    if (prsvec_1.prsa == vindex_1.closew && findex_1.safef) {
	i = 253;
    }
/* 						!CLOSE AFTER? */
    if (prsvec_1.prsa == vindex_1.closew && ! findex_1.safef) {
	i = 255;
    }
    if (i == 0) {
	goto L10;
    }
    rspeak_(i);
    return ret_val;

/* O27--	FUSE */

L43000:
    if (prsvec_1.prsa != vindex_1.burnw) {
	goto L10;
    }
/* 						!BURN? */
    rspeak_(256);
    cevent_1.ctick[cindex_1.cevfus - 1] = 2;
/* 						!START COUNTDOWN. */
    return ret_val;

/* O28--	GNOME */

L44000:
    if (prsvec_1.prsa != vindex_1.givew && prsvec_1.prsa != vindex_1.throww) {

	goto L44500;
    }
    if (objcts_1.otval[prsvec_1.prso - 1] == 0) {
	goto L44100;
    }
/* 						!TREASURE? */
    rspsub_(257, odo2);
/* 						!YES, GET DOOR. */
    newsta_(prsvec_1.prso, 0, 0, 0, 0);
    newsta_(oindex_1.gnome, 0, 0, 0, 0);
/* 						!VANISH GNOME. */
    findex_1.gnodrf = TRUE_;
    return ret_val;

L44100:
    rspsub_(258, odo2);
/* 						!NO, LOSE OBJECT. */
    newsta_(prsvec_1.prso, 0, 0, 0, 0);
    return ret_val;

L44500:
    rspeak_(259);
/* 						!NERVOUS GNOME. */
    if (! findex_1.gnomef) {
	cevent_1.ctick[cindex_1.cevgno - 1] = 5;
    }
/* 						!SCHEDULE BYEBYE. */
    findex_1.gnomef = TRUE_;
    return ret_val;

/* O29--	COKE BOTTLES */

L46000:
    if (prsvec_1.prsa != vindex_1.throww && prsvec_1.prsa != vindex_1.mungw) {

	goto L10;
    }
    newsta_(prsvec_1.prso, 262, 0, 0, 0);
/* 						!MUNG BOTTLES. */
    return ret_val;
/* SOBJS, PAGE 11 */


/* O30--	ROBOT */

L53000:
    if (prsvec_1.prsa != vindex_1.givew) {
	goto L53200;
    }
/* 						!GIVE? */
    newsta_(prsvec_1.prso, 0, 0, 0, aindex_1.arobot);
/* 						!PUT ON ROBOT. */
    rspsub_(302, odo2);
    return ret_val;

L53200:
    if (prsvec_1.prsa != vindex_1.mungw && prsvec_1.prsa != vindex_1.throww) {

	goto L10;
    }
    newsta_(oindex_1.robot, 285, 0, 0, 0);
/* 						!KILL ROBOT. */
    return ret_val;

/* O31--	GRUE */

L56000:
    if (prsvec_1.prsa != vindex_1.examiw) {
	goto L56100;
    }
/* 						!EXAMINE? */
    rspeak_(288);
    return ret_val;

L56100:
    if (prsvec_1.prsa != vindex_1.findw) {
	goto L10;
    }
/* 						!FIND? */
    rspeak_(289);
    return ret_val;

} /* sobjs_ */
