/* RAPPL1- SPECIAL PURPOSE ROOM ROUTINES, PART 1 */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

logical rappl1_(ri)
integer ri;
{
    /* System generated locals */
    integer i__1, i__2;
    logical ret_val;

    /* Local variables */
    logical f;
    integer i;
    integer j;

    ret_val = TRUE_;
/* 						!USUALLY IGNORED. */
    if (ri == 0) {
	return ret_val;
    }
/* 						!RETURN IF NAUGHT. */

/* 						!SET TO FALSE FOR */

/* 						!NEW DESC NEEDED. */
    switch (ri) {
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
	case 22:  goto L22000;
	case 23:  goto L23000;
	case 24:  goto L24000;
	case 25:  goto L25000;
	case 26:  goto L26000;
	case 27:  goto L27000;
	case 28:  goto L28000;
	case 29:  goto L29000;
	case 30:  goto L30000;
	case 31:  goto L31000;
	case 32:  goto L32000;
	case 33:  goto L33000;
	case 34:  goto L34000;
	case 35:  goto L35000;
	case 36:  goto L36000;
	case 37:  goto L37000;
    }
    bug_(1, ri);

/* R1--	EAST OF HOUSE.  DESCRIPTION DEPENDS ON STATE OF WINDOW */

L1000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 13;
/* 						!ASSUME CLOSED. */
    if ((objcts_1.oflag2[oindex_1.windo - 1] & OPENBT) != 0) {
	i = 12;
    }
/* 						!IF OPEN, AJAR. */
    rspsub_(11, i);
/* 						!DESCRIBE. */
    return ret_val;

/* R2--	KITCHEN.  SAME VIEW FROM INSIDE. */

L2000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 13;
/* 						!ASSUME CLOSED. */
    if ((objcts_1.oflag2[oindex_1.windo - 1] & OPENBT) != 0) {
	i = 12;
    }
/* 						!IF OPEN, AJAR. */
    rspsub_(14, i);
/* 						!DESCRIBE. */
    return ret_val;

/* R3--	LIVING ROOM.  DESCRIPTION DEPENDS ON MAGICF (STATE OF */
/* 	DOOR TO CYCLOPS ROOM), RUG (MOVED OR NOT), DOOR (OPEN OR CLOSED) */

L3000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	goto L3500;
    }
/* 						!LOOK? */
    i = 15;
/* 						!ASSUME NO HOLE. */
    if (findex_1.magicf) {
	i = 16;
    }
/* 						!IF MAGICF, CYCLOPS HOLE. */
    rspeak_(i);
/* 						!DESCRIBE. */
    i = findex_1.orrug + 17;
/* 						!ASSUME INITIAL STATE. */
    if ((objcts_1.oflag2[oindex_1.door - 1] & OPENBT) != 0) {
	i += 2;
    }
/* 						!DOOR OPEN? */
    rspeak_(i);
/* 						!DESCRIBE. */
    return ret_val;

/* 	NOT A LOOK WORD.  REEVALUATE TROPHY CASE. */

L3500:
    if (prsvec_1.prsa != vindex_1.takew && (prsvec_1.prsa != vindex_1.putw || 
	    prsvec_1.prsi != oindex_1.tcase)) {
	return ret_val;
    }
    advs_1.ascore[play_1.winner - 1] = state_1.rwscor;
/* 						!SCORE TROPHY CASE. */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!RETAIN RAW SCORE AS WELL. */
	j = i;
/* 						!FIND OUT IF IN CASE. */
L3550:
	j = objcts_1.ocan[j - 1];
/* 						!TRACE OWNERSHIP. */
	if (j == 0) {
	    goto L3600;
	}
	if (j != oindex_1.tcase) {
	    goto L3550;
	}
/* 						!DO ALL LEVELS. */
	advs_1.ascore[play_1.winner - 1] += objcts_1.otval[i - 1];
L3600:
	;
    }
    scrupd_(0);
/* 						!SEE IF ENDGAME TRIG. */
    return ret_val;
/* RAPPL1, PAGE 3 */

/* R4--	CELLAR.  SHUT DOOR AND BAR IT IF HE JUST WALKED IN. */

L4000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	goto L4500;
    }
/* 						!LOOK? */
    rspeak_(21);
/* 						!DESCRIBE CELLAR. */
    return ret_val;

L4500:
    if (prsvec_1.prsa != vindex_1.walkiw) {
	return ret_val;
    }
/* 						!WALKIN? */
    if ((objcts_1.oflag2[oindex_1.door - 1] & OPENBT + 
	    TCHBT) != OPENBT) {
	return ret_val;
    }
    objcts_1.oflag2[oindex_1.door - 1] = (objcts_1.oflag2[oindex_1.door - 1] |
	     TCHBT) & ~ OPENBT;
    rspeak_(22);
/* 						!SLAM AND BOLT DOOR. */
    return ret_val;

/* R5--	MAZE11.  DESCRIBE STATE OF GRATING. */

L5000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(23);
/* 						!DESCRIBE. */
    i = 24;
/* 						!ASSUME LOCKED. */
    if (findex_1.grunlf) {
	i = 26;
    }
/* 						!UNLOCKED? */
    if ((objcts_1.oflag2[oindex_1.grate - 1] & OPENBT) != 0) {
	i = 25;
    }
/* 						!OPEN? */
    rspeak_(i);
/* 						!DESCRIBE GRATE. */
    return ret_val;

/* R6--	CLEARING.  DESCRIBE CLEARING, MOVE LEAVES. */

L6000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	goto L6500;
    }
/* 						!LOOK? */
    rspeak_(27);
/* 						!DESCRIBE. */
    if (findex_1.rvclr == 0) {
	return ret_val;
    }
/* 						!LEAVES MOVED? */
    i = 28;
/* 						!YES, ASSUME GRATE CLOSED. */
    if ((objcts_1.oflag2[oindex_1.grate - 1] & OPENBT) != 0) {
	i = 29;
    }
/* 						!OPEN? */
    rspeak_(i);
/* 						!DESCRIBE GRATE. */
    return ret_val;

L6500:
    if (findex_1.rvclr != 0 || qhere_(oindex_1.leave, rindex_1.clear) && (
	    prsvec_1.prsa != vindex_1.movew || prsvec_1.prso != 
	    oindex_1.leave)) {
	return ret_val;
    }
    rspeak_(30);
/* 						!MOVE LEAVES, REVEAL GRATE. */
    findex_1.rvclr = 1;
/* 						!INDICATE LEAVES MOVED. */
    return ret_val;
/* RAPPL1, PAGE 4 */

/* R7--	RESERVOIR SOUTH.  DESCRIPTION DEPENDS ON LOW TIDE FLAG. */

L7000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 31;
/* 						!ASSUME FULL. */
    if (findex_1.lwtidf) {
	i = 32;
    }
/* 						!IF LOW TIDE, EMPTY. */
    rspeak_(i);
/* 						!DESCRIBE. */
    rspeak_(33);
/* 						!DESCRIBE EXITS. */
    return ret_val;

/* R8--	RESERVOIR.  STATE DEPENDS ON LOW TIDE FLAG. */

L8000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 34;
/* 						!ASSUME FULL. */
    if (findex_1.lwtidf) {
	i = 35;
    }
/* 						!IF LOW TIDE, EMTPY. */
    rspeak_(i);
/* 						!DESCRIBE. */
    return ret_val;

/* R9--	RESERVOIR NORTH.  ALSO DEPENDS ON LOW TIDE FLAG. */

L9000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 36;
/* 						!YOU GET THE IDEA. */
    if (findex_1.lwtidf) {
	i = 37;
    }
    rspeak_(i);
    rspeak_(38);
    return ret_val;

/* R10--	GLACIER ROOM.  STATE DEPENDS ON MELTED, VANISHED FLAGS. */

L10000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(39);
/* 						!BASIC DESCRIPTION. */
    i = 0;
/* 						!ASSUME NO CHANGES. */
    if (findex_1.glacmf) {
	i = 40;
    }
/* 						!PARTIAL MELT? */
    if (findex_1.glacrf) {
	i = 41;
    }
/* 						!COMPLETE MELT? */
    rspeak_(i);
/* 						!DESCRIBE. */
    return ret_val;

/* R11--	FOREST ROOM */

L11000:
    if (prsvec_1.prsa == vindex_1.walkiw) {
	cevent_1.cflag[cindex_1.cevfor - 1] = TRUE_;
    }
/* 						!IF WALK IN, BIRDIE. */
    return ret_val;

/* R12--	MIRROR ROOM.  STATE DEPENDS ON MIRROR INTACT. */

L12000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(42);
/* 						!DESCRIBE. */
    if (findex_1.mirrmf) {
	rspeak_(43);
    }
/* 						!IF BROKEN, NASTY REMARK. */
    return ret_val;
/* RAPPL1, PAGE 5 */

/* R13--	CAVE2 ROOM.  BLOW OUT CANDLES WITH 50% PROBABILITY. */

L13000:
    if (prsvec_1.prsa != vindex_1.walkiw) {
	return ret_val;
    }
/* 						!WALKIN? */
    if (prob_(50, 50) || objcts_1.oadv[oindex_1.candl - 1] != 
	    play_1.winner || ! ((objcts_1.oflag1[oindex_1.candl - 1] & 
	    ONBT) != 0)) {
	return ret_val;
    }
    objcts_1.oflag1[oindex_1.candl - 1] &= ~ ONBT;
    rspeak_(47);
/* 						!TELL OF WINDS. */
    cevent_1.cflag[cindex_1.cevcnd - 1] = FALSE_;
/* 						!HALT CANDLE COUNTDOWN. */
    return ret_val;

/* R14--	BOOM ROOM.  BLOW HIM UP IF CARRYING FLAMING OBJECT. */

L14000:
    j = objcts_1.odesc2[oindex_1.candl - 1];
/* 						!ASSUME CANDLE. */
    if (objcts_1.oadv[oindex_1.candl - 1] == play_1.winner && (
	    objcts_1.oflag1[oindex_1.candl - 1] & ONBT) != 0) {
	goto L14100;
    }
    j = objcts_1.odesc2[oindex_1.torch - 1];
/* 						!ASSUME TORCH. */
    if (objcts_1.oadv[oindex_1.torch - 1] == play_1.winner && (
	    objcts_1.oflag1[oindex_1.torch - 1] & ONBT) != 0) {
	goto L14100;
    }
    j = objcts_1.odesc2[oindex_1.match - 1];
    if (objcts_1.oadv[oindex_1.match - 1] == play_1.winner && (
	    objcts_1.oflag1[oindex_1.match - 1] & ONBT) != 0) {
	goto L14100;
    }
    return ret_val;
/* 						!SAFE */

L14100:
    if (prsvec_1.prsa != vindex_1.trnonw) {
	goto L14200;
    }
/* 						!TURN ON? */
    rspsub_(294, j);
/* 						!BOOM */
/* 						! */
    jigsup_(44);
    return ret_val;

L14200:
    if (prsvec_1.prsa != vindex_1.walkiw) {
	return ret_val;
    }
/* 						!WALKIN? */
    rspsub_(295, j);
/* 						!BOOM */
/* 						! */
    jigsup_(44);
    return ret_val;

/* R15--	NO-OBJS.  SEE IF EMPTY HANDED, SCORE LIGHT SHAFT. */

L15000:
    findex_1.empthf = TRUE_;
/* 						!ASSUME TRUE. */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!SEE IF CARRYING. */
	if (objcts_1.oadv[i - 1] == play_1.winner) {
	    findex_1.empthf = FALSE_;
	}
/* L15100: */
    }

    if (play_1.here != rindex_1.bshaf || ! lit_(play_1.here)) {
	return ret_val;
    }
    scrupd_(state_1.ltshft);
/* 						!SCORE LIGHT SHAFT. */
    state_1.ltshft = 0;
/* 						!NEVER AGAIN. */
    return ret_val;
/* RAPPL1, PAGE 6 */

/* R16--	MACHINE ROOM.  DESCRIBE MACHINE. */

L16000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 46;
/* 						!ASSUME LID CLOSED. */
    if ((objcts_1.oflag2[oindex_1.machi - 1] & OPENBT) != 0) {
	i = 12;
    }
/* 						!IF OPEN, OPEN. */
    rspsub_(45, i);
/* 						!DESCRIBE. */
    return ret_val;

/* R17--	BAT ROOM.  UNLESS CARRYING GARLIC, FLY AWAY WITH ME... */

L17000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	goto L17500;
    }
/* 						!LOOK? */
    rspeak_(48);
/* 						!DESCRIBE ROOM. */
    if (objcts_1.oadv[oindex_1.garli - 1] == play_1.winner) {
	rspeak_(49);
    }
/* 						!BAT HOLDS NOSE. */
    return ret_val;

L17500:
    if (prsvec_1.prsa != vindex_1.walkiw || objcts_1.oadv[oindex_1.garli - 1] 
	    == play_1.winner) {
	return ret_val;
    }
    rspeak_(50);
/* 						!TIME TO FLY, JACK. */
    f = moveto_(bats_1.batdrp[rnd_(9)], play_1.winner);
/* 						!SELECT RANDOM DEST. */
    ret_val = FALSE_;
/* 						!INDICATE NEW DESC NEEDED. */
    return ret_val;

/* R18--	DOME ROOM.  STATE DEPENDS ON WHETHER ROPE TIED TO RAILING. */

L18000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	goto L18500;
    }
/* 						!LOOK? */
    rspeak_(51);
/* 						!DESCRIBE. */
    if (findex_1.domef) {
	rspeak_(52);
    }
/* 						!IF ROPE, DESCRIBE. */
    return ret_val;

L18500:
    if (prsvec_1.prsa == vindex_1.leapw) {
	jigsup_(53);
    }
/* 						!DID HE JUMP??? */
    return ret_val;

/* R19--	TORCH ROOM.  ALSO DEPENDS ON WHETHER ROPE TIED TO RAILING. */

L19000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(54);
/* 						!DESCRIBE. */
    if (findex_1.domef) {
	rspeak_(55);
    }
/* 						!IF ROPE, DESCRIBE. */
    return ret_val;

/* R20--	CAROUSEL ROOM.  SPIN HIM OR KILL HIM. */

L20000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	goto L20500;
    }
/* 						!LOOK? */
    rspeak_(56);
/* 						!DESCRIBE. */
    if (! findex_1.caroff) {
	rspeak_(57);
    }
/* 						!IF NOT FLIPPED, SPIN. */
    return ret_val;

L20500:
    if (prsvec_1.prsa == vindex_1.walkiw && findex_1.carozf) {
	jigsup_(58);
    }
/* 						!WALKED IN. */
    return ret_val;
/* RAPPL1, PAGE 7 */

/* R21--	LLD ROOM.  HANDLE EXORCISE, DESCRIPTIONS. */

L21000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	goto L21500;
    }
/* 						!LOOK? */
    rspeak_(59);
/* 						!DESCRIBE. */
    if (! findex_1.lldf) {
	rspeak_(60);
    }
/* 						!IF NOT VANISHED, GHOSTS. */
    return ret_val;

L21500:
    if (prsvec_1.prsa != vindex_1.exorcw) {
	return ret_val;
    }
/* 						!EXORCISE? */
    if (objcts_1.oadv[oindex_1.bell - 1] == play_1.winner && objcts_1.oadv[
	    oindex_1.book - 1] == play_1.winner && objcts_1.oadv[
	    oindex_1.candl - 1] == play_1.winner && (objcts_1.oflag1[
	    oindex_1.candl - 1] & ONBT) != 0) {
	goto L21600;
    }
    rspeak_(62);
/* 						!NOT EQUIPPED. */
    return ret_val;

L21600:
    if (qhere_(oindex_1.ghost, play_1.here)) {
	goto L21700;
    }
/* 						!GHOST HERE? */
    jigsup_(61);
/* 						!NOPE, EXORCISE YOU. */
    return ret_val;

L21700:
    newsta_(oindex_1.ghost, 63, 0, 0, 0);
/* 						!VANISH GHOST. */
    findex_1.lldf = TRUE_;
/* 						!OPEN GATE. */
    return ret_val;

/* R22--	LLD2-ROOM.  IS HIS HEAD ON A POLE? */

L22000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(64);
/* 						!DESCRIBE. */
    if (findex_1.onpolf) {
	rspeak_(65);
    }
/* 						!ON POLE? */
    return ret_val;

/* R23--	DAM ROOM.  DESCRIBE RESERVOIR, PANEL. */

L23000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(66);
/* 						!DESCRIBE. */
    i = 67;
    if (findex_1.lwtidf) {
	i = 68;
    }
    rspeak_(i);
/* 						!DESCRIBE RESERVOIR. */
    rspeak_(69);
/* 						!DESCRIBE PANEL. */
    if (findex_1.gatef) {
	rspeak_(70);
    }
/* 						!BUBBLE IS GLOWING. */
    return ret_val;

/* R24--	TREE ROOM */

L24000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(660);
/* 						!DESCRIBE. */
    i = 661;
/* 						!SET FLAG FOR BELOW. */
    i__1 = objcts_1.olnt;
    for (j = 1; j <= i__1; ++j) {
/* 						!DESCRIBE OBJ IN FORE3. */
	if (! qhere_(j, rindex_1.fore3) || j == oindex_1.ftree) {
	    goto L24200;
	}
	rspeak_(i);
/* 						!SET STAGE, */
	i = 0;
	rspsub_(502, objcts_1.odesc2[j - 1]);
/* 						!DESCRIBE. */
L24200:
	;
    }
    return ret_val;
/* RAPPL1, PAGE 8 */

/* R25--	CYCLOPS-ROOM.  DEPENDS ON CYCLOPS STATE, ASLEEP FLAG, MAGIC FLAG.
 */

L25000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(606);
/* 						!DESCRIBE. */
    i = 607;
/* 						!ASSUME BASIC STATE. */
    if (findex_1.rvcyc > 0) {
	i = 608;
    }
/* 						!>0?  HUNGRY. */
    if (findex_1.rvcyc < 0) {
	i = 609;
    }
/* 						!<0?  THIRSTY. */
    if (findex_1.cyclof) {
	i = 610;
    }
/* 						!ASLEEP? */
    if (findex_1.magicf) {
	i = 611;
    }
/* 						!GONE? */
    rspeak_(i);
/* 						!DESCRIBE. */
    if (! findex_1.cyclof && findex_1.rvcyc != 0) {
	i__1 = abs(findex_1.rvcyc) + 193;
	rspeak_(i__1);
    }
    return ret_val;

/* R26--	BANK BOX ROOM. */

L26000:
    if (prsvec_1.prsa != vindex_1.walkiw) {
	return ret_val;
    }
/* 						!SURPRISE HIM. */
    for (i = 1; i <= 8; i += 2) {
/* 						!SCOLRM DEPENDS ON */
	if (screen_1.fromdr == screen_1.scoldr[i - 1]) {
	    screen_1.scolrm = screen_1.scoldr[i];
	}
/* L26100: */
    }
/* 						!ENTRY DIRECTION. */
    return ret_val;

/* R27--	TREASURE ROOM. */

L27000:
    if (prsvec_1.prsa != vindex_1.walkiw || ! hack_1.thfact) {
	return ret_val;
    }
    if (objcts_1.oroom[oindex_1.thief - 1] != play_1.here) {
	newsta_(oindex_1.thief, 82, play_1.here, 0, 0);
    }
    hack_1.thfpos = play_1.here;
/* 						!RESET SEARCH PATTERN. */
    objcts_1.oflag2[oindex_1.thief - 1] |= FITEBT;
    if (objcts_1.oroom[oindex_1.chali - 1] == play_1.here) {
	objcts_1.oflag1[oindex_1.chali - 1] &= ~ TAKEBT;
    }

/* 	VANISH EVERYTHING IN ROOM */

    j = 0;
/* 						!ASSUME NOTHING TO VANISH. */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
	if (i == oindex_1.chali || i == oindex_1.thief || ! qhere_(i, 
		play_1.here)) {
	    goto L27200;
	}
	j = 83;
/* 						!FLAG BYEBYE. */
	objcts_1.oflag1[i - 1] &= ~ VISIBT;
L27200:
	;
    }
    rspeak_(j);
/* 						!DESCRIBE. */
    return ret_val;

/* R28--	CLIFF FUNCTION.  SEE IF CARRYING INFLATED BOAT. */

L28000:
    findex_1.deflaf = objcts_1.oadv[oindex_1.rboat - 1] != play_1.winner;
/* 						!TRUE IF NOT CARRYING. */
    return ret_val;
/* RAPPL1, PAGE 9 */

/* R29--	RIVR4 ROOM.  PLAY WITH BUOY. */

L29000:
    if (! findex_1.buoyf || objcts_1.oadv[oindex_1.buoy - 1] != play_1.winner)
	     {
	return ret_val;
    }
    rspeak_(84);
/* 						!GIVE HINT, */
    findex_1.buoyf = FALSE_;
/* 						!THEN DISABLE. */
    return ret_val;

/* R30--	OVERFALLS.  DOOM. */

L30000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	jigsup_(85);
    }
/* 						!OVER YOU GO. */
    return ret_val;

/* R31--	BEACH ROOM.  DIG A HOLE. */

L31000:
    if (prsvec_1.prsa != vindex_1.digw || prsvec_1.prso != oindex_1.shove) {
	return ret_val;
    }
    ++findex_1.rvsnd;
/* 						!INCREMENT DIG STATE. */
    switch (findex_1.rvsnd) {
	case 1:  goto L31100;
	case 2:  goto L31100;
	case 3:  goto L31100;
	case 4:  goto L31400;
	case 5:  goto L31500;
    }
/* 						!PROCESS STATE. */
    bug_(2, findex_1.rvsnd);

L31100:
    i__1 = findex_1.rvsnd + 85;
    rspeak_(i__1);
/* 						!1-3... DISCOURAGE HIM. */
    return ret_val;

L31400:
    i = 89;
/* 						!ASSUME DISCOVERY. */
    if ((objcts_1.oflag1[oindex_1.statu - 1] & VISIBT) != 0) {
	i = 88;
    }
    rspeak_(i);
    objcts_1.oflag1[oindex_1.statu - 1] |= VISIBT;
    return ret_val;

L31500:
    findex_1.rvsnd = 0;
/* 						!5... SAND COLLAPSES */
    jigsup_(90);
/* 						!AND SO DOES HE. */
    return ret_val;

/* R32--	TCAVE ROOM.  DIG A HOLE IN GUANO. */

L32000:
    if (prsvec_1.prsa != vindex_1.digw || prsvec_1.prso != oindex_1.shove) {
	return ret_val;
    }
    i = 91;
/* 						!ASSUME NO GUANO. */
    if (! qhere_(oindex_1.guano, play_1.here)) {
	goto L32100;
    }
/* 						!IS IT HERE? */
/* Computing MIN */
    i__1 = 4, i__2 = findex_1.rvgua + 1;
    findex_1.rvgua = min(i__1,i__2);
/* 						!YES, SET NEW STATE. */
    i = findex_1.rvgua + 91;
/* 						!GET NASTY REMARK. */
L32100:
    rspeak_(i);
/* 						!DESCRIBE. */
    return ret_val;

/* R33--	FALLS ROOM */

L33000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(96);
/* 						!DESCRIBE. */
    i = 97;
/* 						!ASSUME NO RAINBOW. */
    if (findex_1.rainbf) {
	i = 98;
    }
/* 						!GOT ONE? */
    rspeak_(i);
/* 						!DESCRIBE. */
    return ret_val;
/* RAPPL1, PAGE 10 */

/* R34--	LEDGE FUNCTION.  LEDGE CAN COLLAPSE. */

L34000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(100);
/* 						!DESCRIBE. */
    i = 102;
/* 						!ASSUME SAFE ROOM OK. */
    if ((rooms_1.rflag[rindex_1.msafe - 1] & RMUNG) != 0) {
	i = 101;
    }
    rspeak_(i);
/* 						!DESCRIBE. */
    return ret_val;

/* R35--	SAFE ROOM.  STATE DEPENDS ON WHETHER SAFE BLOWN. */

L35000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(104);
/* 						!DESCRIBE. */
    i = 105;
/* 						!ASSUME OK. */
    if (findex_1.safef) {
	i = 106;
    }
/* 						!BLOWN? */
    rspeak_(i);
/* 						!DESCRIBE. */
    return ret_val;

/* R36--	MAGNET ROOM.  DESCRIBE, CHECK FOR SPINDIZZY DOOM. */

L36000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	goto L36500;
    }
/* 						!LOOK? */
    rspeak_(107);
/* 						!DESCRIBE. */
    return ret_val;

L36500:
    if (prsvec_1.prsa != vindex_1.walkiw || ! findex_1.caroff) {
	return ret_val;
    }
/* 						!WALKIN ON FLIPPED? */
    if (findex_1.carozf) {
	goto L36600;
    }
/* 						!ZOOM? */
    rspeak_(108);
/* 						!NO, SPIN HIS COMPASS. */
    return ret_val;

L36600:
    i = 58;
/* 						!SPIN HIS INSIDES. */
    if (play_1.winner != aindex_1.player) {
	i = 99;
    }
/* 						!SPIN ROBOT. */
    jigsup_(i);
/* 						!DEAD. */
    return ret_val;

/* R37--	CAGE ROOM.  IF SOLVED CAGE, MOVE TO OTHER CAGE ROOM. */

L37000:
    if (findex_1.cagesf) {
	f = moveto_(rindex_1.cager, play_1.winner);
    }
/* 						!IF SOLVED, MOVE. */
    return ret_val;

} /* rappl1_ */
