/* CEVAPP- CLOCK EVENT APPLICABLES */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

static void litint_ P((integer, integer *, integer, const integer *,
		       integer));

void cevapp_(ri)
integer ri;
{
    /* Initialized data */

    static const integer cndtck[10] = { 50,20,10,5,0,156,156,156,157,0 };
    static const integer lmptck[12] = { 50,30,20,10,4,0,154,154,154,154,155,0 };

    /* System generated locals */
    integer i__1, i__2;

    /* Local variables */
    logical f;
    integer i, j, bc, br;

    if (ri == 0) {
	return;
    }
/* 						!IGNORE DISABLED. */
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
    }
    bug_(3, ri);

/* CEV1--	CURE CLOCK.  LET PLAYER SLOWLY RECOVER. */

L1000:
/* Computing MIN */
    i__1 = 0, i__2 = advs_1.astren[aindex_1.player - 1] + 1;
    advs_1.astren[aindex_1.player - 1] = min(i__1,i__2);
/* 						!RECOVER. */
    if (advs_1.astren[aindex_1.player - 1] >= 0) {
	return;
    }
/* 						!FULLY RECOVERED? */
    cevent_1.ctick[cindex_1.cevcur - 1] = 30;
/* 						!NO, WAIT SOME MORE. */
    return;

/* CEV2--	MAINT-ROOM WITH LEAK.  RAISE THE WATER LEVEL. */

L2000:
    if (play_1.here == rindex_1.maint) {
	i__1 = findex_1.rvmnt / 2 + 71;
	rspeak_(i__1);
    }
/* 						!DESCRIBE. */
    ++findex_1.rvmnt;
/* 						!RAISE WATER LEVEL. */
    if (findex_1.rvmnt <= 16) {
	return;
    }
/* 						!IF NOT FULL, EXIT. */
    cevent_1.ctick[cindex_1.cevmnt - 1] = 0;
/* 						!FULL, DISABLE CLOCK. */
    rooms_1.rflag[rindex_1.maint - 1] |= RMUNG;
    rrand[rindex_1.maint - 1] = 80;
/* 						!SAY IT IS FULL OF WATER. */
    if (play_1.here == rindex_1.maint) {
	jigsup_(81);
    }
/* 						!DROWN HIM IF PRESENT. */
    return;

/* CEV3--	LANTERN.  DESCRIBE GROWING DIMNESS. */

L3000:
    litint_(oindex_1.lamp, &findex_1.orlamp, cindex_1.cevlnt, lmptck, 12);
/* 						!DO LIGHT INTERRUPT. */
    return;

/* CEV4--	MATCH.  OUT IT GOES. */

L4000:
    rspeak_(153);
/* 						!MATCH IS OUT. */
    objcts_1.oflag1[oindex_1.match - 1] &= ~ ONBT;
    return;

/* CEV5--	CANDLE.  DESCRIBE GROWING DIMNESS. */

L5000:
    litint_(oindex_1.candl, &findex_1.orcand, cindex_1.cevcnd, cndtck, 10);
/* 						!DO CANDLE INTERRUPT. */
    return;
/* CEVAPP, PAGE 3 */

/* CEV6--	BALLOON */

L6000:
    cevent_1.ctick[cindex_1.cevbal - 1] = 3;
/* 						!RESCHEDULE INTERRUPT. */
    f = advs_1.avehic[play_1.winner - 1] == oindex_1.ballo;
/* 						!SEE IF IN BALLOON. */
    if (state_1.bloc == rindex_1.vlbot) {
	goto L6800;
    }
/* 						!AT BOTTOM? */
    if (state_1.bloc == rindex_1.ledg2 || state_1.bloc == rindex_1.ledg3 || 
	    state_1.bloc == rindex_1.ledg4 || state_1.bloc == rindex_1.vlbot) 
	    {
	goto L6700;
    }
/* 						!ON LEDGE? */
    if ((objcts_1.oflag2[oindex_1.recep - 1] & OPENBT) != 0 && 
	    findex_1.binff != 0) {
	goto L6500;
    }

/* BALLOON IS IN MIDAIR AND IS DEFLATED (OR HAS RECEPTACLE CLOSED). */
/* FALL TO NEXT ROOM. */

    if (state_1.bloc != rindex_1.vair1) {
	goto L6300;
    }
/* 						!IN VAIR1? */
    state_1.bloc = rindex_1.vlbot;
/* 						!YES, NOW AT VLBOT. */
    newsta_(oindex_1.ballo, 0, state_1.bloc, 0, 0);
    if (f) {
	goto L6200;
    }
/* 						!IN BALLOON? */
    if (play_1.here == rindex_1.ledg2 || play_1.here == rindex_1.ledg3 || 
	    play_1.here == rindex_1.ledg4 || play_1.here == rindex_1.vlbot) {
	rspeak_(530);
    }
/* 						!ON LEDGE, DESCRIBE. */
    return;

L6200:
    f = moveto_(state_1.bloc, play_1.winner);
/* 						!MOVE HIM. */
    if (findex_1.binff == 0) {
	goto L6250;
    }
/* 						!IN BALLOON.  INFLATED? */
    rspeak_(531);
/* 						!YES, LANDED. */
    f = rmdesc_(0);
/* 						!DESCRIBE. */
    return;

L6250:
    newsta_(oindex_1.ballo, 532, 0, 0, 0);
/* 						!NO, BALLOON & CONTENTS DIE. */
    newsta_(oindex_1.dball, 0, state_1.bloc, 0, 0);
/* 						!INSERT DEAD BALLOON. */
    advs_1.avehic[play_1.winner - 1] = 0;
/* 						!NOT IN VEHICLE. */
    cevent_1.cflag[cindex_1.cevbal - 1] = FALSE_;
/* 						!DISABLE INTERRUPTS. */
    cevent_1.cflag[cindex_1.cevbrn - 1] = FALSE_;
    findex_1.binff = 0;
    findex_1.btief = 0;
    return;

L6300:
    --state_1.bloc;
/* 						!NOT IN VAIR1, DESCEND. */
    newsta_(oindex_1.ballo, 0, state_1.bloc, 0, 0);
    if (f) {
	goto L6400;
    }
/* 						!IS HE IN BALLOON? */
    if (play_1.here == rindex_1.ledg2 || play_1.here == rindex_1.ledg3 || 
	    play_1.here == rindex_1.ledg4 || play_1.here == rindex_1.vlbot) {
	rspeak_(533);
    }
/* 						!IF ON LEDGE, DESCRIBE. */
    return;

L6400:
    f = moveto_(state_1.bloc, play_1.winner);
/* 						!IN BALLOON, MOVE HIM. */
    rspeak_(534);
/* 						!DESCRIBE. */
    f = rmdesc_(0);
    return;

/* BALLOON IS IN MIDAIR AND IS INFLATED, UP-UP-AND-AWAY */
/* 						! */

L6500:
    if (state_1.bloc != rindex_1.vair4) {
	goto L6600;
    }
/* 						!AT VAIR4? */
    cevent_1.ctick[cindex_1.cevbrn - 1] = 0;
    cevent_1.ctick[cindex_1.cevbal - 1] = 0;
    findex_1.binff = 0;
    findex_1.btief = 0;
    state_1.bloc = rindex_1.vlbot;
/* 						!FALL TO BOTTOM. */
    newsta_(oindex_1.ballo, 0, 0, 0, 0);
/* 						!BALLOON & CONTENTS DIE. */
    newsta_(oindex_1.dball, 0, state_1.bloc, 0, 0);
/* 						!SUBSTITUTE DEAD BALLOON. */
    if (f) {
	goto L6550;
    }
/* 						!WAS HE IN IT? */
    if (play_1.here == rindex_1.ledg2 || play_1.here == rindex_1.ledg3 || 
	    play_1.here == rindex_1.ledg4 || play_1.here == rindex_1.vlbot) {
	rspeak_(535);
    }
/* 						!IF HE CAN SEE, DESCRIBE. */
    return;

L6550:
    jigsup_(536);
/* 						!IN BALLOON AT CRASH, DIE. */
    return;

L6600:
    ++state_1.bloc;
/* 						!NOT AT VAIR4, GO UP. */
    newsta_(oindex_1.ballo, 0, state_1.bloc, 0, 0);
    if (f) {
	goto L6650;
    }
/* 						!IN BALLOON? */
    if (play_1.here == rindex_1.ledg2 || play_1.here == rindex_1.ledg3 || 
	    play_1.here == rindex_1.ledg4 || play_1.here == rindex_1.vlbot) {
	rspeak_(537);
    }
/* 						!CAN HE SEE IT? */
    return;

L6650:
    f = moveto_(state_1.bloc, play_1.winner);
/* 						!MOVE PLAYER. */
    rspeak_(538);
/* 						!DESCRIBE. */
    f = rmdesc_(0);
    return;

/* ON LEDGE, GOES TO MIDAIR ROOM WHETHER INFLATED OR NOT. */

L6700:
    state_1.bloc += rindex_1.vair2 - rindex_1.ledg2;
/* 						!MOVE TO MIDAIR. */
    newsta_(oindex_1.ballo, 0, state_1.bloc, 0, 0);
    if (f) {
	goto L6750;
    }
/* 						!IN BALLOON? */
    if (play_1.here == rindex_1.ledg2 || play_1.here == rindex_1.ledg3 || 
	    play_1.here == rindex_1.ledg4 || play_1.here == rindex_1.vlbot) {
	rspeak_(539);
    }
/* 						!NO, STRANDED. */
    cevent_1.ctick[cindex_1.cevvlg - 1] = 10;
/* 						!MATERIALIZE GNOME. */
    return;

L6750:
    f = moveto_(state_1.bloc, play_1.winner);
/* 						!MOVE TO NEW ROOM. */
    rspeak_(540);
/* 						!DESCRIBE. */
    f = rmdesc_(0);
    return;

/* AT BOTTOM, GO UP IF INFLATED, DO NOTHING IF DEFLATED. */

L6800:
    if (findex_1.binff == 0 || ! ((objcts_1.oflag2[oindex_1.recep - 1] & 
	    OPENBT) != 0)) {
	return;
    }
    state_1.bloc = rindex_1.vair1;
/* 						!INFLATED AND OPEN, */
    newsta_(oindex_1.ballo, 0, state_1.bloc, 0, 0);
/* 						!GO UP TO VAIR1. */
    if (f) {
	goto L6850;
    }
/* 						!IN BALLOON? */
    if (play_1.here == rindex_1.ledg2 || play_1.here == rindex_1.ledg3 || 
	    play_1.here == rindex_1.ledg4 || play_1.here == rindex_1.vlbot) {
	rspeak_(541);
    }
/* 						!IF CAN SEE, DESCRIBE. */
    return;

L6850:
    f = moveto_(state_1.bloc, play_1.winner);
/* 						!MOVE PLAYER. */
    rspeak_(542);
    f = rmdesc_(0);
    return;
/* CEVAPP, PAGE 4 */

/* CEV7--	BALLOON BURNUP */

L7000:
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!FIND BURNING OBJECT */
	if (oindex_1.recep == objcts_1.ocan[i - 1] && (objcts_1.oflag1[i - 1] 
		& FLAMBT) != 0) {
	    goto L7200;
	}
/* L7100: */
    }
    bug_(4, 0);

L7200:
    newsta_(i, 0, 0, 0, 0);
/* 						!VANISH OBJECT. */
    findex_1.binff = 0;
/* 						!UNINFLATED. */
    if (play_1.here == state_1.bloc) {
	rspsub_(292, objcts_1.odesc2[i - 1]);
    }
/* 						!DESCRIBE. */
    return;

/* CEV8--	FUSE FUNCTION */

L8000:
    if (objcts_1.ocan[oindex_1.fuse - 1] != oindex_1.brick) {
	goto L8500;
    }
/* 						!IGNITED BRICK? */
    br = objcts_1.oroom[oindex_1.brick - 1];
/* 						!GET BRICK ROOM. */
    bc = objcts_1.ocan[oindex_1.brick - 1];
/* 						!GET CONTAINER. */
    if (br == 0 && bc != 0) {
	br = objcts_1.oroom[bc - 1];
    }
    newsta_(oindex_1.fuse, 0, 0, 0, 0);
/* 						!KILL FUSE. */
    newsta_(oindex_1.brick, 0, 0, 0, 0);
/* 						!KILL BRICK. */
    if (br != 0 && br != play_1.here) {
	goto L8100;
    }
/* 						!BRICK ELSEWHERE? */

    rooms_1.rflag[play_1.here - 1] |= RMUNG;
    rrand[play_1.here - 1] = 114;
/* 						!MUNG ROOM. */
    jigsup_(150);
/* 						!DEAD. */
    return;

L8100:
    rspeak_(151);
/* 						!BOOM. */
    state_1.mungrm = br;
/* 						!SAVE ROOM THAT BLEW. */
    cevent_1.ctick[cindex_1.cevsaf - 1] = 5;
/* 						!SET SAFE INTERRUPT. */
    if (br != rindex_1.msafe) {
	goto L8200;
    }
/* 						!BLEW SAFE ROOM? */
    if (bc != oindex_1.sslot) {
	return;
    }
/* 						!WAS BRICK IN SAFE? */
    newsta_(oindex_1.sslot, 0, 0, 0, 0);
/* 						!KILL SLOT. */
    objcts_1.oflag2[oindex_1.safe - 1] |= OPENBT;
    findex_1.safef = TRUE_;
/* 						!INDICATE SAFE BLOWN. */
    return;

L8200:
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!BLEW WRONG ROOM. */
	if (qhere_(i, br) && (objcts_1.oflag1[i - 1] & TAKEBT) != 
		0) {
	    newsta_(i, 0, 0, 0, 0);
	}
/* L8250: */
    }
    if (br != rindex_1.lroom) {
	return;
    }
/* 						!BLEW LIVING ROOM? */
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
	if (objcts_1.ocan[i - 1] == oindex_1.tcase) {
	    newsta_(i, 0, 0, 0, 0);
	}
/* 						!KILL TROPHY CASE. */
/* L8300: */
    }
    return;

L8500:
    if (qhere_(oindex_1.fuse, play_1.here) || objcts_1.oadv[oindex_1.fuse - 
	    1] == play_1.winner) {
	rspeak_(152);
    }
    newsta_(oindex_1.fuse, 0, 0, 0, 0);
/* 						!KILL FUSE. */
    return;
/* CEVAPP, PAGE 5 */

/* CEV9--	LEDGE MUNGE. */

L9000:
    rooms_1.rflag[rindex_1.ledg4 - 1] |= RMUNG;
    rrand[rindex_1.ledg4 - 1] = 109;
    if (play_1.here == rindex_1.ledg4) {
	goto L9100;
    }
/* 						!WAS HE THERE? */
    rspeak_(110);
/* 						!NO, NARROW ESCAPE. */
    return;

L9100:
    if (advs_1.avehic[play_1.winner - 1] != 0) {
	goto L9200;
    }
/* 						!IN VEHICLE? */
    jigsup_(111);
/* 						!NO, DEAD. */
    return;

L9200:
    if (findex_1.btief != 0) {
	goto L9300;
    }
/* 						!TIED TO LEDGE? */
    rspeak_(112);
/* 						!NO, NO PLACE TO LAND. */
    return;

L9300:
    state_1.bloc = rindex_1.vlbot;
/* 						!YES, CRASH BALLOON. */
    newsta_(oindex_1.ballo, 0, 0, 0, 0);
/* 						!BALLOON & CONTENTS DIE. */
    newsta_(oindex_1.dball, 0, state_1.bloc, 0, 0);
/* 						!INSERT DEAD BALLOON. */
    findex_1.btief = 0;
    findex_1.binff = 0;
    cevent_1.cflag[cindex_1.cevbal - 1] = FALSE_;
    cevent_1.cflag[cindex_1.cevbrn - 1] = FALSE_;
    jigsup_(113);
/* 						!DEAD */
    return;

/* CEV10--	SAFE MUNG. */

L10000:
    rooms_1.rflag[state_1.mungrm - 1] |= RMUNG;
    rrand[state_1.mungrm - 1] = 114;
    if (play_1.here == state_1.mungrm) {
	goto L10100;
    }
/* 						!IS HE PRESENT? */
    rspeak_(115);
/* 						!LET HIM KNOW. */
    if (state_1.mungrm == rindex_1.msafe) {
	cevent_1.ctick[cindex_1.cevled - 1] = 8;
    }
/* 						!START LEDGE CLOCK. */
    return;

L10100:
    i = 116;
/* 						!HE'S DEAD, */
    if ((rooms_1.rflag[play_1.here - 1] & RHOUSE) != 0) {
	i = 117;
    }
    jigsup_(i);
/* 						!LET HIM KNOW. */
    return;
/* CEVAPP, PAGE 6 */

/* CEV11--	VOLCANO GNOME */

L11000:
    if (play_1.here == rindex_1.ledg2 || play_1.here == rindex_1.ledg3 || 
	    play_1.here == rindex_1.ledg4 || play_1.here == rindex_1.vlbot) {
	goto L11100;
    }
/* 						!IS HE ON LEDGE? */
    cevent_1.ctick[cindex_1.cevvlg - 1] = 1;
/* 						!NO, WAIT A WHILE. */
    return;

L11100:
    newsta_(oindex_1.gnome, 118, play_1.here, 0, 0);
/* 						!YES, MATERIALIZE GNOME. */
    return;

/* CEV12--	VOLCANO GNOME DISAPPEARS */

L12000:
    newsta_(oindex_1.gnome, 149, 0, 0, 0);
/* 						!DISAPPEAR THE GNOME. */
    return;

/* CEV13--	BUCKET. */

L13000:
    if (objcts_1.ocan[oindex_1.water - 1] == oindex_1.bucke) {
	newsta_(oindex_1.water, 0, 0, 0, 0);
    }
    return;

/* CEV14--	SPHERE.  IF EXPIRES, HE'S TRAPPED. */

L14000:
    rooms_1.rflag[rindex_1.cager - 1] |= RMUNG;
    rrand[rindex_1.cager - 1] = 147;
    jigsup_(148);
/* 						!MUNG PLAYER. */
    return;

/* CEV15--	END GAME HERALD. */

L15000:
    findex_1.endgmf = TRUE_;
/* 						!WE'RE IN ENDGAME. */
    rspeak_(119);
/* 						!INFORM OF ENDGAME. */
    return;
/* CEVAPP, PAGE 7 */

/* CEV16--	FOREST MURMURS */

L16000:
    cevent_1.cflag[cindex_1.cevfor - 1] = play_1.here == rindex_1.mtree || 
	    play_1.here >= rindex_1.fore1 && play_1.here < rindex_1.clear;
    if (cevent_1.cflag[cindex_1.cevfor - 1] && prob_(10, 10)) {
	rspeak_(635);
    }
    return;

/* CEV17--	SCOL ALARM */

L17000:
    if (play_1.here == rindex_1.bktwi) {
	cevent_1.cflag[cindex_1.cevzgi - 1] = TRUE_;
    }
/* 						!IF IN TWI, GNOME. */
    if (play_1.here == rindex_1.bkvau) {
	jigsup_(636);
    }
/* 						!IF IN VAU, DEAD. */
    return;

/* CEV18--	ENTER GNOME OF ZURICH */

L18000:
    cevent_1.cflag[cindex_1.cevzgo - 1] = TRUE_;
/* 						!EXITS, TOO. */
    newsta_(oindex_1.zgnom, 0, rindex_1.bktwi, 0, 0);
/* 						!PLACE IN TWI. */
    if (play_1.here == rindex_1.bktwi) {
	rspeak_(637);
    }
/* 						!ANNOUNCE. */
    return;

/* CEV19--	EXIT GNOME */

L19000:
    newsta_(oindex_1.zgnom, 0, 0, 0, 0);
/* 						!VANISH. */
    if (play_1.here == rindex_1.bktwi) {
	rspeak_(638);
    }
/* 						!ANNOUNCE. */
    return;
/* CEVAPP, PAGE 8 */

/* CEV20--	START OF ENDGAME */

L20000:
    if (findex_1.spellf) {
	goto L20200;
    }
/* 						!SPELL HIS WAY IN? */
    if (play_1.here != rindex_1.crypt) {
	return;
    }
/* 						!NO, STILL IN TOMB? */
    if (! lit_(play_1.here)) {
	goto L20100;
    }
/* 						!LIGHTS OFF? */
    cevent_1.ctick[cindex_1.cevste - 1] = 3;
/* 						!RESCHEDULE. */
    return;

L20100:
    rspeak_(727);
/* 						!ANNOUNCE. */
L20200:
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!STRIP HIM OF OBJS. */
	newsta_(i, 0, objcts_1.oroom[i - 1], objcts_1.ocan[i - 1], 
		0);
/* L20300: */
    }
    newsta_(oindex_1.lamp, 0, 0, 0, aindex_1.player);
/* 						!GIVE HIM LAMP. */
    newsta_(oindex_1.sword, 0, 0, 0, aindex_1.player);
/* 						!GIVE HIM SWORD. */

    objcts_1.oflag1[oindex_1.lamp - 1] = (objcts_1.oflag1[oindex_1.lamp - 1] |
	     LITEBT) & ~ ONBT;
    objcts_1.oflag2[oindex_1.lamp - 1] |= TCHBT;
    cevent_1.cflag[cindex_1.cevlnt - 1] = FALSE_;
/* 						!LAMP IS GOOD AS NEW. */
    cevent_1.ctick[cindex_1.cevlnt - 1] = 350;
    findex_1.orlamp = 0;
    objcts_1.oflag2[oindex_1.sword - 1] |= TCHBT;
    hack_1.swdact = TRUE_;
    hack_1.swdsta = 0;

    hack_1.thfact = FALSE_;
/* 						!THIEF GONE. */
    findex_1.endgmf = TRUE_;
/* 						!ENDGAME RUNNING. */
    cevent_1.cflag[cindex_1.cevmat - 1] = FALSE_;
/* 						!MATCHES GONE, */
    cevent_1.cflag[cindex_1.cevcnd - 1] = FALSE_;
/* 						!CANDLES GONE. */

    scrupd_(rooms_1.rval[rindex_1.crypt - 1]);
/* 						!SCORE CRYPT, */
    rooms_1.rval[rindex_1.crypt - 1] = 0;
/* 						!BUT ONLY ONCE. */
    f = moveto_(rindex_1.tstrs, play_1.winner);
/* 						!TO TOP OF STAIRS, */
    f = rmdesc_(3);
/* 						!AND DESCRIBE. */
    return;
/* 						!BAM */
/* 						! */

/* CEV21--	MIRROR CLOSES. */

L21000:
    findex_1.mrpshf = FALSE_;
/* 						!BUTTON IS OUT. */
    findex_1.mropnf = FALSE_;
/* 						!MIRROR IS CLOSED. */
    if (play_1.here == rindex_1.mrant) {
	rspeak_(728);
    }
/* 						!DESCRIBE BUTTON. */
    if (play_1.here == rindex_1.inmir || mrhere_(play_1.here) == 1) {
	rspeak_(729);
    }
    return;
/* CEVAPP, PAGE 9 */

/* CEV22--	DOOR CLOSES. */

L22000:
    if (findex_1.wdopnf) {
	rspeak_(730);
    }
/* 						!DESCRIBE. */
    findex_1.wdopnf = FALSE_;
/* 						!CLOSED. */
    return;

/* CEV23--	INQUISITOR'S QUESTION */

L23000:
    if (advs_1.aroom[aindex_1.player - 1] != rindex_1.fdoor) {
	return;
    }
/* 						!IF PLAYER LEFT, DIE. */
    rspeak_(769);
    i__1 = findex_1.quesno + 770;
    rspeak_(i__1);
    cevent_1.ctick[cindex_1.cevinq - 1] = 2;
    return;

/* CEV24--	MASTER FOLLOWS */

L24000:
    if (advs_1.aroom[aindex_1.amastr - 1] == play_1.here) {
	return;
    }
/* 						!NO MOVEMENT, DONE. */
    if (play_1.here != rindex_1.cell && play_1.here != rindex_1.pcell) {
	goto L24100;
    }
    if (findex_1.follwf) {
	rspeak_(811);
    }
/* 						!WONT GO TO CELLS. */
    findex_1.follwf = FALSE_;
    return;

L24100:
    findex_1.follwf = TRUE_;
/* 						!FOLLOWING. */
    i = 812;
/* 						!ASSUME CATCHES UP. */
    i__1 = xsrch_1.xmax;
    i__2 = xsrch_1.xmin;
    for (j = xsrch_1.xmin; i__2 < 0 ? j >= i__1 : j <= i__1; j += i__2) {
	if (findxt_(j, advs_1.aroom[aindex_1.amastr - 1]) && curxt_1.xroom1 
		== play_1.here) {
	    i = 813;
	}
/* L24200: */
    }
    rspeak_(i);
    newsta_(oindex_1.master, 0, play_1.here, 0, 0);
/* 						!MOVE MASTER OBJECT. */
    advs_1.aroom[aindex_1.amastr - 1] = play_1.here;
/* 						!MOVE MASTER PLAYER. */
    return;

} /* cevapp_ */

/* LITINT-	LIGHT INTERRUPT PROCESSOR */

/* DECLARATIONS */

static void litint_(obj, ctr, cev, ticks, tickln)
integer obj;
integer *ctr;
integer cev;
const integer *ticks;
integer tickln;
{
    /* Parameter adjustments */
    --ticks;

    /* Function Body */
    ++(*ctr);
/* 						!ADVANCE STATE CNTR. */
    cevent_1.ctick[cev - 1] = ticks[*ctr];
/* 						!RESET INTERRUPT. */
    if (cevent_1.ctick[cev - 1] != 0) {
	goto L100;
    }
/* 						!EXPIRED? */
    objcts_1.oflag1[obj - 1] &= ~ (LITEBT + FLAMBT + 
	    ONBT);
    if (objcts_1.oroom[obj - 1] == play_1.here || objcts_1.oadv[obj - 1] == 
	    play_1.winner) {
	rspsub_(293, objcts_1.odesc2[obj - 1]);
    }
    return;

L100:
    if (objcts_1.oroom[obj - 1] == play_1.here || objcts_1.oadv[obj - 1] == 
	    play_1.winner) {
	rspeak_(ticks[*ctr + tickln / 2]);
    }
    return;

} /* litint_ */
