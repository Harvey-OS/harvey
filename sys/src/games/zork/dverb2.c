/* SAVE- SAVE GAME STATE */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include <stdio.h>
#include "funcs.h"
#include "vars.h"

/* DECLARATIONS */

static integer cxappl_ P((integer));

void savegm_()
{
    /* Local variables */
    integer i;
    FILE *e;

    prsvec_1.prswon = FALSE_;
/* 						!DISABLE GAME. */
/* Note: save file format is different for PDP vs. non-PDP versions */

    if ((e = fopen("dsave.dat", BINWRITE)) == NULL)
	goto L100;

    gttime_(&i);
/* 						!GET TIME. */

#define do_uio(i, zbuf, cbytes) \
	(void) fwrite((const char *)(zbuf), (cbytes), (i), e)

    do_uio(1, &vers_1.vmaj, sizeof(integer));
    do_uio(1, &vers_1.vmin, sizeof(integer));
    do_uio(1, &vers_1.vedit, sizeof(integer));

    do_uio(1, &play_1.winner, sizeof(integer));
    do_uio(1, &play_1.here, sizeof(integer));
    do_uio(1, &hack_1.thfpos, sizeof(integer));
    do_uio(1, &play_1.telflg, sizeof(logical));
    do_uio(1, &hack_1.thfflg, sizeof(logical));
    do_uio(1, &hack_1.thfact, sizeof(logical));
    do_uio(1, &hack_1.swdact, sizeof(logical));
    do_uio(1, &hack_1.swdsta, sizeof(integer));
    do_uio(64, &puzzle_1.cpvec[0], sizeof(integer));

    do_uio(1, &i, sizeof(integer));
    do_uio(1, &state_1.moves, sizeof(integer));
    do_uio(1, &state_1.deaths, sizeof(integer));
    do_uio(1, &state_1.rwscor, sizeof(integer));
    do_uio(1, &state_1.egscor, sizeof(integer));
    do_uio(1, &state_1.mxload, sizeof(integer));
    do_uio(1, &state_1.ltshft, sizeof(integer));
    do_uio(1, &state_1.bloc, sizeof(integer));
    do_uio(1, &state_1.mungrm, sizeof(integer));
    do_uio(1, &state_1.hs, sizeof(integer));
    do_uio(1, &screen_1.fromdr, sizeof(integer));
    do_uio(1, &screen_1.scolrm, sizeof(integer));
    do_uio(1, &screen_1.scolac, sizeof(integer));

    do_uio(220, &objcts_1.odesc1[0], sizeof(integer));
    do_uio(220, &objcts_1.odesc2[0], sizeof(integer));
    do_uio(220, &objcts_1.oflag1[0], sizeof(integer));
    do_uio(220, &objcts_1.oflag2[0], sizeof(integer));
    do_uio(220, &objcts_1.ofval[0], sizeof(integer));
    do_uio(220, &objcts_1.otval[0], sizeof(integer));
    do_uio(220, &objcts_1.osize[0], sizeof(integer));
    do_uio(220, &objcts_1.ocapac[0], sizeof(integer));
    do_uio(220, &objcts_1.oroom[0], sizeof(integer));
    do_uio(220, &objcts_1.oadv[0], sizeof(integer));
    do_uio(220, &objcts_1.ocan[0], sizeof(integer));

    do_uio(200, &rooms_1.rval[0], sizeof(integer));
    do_uio(200, &rooms_1.rflag[0], sizeof(integer));
    /* ILT 11/20/91 Save the ractio array as well, since otherwise
    a change to rrand can confuse the restored game.  */
    do_uio(200, &rooms_1.ractio[0], sizeof(integer));

    do_uio(4, &advs_1.aroom[0], sizeof(integer));
    do_uio(4, &advs_1.ascore[0], sizeof(integer));
    do_uio(4, &advs_1.avehic[0], sizeof(integer));
    do_uio(4, &advs_1.astren[0], sizeof(integer));
    do_uio(4, &advs_1.aflag[0], sizeof(integer));

    do_uio(46, &flags[0], sizeof(logical));
    do_uio(22, &switch_[0], sizeof(integer));
    do_uio(4, &vill_1.vprob[0], sizeof(integer));
    do_uio(25, &cevent_1.cflag[0], sizeof(logical));
    do_uio(25, &cevent_1.ctick[0], sizeof(integer));

#undef do_uio

    if (fclose(e) == EOF)
	goto L100;

    rspeak_(597);
    return;

L100:
    rspeak_(598);
/* 						!CANT DO IT. */
} /* savegm_ */

/* RESTORE- RESTORE GAME STATE */

/* DECLARATIONS */

void rstrgm_()
{
    /* Local variables */
    integer i, j, k;
    FILE *e;

    prsvec_1.prswon = FALSE_;
/* 						!DISABLE GAME. */
/* Note: save file format is different for PDP vs. non-PDP versions */

    if ((e = fopen("dsave.dat", BINREAD)) == NULL)
	goto L100;

#define do_uio(i, zbuf, cbytes) \
	(void)fread((char *)(zbuf), (cbytes), (i), e)

    do_uio(1, &i, sizeof(integer));
    do_uio(1, &j, sizeof(integer));
    do_uio(1, &k, sizeof(integer));

    if (i != vers_1.vmaj | j != vers_1.vmin) {
	goto L200;
    }

    do_uio(1, &play_1.winner, sizeof(integer));
    do_uio(1, &play_1.here, sizeof(integer));
    do_uio(1, &hack_1.thfpos, sizeof(integer));
    do_uio(1, &play_1.telflg, sizeof(logical));
    do_uio(1, &hack_1.thfflg, sizeof(logical));
    do_uio(1, &hack_1.thfact, sizeof(logical));
    do_uio(1, &hack_1.swdact, sizeof(logical));
    do_uio(1, &hack_1.swdsta, sizeof(integer));
    do_uio(64, &puzzle_1.cpvec[0], sizeof(integer));

    do_uio(1, &time_1.pltime, sizeof(integer));
    do_uio(1, &state_1.moves, sizeof(integer));
    do_uio(1, &state_1.deaths, sizeof(integer));
    do_uio(1, &state_1.rwscor, sizeof(integer));
    do_uio(1, &state_1.egscor, sizeof(integer));
    do_uio(1, &state_1.mxload, sizeof(integer));
    do_uio(1, &state_1.ltshft, sizeof(integer));
    do_uio(1, &state_1.bloc, sizeof(integer));
    do_uio(1, &state_1.mungrm, sizeof(integer));
    do_uio(1, &state_1.hs, sizeof(integer));
    do_uio(1, &screen_1.fromdr, sizeof(integer));
    do_uio(1, &screen_1.scolrm, sizeof(integer));
    do_uio(1, &screen_1.scolac, sizeof(integer));

    do_uio(220, &objcts_1.odesc1[0], sizeof(integer));
    do_uio(220, &objcts_1.odesc2[0], sizeof(integer));
    do_uio(220, &objcts_1.oflag1[0], sizeof(integer));
    do_uio(220, &objcts_1.oflag2[0], sizeof(integer));
    do_uio(220, &objcts_1.ofval[0], sizeof(integer));
    do_uio(220, &objcts_1.otval[0], sizeof(integer));
    do_uio(220, &objcts_1.osize[0], sizeof(integer));
    do_uio(220, &objcts_1.ocapac[0], sizeof(integer));
    do_uio(220, &objcts_1.oroom[0], sizeof(integer));
    do_uio(220, &objcts_1.oadv[0], sizeof(integer));
    do_uio(220, &objcts_1.ocan[0], sizeof(integer));

    do_uio(200, &rooms_1.rval[0], sizeof(integer));
    do_uio(200, &rooms_1.rflag[0], sizeof(integer));
    /* ILT 11/20/91 If this is 2.7B, the ractio array was also saved.  */
    if (k != 'A')
      do_uio(200, &rooms_1.ractio[0], sizeof(integer));

    do_uio(4, &advs_1.aroom[0], sizeof(integer));
    do_uio(4, &advs_1.ascore[0], sizeof(integer));
    do_uio(4, &advs_1.avehic[0], sizeof(integer));
    do_uio(4, &advs_1.astren[0], sizeof(integer));
    do_uio(4, &advs_1.aflag[0], sizeof(integer));

    do_uio(46, &flags[0], sizeof(logical));
    do_uio(22, &switch_[0], sizeof(integer));
    do_uio(4, &vill_1.vprob[0], sizeof(integer));
    do_uio(25, &cevent_1.cflag[0], sizeof(logical));
    do_uio(25, &cevent_1.ctick[0], sizeof(integer));

    (void)fclose(e);

    rspeak_(599);
    return;

L100:
    rspeak_(598);
/* 						!CANT DO IT. */
    return;

L200:
    rspeak_(600);
/* 						!OBSOLETE VERSION */
    (void)fclose(e);
} /* rstrgm_ */

/* WALK- MOVE IN SPECIFIED DIRECTION */

/* DECLARATIONS */

logical walk_()
{
    /* System generated locals */
    logical ret_val;

    ret_val = TRUE_;
/* 						!ASSUME WINS. */
    if (play_1.winner != aindex_1.player || lit_(play_1.here) || prob_(25,
	    25)) {
	goto L500;
    }
    if (! findxt_(prsvec_1.prso, play_1.here)) {
	goto L450;
    }
/* 						!INVALID EXIT? GRUE */
/* 						! */
    switch (curxt_1.xtype) {
	case 1:  goto L400;
	case 2:  goto L200;
	case 3:  goto L100;
	case 4:  goto L300;
    }
/* 						!DECODE EXIT TYPE. */
    bug_(9, curxt_1.xtype);

L100:
    if (cxappl_(curxt_1.xactio) != 0) {
	goto L400;
    }
/* 						!CEXIT... RETURNED ROOM? */
    if (flags[*xflag - 1]) {
	goto L400;
    }
/* 						!NO, FLAG ON? */
L200:
    jigsup_(523);
/* 						!BAD EXIT, GRUE */
/* 						! */
    return ret_val;

L300:
    if (cxappl_(curxt_1.xactio) != 0) {
	goto L400;
    }
/* 						!DOOR... RETURNED ROOM? */
    if ((objcts_1.oflag2[curxt_1.xobj - 1] & OPENBT) != 0) {
	goto L400;
    }
/* 						!NO, DOOR OPEN? */
    jigsup_(523);
/* 						!BAD EXIT, GRUE */
/* 						! */
    return ret_val;

L400:
    if (lit_(curxt_1.xroom1)) {
	goto L900;
    }
/* 						!VALID ROOM, IS IT LIT? */
L450:
    jigsup_(522);
/* 						!NO, GRUE */
/* 						! */
    return ret_val;

/* ROOM IS LIT, OR WINNER IS NOT PLAYER (NO GRUE). */

L500:
    if (findxt_(prsvec_1.prso, play_1.here)) {
	goto L550;
    }
/* 						!EXIT EXIST? */
L525:
    curxt_1.xstrng = 678;
/* 						!ASSUME WALL. */
    if (prsvec_1.prso == xsrch_1.xup) {
	curxt_1.xstrng = 679;
    }
/* 						!IF UP, CANT. */
    if (prsvec_1.prso == xsrch_1.xdown) {
	curxt_1.xstrng = 680;
    }
/* 						!IF DOWN, CANT. */
    if ((rooms_1.rflag[play_1.here - 1] & RNWALL) != 0) {
	curxt_1.xstrng = 524;
    }
    rspeak_(curxt_1.xstrng);
    prsvec_1.prscon = 1;
/* 						!STOP CMD STREAM. */
    return ret_val;

L550:
    switch (curxt_1.xtype) {
	case 1:  goto L900;
	case 2:  goto L600;
	case 3:  goto L700;
	case 4:  goto L800;
    }
/* 						!BRANCH ON EXIT TYPE. */
    bug_(9, curxt_1.xtype);

L700:
    if (cxappl_(curxt_1.xactio) != 0) {
	goto L900;
    }
/* 						!CEXIT... RETURNED ROOM? */
    if (flags[*xflag - 1]) {
	goto L900;
    }
/* 						!NO, FLAG ON? */
L600:
    if (curxt_1.xstrng == 0) {
	goto L525;
    }
/* 						!IF NO REASON, USE STD. */
    rspeak_(curxt_1.xstrng);
/* 						!DENY EXIT. */
    prsvec_1.prscon = 1;
/* 						!STOP CMD STREAM. */
    return ret_val;

L800:
    if (cxappl_(curxt_1.xactio) != 0) {
	goto L900;
    }
/* 						!DOOR... RETURNED ROOM? */
    if ((objcts_1.oflag2[curxt_1.xobj - 1] & OPENBT) != 0) {
	goto L900;
    }
/* 						!NO, DOOR OPEN? */
    if (curxt_1.xstrng == 0) {
	curxt_1.xstrng = 525;
    }
/* 						!IF NO REASON, USE STD. */
    rspsub_(curxt_1.xstrng, objcts_1.odesc2[curxt_1.xobj - 1]);
    prsvec_1.prscon = 1;
/* 						!STOP CMD STREAM. */
    return ret_val;

L900:
    ret_val = moveto_(curxt_1.xroom1, play_1.winner);
/* 						!MOVE TO ROOM. */
    if (ret_val) {
	ret_val = rmdesc_(0);
    }
/* 						!DESCRIBE ROOM. */
    return ret_val;
} /* walk_ */

/* CXAPPL- CONDITIONAL EXIT PROCESSORS */

/* DECLARATIONS */

static integer cxappl_(ri)
integer ri;
{
    /* System generated locals */
    integer ret_val, i__1;

    /* Local variables */
    integer i, j, k;
    integer nxt;
    integer ldir;

    ret_val = 0;
/* 						!NO RETURN. */
    if (ri == 0) {
	return ret_val;
    }
/* 						!IF NO ACTION, DONE. */
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
    }
    bug_(5, ri);

/* C1- COFFIN-CURE */

L1000:
    findex_1.egyptf = objcts_1.oadv[oindex_1.coffi - 1] != play_1.winner;
/* 						!T IF NO COFFIN. */
    return ret_val;

/* C2- CAROUSEL EXIT */
/* C5- CAROUSEL OUT */

L2000:
    if (findex_1.caroff) {
	return ret_val;
    }
/* 						!IF FLIPPED, NOTHING. */
L2500:
    rspeak_(121);
/* 						!SPIN THE COMPASS. */
L5000:
    i = xpars_1.xelnt[xpars_1.xcond - 1] * rnd_(8);
/* 						!CHOOSE RANDOM EXIT. */
    curxt_1.xroom1 = exits_1.travel[rooms_1.rexit[play_1.here - 1] + i - 1] & 
	    xpars_1.xrmask;
    ret_val = curxt_1.xroom1;
/* 						!RETURN EXIT. */
    return ret_val;

/* C3- CHIMNEY FUNCTION */

L3000:
    findex_1.litldf = FALSE_;
/* 						!ASSUME HEAVY LOAD. */
    j = 0;
    i__1 = objcts_1.olnt;
    for (i = 1; i <= i__1; ++i) {
/* 						!COUNT OBJECTS. */
	if (objcts_1.oadv[i - 1] == play_1.winner) {
	    ++j;
	}
/* L3100: */
    }

    if (j > 2) {
	return ret_val;
    }
/* 						!CARRYING TOO MUCH? */
    curxt_1.xstrng = 446;
/* 						!ASSUME NO LAMP. */
    if (objcts_1.oadv[oindex_1.lamp - 1] != play_1.winner) {
	return ret_val;
    }
/* 						!NO LAMP? */
    findex_1.litldf = TRUE_;
/* 						!HE CAN DO IT. */
    if ((objcts_1.oflag2[oindex_1.door - 1] & OPENBT) == 0) {
	objcts_1.oflag2[oindex_1.door - 1] &= ~ TCHBT;
    }
    return ret_val;

/* C4-	FROBOZZ FLAG (MAGNET ROOM, FAKE EXIT) */
/* C6-	FROBOZZ FLAG (MAGNET ROOM, REAL EXIT) */

L4000:
    if (findex_1.caroff) {
	goto L2500;
    }
/* 						!IF FLIPPED, GO SPIN. */
    findex_1.frobzf = FALSE_;
/* 						!OTHERWISE, NOT AN EXIT. */
    return ret_val;

L6000:
    if (findex_1.caroff) {
	goto L2500;
    }
/* 						!IF FLIPPED, GO SPIN. */
    findex_1.frobzf = TRUE_;
/* 						!OTHERWISE, AN EXIT. */
    return ret_val;

/* C7-	FROBOZZ FLAG (BANK ALARM) */

L7000:
    findex_1.frobzf = objcts_1.oroom[oindex_1.bills - 1] != 0 & 
	    objcts_1.oroom[oindex_1.portr - 1] != 0;
    return ret_val;
/* CXAPPL, PAGE 3 */

/* C8-	FROBOZZ FLAG (MRGO) */

L8000:
    findex_1.frobzf = FALSE_;
/* 						!ASSUME CANT MOVE. */
    if (findex_1.mloc != curxt_1.xroom1) {
	goto L8100;
    }
/* 						!MIRROR IN WAY? */
    if (prsvec_1.prso == xsrch_1.xnorth || prsvec_1.prso == xsrch_1.xsouth) {
	goto L8200;
    }
    if (findex_1.mdir % 180 != 0) {
	goto L8300;
    }
/* 						!MIRROR MUST BE N-S. */
    curxt_1.xroom1 = (curxt_1.xroom1 - rindex_1.mra << 1) + rindex_1.mrae;
/* 						!CALC EAST ROOM. */
    if (prsvec_1.prso > xsrch_1.xsouth) {
	++curxt_1.xroom1;
    }
/* 						!IF SW/NW, CALC WEST. */
L8100:
    ret_val = curxt_1.xroom1;
    return ret_val;

L8200:
    curxt_1.xstrng = 814;
/* 						!ASSUME STRUC BLOCKS. */
    if (findex_1.mdir % 180 == 0) {
	return ret_val;
    }
/* 						!IF MIRROR N-S, DONE. */
L8300:
    ldir = findex_1.mdir;
/* 						!SEE WHICH MIRROR. */
    if (prsvec_1.prso == xsrch_1.xsouth) {
	ldir = 180;
    }
    curxt_1.xstrng = 815;
/* 						!MIRROR BLOCKS. */
    if (ldir > 180 && ! findex_1.mr1f || ldir < 180 && ! findex_1.mr2f) {
	curxt_1.xstrng = 816;
    }
    return ret_val;

/* C9-	FROBOZZ FLAG (MIRIN) */

L9000:
    if (mrhere_(play_1.here) != 1) {
	goto L9100;
    }
/* 						!MIRROR 1 HERE? */
    if (findex_1.mr1f) {
	curxt_1.xstrng = 805;
    }
/* 						!SEE IF BROKEN. */
    findex_1.frobzf = findex_1.mropnf;
/* 						!ENTER IF OPEN. */
    return ret_val;

L9100:
    findex_1.frobzf = FALSE_;
/* 						!NOT HERE, */
    curxt_1.xstrng = 817;
/* 						!LOSE. */
    return ret_val;
/* CXAPPL, PAGE 4 */

/* C10-	FROBOZZ FLAG (MIRROR EXIT) */

L10000:
    findex_1.frobzf = FALSE_;
/* 						!ASSUME CANT. */
    ldir = (prsvec_1.prso - xsrch_1.xnorth) / xsrch_1.xnorth * 45;
/* 						!XLATE DIR TO DEGREES. */
    if (! findex_1.mropnf || (findex_1.mdir + 270) % 360 != ldir && 
	    prsvec_1.prso != xsrch_1.xexit) {
	goto L10200;
    }
    curxt_1.xroom1 = (findex_1.mloc - rindex_1.mra << 1) + rindex_1.mrae + 1 
	    - findex_1.mdir / 180;
/* 						!ASSUME E-W EXIT. */
    if (findex_1.mdir % 180 == 0) {
	goto L10100;
    }
/* 						!IF N-S, OK. */
    curxt_1.xroom1 = findex_1.mloc + 1;
/* 						!ASSUME N EXIT. */
    if (findex_1.mdir > 180) {
	curxt_1.xroom1 = findex_1.mloc - 1;
    }
/* 						!IF SOUTH. */
L10100:
    ret_val = curxt_1.xroom1;
    return ret_val;

L10200:
    if (! findex_1.wdopnf || (findex_1.mdir + 180) % 360 != ldir && 
	    prsvec_1.prso != xsrch_1.xexit) {
	return ret_val;
    }
    curxt_1.xroom1 = findex_1.mloc + 1;
/* 						!ASSUME N. */
    if (findex_1.mdir == 0) {
	curxt_1.xroom1 = findex_1.mloc - 1;
    }
/* 						!IF S. */
    rspeak_(818);
/* 						!CLOSE DOOR. */
    findex_1.wdopnf = FALSE_;
    ret_val = curxt_1.xroom1;
    return ret_val;

/* C11-	MAYBE DOOR.  NORMAL MESSAGE IS THAT DOOR IS CLOSED. */
/* 	BUT IF LCELL.NE.4, DOOR ISNT THERE. */

L11000:
    if (findex_1.lcell != 4) {
	curxt_1.xstrng = 678;
    }
/* 						!SET UP MSG. */
    return ret_val;

/* C12-	FROBZF (PUZZLE ROOM MAIN ENTRANCE) */

L12000:
    findex_1.frobzf = TRUE_;
/* 						!ALWAYS ENTER. */
    findex_1.cphere = 10;
/* 						!SET SUBSTATE. */
    return ret_val;

/* C13-	CPOUTF (PUZZLE ROOM SIZE ENTRANCE) */

L13000:
    findex_1.cphere = 52;
/* 						!SET SUBSTATE. */
    return ret_val;
/* CXAPPL, PAGE 5 */

/* C14-	FROBZF (PUZZLE ROOM TRANSITIONS) */

L14000:
    findex_1.frobzf = FALSE_;
/* 						!ASSSUME LOSE. */
    if (prsvec_1.prso != xsrch_1.xup) {
	goto L14100;
    }
/* 						!UP? */
    if (findex_1.cphere != 10) {
	return ret_val;
    }
/* 						!AT EXIT? */
    curxt_1.xstrng = 881;
/* 						!ASSUME NO LADDER. */
    if (puzzle_1.cpvec[findex_1.cphere] != -2) {
	return ret_val;
    }
/* 						!LADDER HERE? */
    rspeak_(882);
/* 						!YOU WIN. */
    findex_1.frobzf = TRUE_;
/* 						!LET HIM OUT. */
    return ret_val;

L14100:
    if (findex_1.cphere != 52 || prsvec_1.prso != xsrch_1.xwest || ! 
	    findex_1.cpoutf) {
	goto L14200;
    }
    findex_1.frobzf = TRUE_;
/* 						!YES, LET HIM OUT. */
    return ret_val;

L14200:
    for (i = 1; i <= 16; i += 2) {
/* 						!LOCATE EXIT. */
	if (prsvec_1.prso == puzzle_1.cpdr[i - 1]) {
	    goto L14400;
	}
/* L14300: */
    }
    return ret_val;
/* 						!NO SUCH EXIT. */

L14400:
    j = puzzle_1.cpdr[i];
/* 						!GET DIRECTIONAL OFFSET. */
    nxt = findex_1.cphere + j;
/* 						!GET NEXT STATE. */
    k = 8;
/* 						!GET ORTHOGONAL DIR. */
    if (j < 0) {
	k = -8;
    }
    if ((abs(j) == 1 || abs(j) == 8 || (puzzle_1.cpvec[findex_1.cphere + k - 
	    1] == 0 || puzzle_1.cpvec[nxt - k - 1] == 0)) && puzzle_1.cpvec[
	    nxt - 1] == 0) {
	goto L14500;
    }
    return ret_val;

L14500:
    cpgoto_(nxt);
/* 						!MOVE TO STATE. */
    curxt_1.xroom1 = rindex_1.cpuzz;
/* 						!STAY IN ROOM. */
    ret_val = curxt_1.xroom1;
    return ret_val;

} /* cxappl_ */
