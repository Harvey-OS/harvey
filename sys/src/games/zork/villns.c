/* TROLLP-	TROLL FUNCTION */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

logical trollp_(arg)
integer arg;
{
    /* System generated locals */
    logical ret_val;

    /* Local variables */
    integer i;

    ret_val = TRUE_;
/* 						!ASSUME WINS. */
    if (prsvec_1.prsa != vindex_1.fightw) {
	goto L1100;
    }
/* 						!FIGHT? */
    if (objcts_1.ocan[oindex_1.axe - 1] == oindex_1.troll) {
	goto L10;
    }
/* 						!GOT AXE?  NOTHING. */
    i = 433;
/* 						!ASSUME CANT GET. */
    if (! qhere_(oindex_1.axe, play_1.here)) {
	goto L1050;
    }
/* 						!HERE? */
    i = 434;
/* 						!YES, RECOVER. */
    newsta_(oindex_1.axe, 0, 0, oindex_1.troll, 0);
L1050:
    if (qhere_(oindex_1.troll, play_1.here)) {
	rspeak_(i);
    }
/* 						!IF PLAYER HERE. */
    return ret_val;

L1100:
    if (prsvec_1.prsa != vindex_1.deadxw) {
	goto L1200;
    }
/* 						!DEAD? */
    findex_1.trollf = TRUE_;
/* 						!PERMIT EXITS. */
    return ret_val;

L1200:
    if (prsvec_1.prsa != vindex_1.outxw) {
	goto L1300;
    }
/* 						!OUT? */
    findex_1.trollf = TRUE_;
/* 						!PERMIT EXITS. */
    objcts_1.oflag1[oindex_1.axe - 1] &= ~ VISIBT;
    objcts_1.odesc1[oindex_1.troll - 1] = 435;
/* 						!TROLL OUT. */
    return ret_val;

L1300:
    if (prsvec_1.prsa != vindex_1.inxw) {
	goto L1400;
    }
/* 						!WAKE UP? */
    findex_1.trollf = FALSE_;
/* 						!FORBID EXITS. */
    objcts_1.oflag1[oindex_1.axe - 1] |= VISIBT;
    objcts_1.odesc1[oindex_1.troll - 1] = 436;
/* 						!TROLL IN. */
    if (qhere_(oindex_1.troll, play_1.here)) {
	rspeak_(437);
    }
    return ret_val;

L1400:
    if (prsvec_1.prsa != vindex_1.frstqw) {
	goto L1500;
    }
/* 						!FIRST ENCOUNTER? */
    ret_val = prob_(33, 66);
/* 						!33% TRUE UNLESS BADLK. */
    return ret_val;

L1500:
    if (prsvec_1.prsa != vindex_1.movew && prsvec_1.prsa != vindex_1.takew && 
	    prsvec_1.prsa != vindex_1.mungw && prsvec_1.prsa != 
	    vindex_1.throww && prsvec_1.prsa != vindex_1.givew) {
	goto L2000;
    }
    if (objcts_1.ocapac[oindex_1.troll - 1] >= 0) {
	goto L1550;
    }
/* 						!TROLL OUT? */
    objcts_1.ocapac[oindex_1.troll - 1] = -objcts_1.ocapac[oindex_1.troll - 1]
	    ;
/* 						!YES, WAKE HIM. */
    objcts_1.oflag1[oindex_1.axe - 1] |= VISIBT;
    findex_1.trollf = FALSE_;
    objcts_1.odesc1[oindex_1.troll - 1] = 436;
    rspeak_(437);

L1550:
    if (prsvec_1.prsa != vindex_1.takew && prsvec_1.prsa != vindex_1.movew) {
	goto L1600;
    }
    rspeak_(438);
/* 						!JOKE. */
    return ret_val;

L1600:
    if (prsvec_1.prsa != vindex_1.mungw) {
	goto L1700;
    }
/* 						!MUNG? */
    rspeak_(439);
/* 						!JOKE. */
    return ret_val;

L1700:
    if (prsvec_1.prso == 0) {
	goto L10;
    }
/* 						!NO OBJECT? */
    i = 440;
/* 						!ASSUME THROW. */
    if (prsvec_1.prsa == vindex_1.givew) {
	i = 441;
    }
/* 						!GIVE? */
    rspsub_(i, objcts_1.odesc2[prsvec_1.prso - 1]);
/* 						!TROLL TAKES. */
    if (prsvec_1.prso == oindex_1.knife) {
	goto L1900;
    }
/* 						!OBJ KNIFE? */
    newsta_(prsvec_1.prso, 442, 0, 0, 0);
/* 						!NO, EATS IT. */
    return ret_val;

L1900:
    rspeak_(443);
/* 						!KNIFE, THROWS IT BACK */
    objcts_1.oflag2[oindex_1.troll - 1] |= FITEBT;
    return ret_val;

L2000:
    if (! findex_1.trollf || prsvec_1.prsa != vindex_1.hellow) {
	goto L10;
    }
    rspeak_(366);
/* 						!TROLL OUT. */
    return ret_val;

L10:
    ret_val = FALSE_;
/* 						!COULDNT HANDLE IT. */
    return ret_val;
} /* trollp_ */

/* CYCLOP-	CYCLOPS FUNCTION */

/* DECLARATIONS */

logical cyclop_(arg)
integer arg;
{
    /* System generated locals */
    integer i__1, i__2;
    logical ret_val;

    /* Local variables */
    integer i;

    ret_val = TRUE_;
/* 						!ASSUME WINS. */
    if (! findex_1.cyclof) {
	goto L100;
    }
/* 						!ASLEEP? */
    if (prsvec_1.prsa != vindex_1.alarmw && prsvec_1.prsa != vindex_1.mungw &&
	     prsvec_1.prsa != vindex_1.hellow && prsvec_1.prsa != 
	    vindex_1.burnw && prsvec_1.prsa != vindex_1.killw && 
	    prsvec_1.prsa != vindex_1.attacw) {
	goto L10;
    }
    findex_1.cyclof = FALSE_;
/* 						!WAKE CYCLOPS. */
    rspeak_(187);
/* 						!DESCRIBE. */
    findex_1.rvcyc = abs(findex_1.rvcyc);
    objcts_1.oflag2[oindex_1.cyclo - 1] = (objcts_1.oflag2[oindex_1.cyclo - 1]
	     | FITEBT) & ~ SLEPBT;
    return ret_val;

L100:
    if (prsvec_1.prsa == vindex_1.fightw || prsvec_1.prsa == vindex_1.frstqw) 
	    {
	goto L10;
    }
    if (abs(findex_1.rvcyc) <= 5) {
	goto L200;
    }
/* 						!ANNOYED TOO MUCH? */
    findex_1.rvcyc = 0;
/* 						!RESTART COUNT. */
    jigsup_(188);
/* 						!YES, EATS PLAYER. */
    return ret_val;

L200:
    if (prsvec_1.prsa != vindex_1.givew) {
	goto L500;
    }
/* 						!GIVE? */
    if (prsvec_1.prso != oindex_1.food || findex_1.rvcyc < 0) {
	goto L300;
    }
/* 						!FOOD WHEN HUNGRY? */
    newsta_(oindex_1.food, 189, 0, 0, 0);
/* 						!EATS PEPPERS. */
/* Computing MIN */
    i__1 = -1, i__2 = -findex_1.rvcyc;
    findex_1.rvcyc = min(i__1,i__2);
/* 						!GETS THIRSTY. */
    return ret_val;

L300:
    if (prsvec_1.prso != oindex_1.water) {
	goto L400;
    }
/* 						!DRINK WHEN THIRSTY? */
    if (findex_1.rvcyc >= 0) {
	goto L350;
    }
    newsta_(prsvec_1.prso, 190, 0, 0, 0);
/* 						!DRINKS AND */
    findex_1.cyclof = TRUE_;
/* 						!FALLS ASLEEP. */
    objcts_1.oflag2[oindex_1.cyclo - 1] = (objcts_1.oflag2[oindex_1.cyclo - 1]
	     | SLEPBT) & ~ FITEBT;
    return ret_val;

L350:
    rspeak_(191);
/* 						!NOT THIRSTY. */
L10:
    ret_val = FALSE_;
/* 						!FAILS. */
    return ret_val;

L400:
    i = 192;
/* 						!ASSUME INEDIBLE. */
    if (prsvec_1.prso == oindex_1.garli) {
	i = 193;
    }
/* 						!GARLIC IS JOKE. */
L450:
    rspeak_(i);
/* 						!DISDAIN IT. */
    if (findex_1.rvcyc < 0) {
	--findex_1.rvcyc;
    }
    if (findex_1.rvcyc >= 0) {
	++findex_1.rvcyc;
    }
    if (! findex_1.cyclof) {
	i__1 = abs(findex_1.rvcyc) + 193;
	rspeak_(i__1);
    }
    return ret_val;

L500:
    i = 0;
/* 						!ASSUME NOT HANDLED. */
    if (prsvec_1.prsa == vindex_1.hellow) {
	goto L450;
    }
/* 						!HELLO IS NO GO. */
    if (prsvec_1.prsa == vindex_1.throww || prsvec_1.prsa == vindex_1.mungw) {

	i = rnd_(2) + 200;
    }
    if (prsvec_1.prsa == vindex_1.takew) {
	i = 202;
    }
    if (prsvec_1.prsa == vindex_1.tiew) {
	i = 203;
    }
    if (i <= 0) {
	goto L10;
    } else {
	goto L450;
    }
/* 						!SEE IF HANDLED. */

} /* cyclop_ */

/* THIEFP-	THIEF FUNCTION */

/* DECLARATIONS */

logical thiefp_(arg)
integer arg;
{
    /* System generated locals */
    integer i__1;
    logical ret_val;

    /* Local variables */
    integer i, j;

    ret_val = TRUE_;
/* 						!ASSUME WINS. */
    if (prsvec_1.prsa != vindex_1.fightw) {
	goto L100;
    }
/* 						!FIGHT? */
    if (objcts_1.ocan[oindex_1.still - 1] == oindex_1.thief) {
	goto L10;
    }
/* 						!GOT STILLETTO?  F. */
    if (qhere_(oindex_1.still, hack_1.thfpos)) {
	goto L50;
    }
/* 						!CAN HE RECOVER IT? */
    newsta_(oindex_1.thief, 0, 0, 0, 0);
/* 						!NO, VANISH. */
    if (qhere_(oindex_1.thief, play_1.here)) {
	rspeak_(498);
    }
/* 						!IF HERO, TELL. */
    return ret_val;

L50:
    newsta_(oindex_1.still, 0, 0, oindex_1.thief, 0);
/* 						!YES, RECOVER. */
    if (qhere_(oindex_1.thief, play_1.here)) {
	rspeak_(499);
    }
/* 						!IF HERO, TELL. */
    return ret_val;

L100:
    if (prsvec_1.prsa != vindex_1.deadxw) {
	goto L200;
    }
/* 						!DEAD? */
    hack_1.thfact = FALSE_;
/* 						!DISABLE DEMON. */
    objcts_1.oflag1[oindex_1.chali - 1] |= TAKEBT;
    j = 0;
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!CARRYING ANYTHING? */
/* L125: */
	if (objcts_1.oadv[i - 1] == -oindex_1.thief) {
	    j = 500;
	}
    }
    rspeak_(j);
/* 						!TELL IF BOOTY REAPPEARS. */

    j = 501;
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!LOOP. */
	if (i == oindex_1.chali || i == oindex_1.thief || play_1.here != 
		rindex_1.treas || ! qhere_(i, play_1.here)) {
	    goto L135;
	}
	objcts_1.oflag1[i - 1] |= VISIBT;
	rspsub_(j, objcts_1.odesc2[i - 1]);
/* 						!DESCRIBE. */
	j = 502;
	goto L150;

L135:
	if (objcts_1.oadv[i - 1] == -oindex_1.thief) {
	    newsta_(i, 0, play_1.here, 0, 0);
	}
L150:
	;
    }
    return ret_val;

L200:
    if (prsvec_1.prsa != vindex_1.frstqw) {
	goto L250;
    }
/* 						!FIRST ENCOUNTER? */
    ret_val = prob_(20, 75);
    return ret_val;

L250:
    if (prsvec_1.prsa != vindex_1.hellow || objcts_1.odesc1[oindex_1.thief - 
	    1] != 504) {
	goto L300;
    }
    rspeak_(626);
    return ret_val;

L300:
    if (prsvec_1.prsa != vindex_1.outxw) {
	goto L400;
    }
/* 						!OUT? */
    hack_1.thfact = FALSE_;
/* 						!DISABLE DEMON. */
    objcts_1.odesc1[oindex_1.thief - 1] = 504;
/* 						!CHANGE DESCRIPTION. */
    objcts_1.oflag1[oindex_1.still - 1] &= ~ VISIBT;
    objcts_1.oflag1[oindex_1.chali - 1] |= TAKEBT;
    return ret_val;

L400:
    if (prsvec_1.prsa != vindex_1.inxw) {
	goto L500;
    }
/* 						!IN? */
    if (qhere_(oindex_1.thief, play_1.here)) {
	rspeak_(505);
    }
/* 						!CAN HERO SEE? */
    hack_1.thfact = TRUE_;
/* 						!ENABLE DEMON. */
    objcts_1.odesc1[oindex_1.thief - 1] = 503;
/* 						!CHANGE DESCRIPTION. */
    objcts_1.oflag1[oindex_1.still - 1] |= VISIBT;
    if (play_1.here == rindex_1.treas && qhere_(oindex_1.chali, play_1.here)
	    ) {
	objcts_1.oflag1[oindex_1.chali - 1] &= ~ TAKEBT;
    }
    return ret_val;

L500:
    if (prsvec_1.prsa != vindex_1.takew) {
	goto L600;
    }
/* 						!TAKE? */
    rspeak_(506);
/* 						!JOKE. */
    return ret_val;

L600:
    if (prsvec_1.prsa != vindex_1.throww || prsvec_1.prso != oindex_1.knife ||
	     (objcts_1.oflag2[oindex_1.thief - 1] & FITEBT) != 0) {
	goto L700;
    }
    if (prob_(10, 10)) {
	goto L650;
    }
/* 						!THREW KNIFE, 10%? */
    rspeak_(507);
/* 						!NO, JUST MAKES */
    objcts_1.oflag2[oindex_1.thief - 1] |= FITEBT;
    return ret_val;

L650:
    j = 508;
/* 						!THIEF DROPS STUFF. */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
	if (objcts_1.oadv[i - 1] != -oindex_1.thief) {
	    goto L675;
	}
/* 						!THIEF CARRYING? */
	j = 509;
	newsta_(i, 0, play_1.here, 0, 0);
L675:
	;
    }
    newsta_(oindex_1.thief, j, 0, 0, 0);
/* 						!THIEF VANISHES. */
    return ret_val;

L700:
    if (prsvec_1.prsa != vindex_1.throww && prsvec_1.prsa != vindex_1.givew ||
	     prsvec_1.prso == 0 || prsvec_1.prso == oindex_1.thief) {
	goto L10;
    }
    if (objcts_1.ocapac[oindex_1.thief - 1] >= 0) {
	goto L750;
    }
/* 						!WAKE HIM UP. */
    objcts_1.ocapac[oindex_1.thief - 1] = -objcts_1.ocapac[oindex_1.thief - 1]
	    ;
    hack_1.thfact = TRUE_;
    objcts_1.oflag1[oindex_1.still - 1] |= VISIBT;
    objcts_1.odesc1[oindex_1.thief - 1] = 503;
    rspeak_(510);

L750:
    if (prsvec_1.prso != oindex_1.brick || objcts_1.ocan[oindex_1.fuse - 1] !=
	     oindex_1.brick || cevent_1.ctick[cindex_1.cevfus - 1] == 0) {
	goto L800;
    }
    rspeak_(511);
/* 						!THIEF REFUSES BOMB. */
    return ret_val;

L800:
    i__1 = -oindex_1.thief;
    newsta_(prsvec_1.prso, 0, 0, 0, i__1);
/* 						!THIEF TAKES GIFT. */
    if (objcts_1.otval[prsvec_1.prso - 1] > 0) {
	goto L900;
    }
/* 						!A TREASURE? */
    rspsub_(512, objcts_1.odesc2[prsvec_1.prso - 1]);
    return ret_val;

L900:
    rspsub_(627, objcts_1.odesc2[prsvec_1.prso - 1]);
/* 						!THIEF ENGROSSED. */
    findex_1.thfenf = TRUE_;
    return ret_val;

L10:
    ret_val = FALSE_;
    return ret_val;
} /* thiefp_ */
