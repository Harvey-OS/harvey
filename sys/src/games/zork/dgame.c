/* GAME- MAIN COMMAND LOOP FOR DUNGEON */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include <stdio.h>
#include "funcs.h"
#include "vars.h"

/* Do all systems have <string.h>?  Don't use it, just in case */

extern int strcmp P((const char *, const char *));

static logical xvehic_ P((integer));
static void xendmv_ P((logical));

void game_()
{
    /* Local variables */
    logical f;
    integer i;

/* START UP, DESCRIBE CURRENT LOCATION. */

    rspeak_(1);
/* 						!WELCOME ABOARD. */
    f = rmdesc_(3);
/* 						!START GAME. */

/* NOW LOOP, READING AND EXECUTING COMMANDS. */

L100:
    play_1.winner = aindex_1.player;
/* 						!PLAYER MOVING. */
    play_1.telflg = FALSE_;
/* 						!ASSUME NOTHING TOLD. */
    if (prsvec_1.prscon <= 1) {
	rdline_(input_1.inbuf, 1);
    }

#ifdef ALLOW_GDT

    if (strcmp(input_1.inbuf + prsvec_1.prscon - 1, "GDT") == 0) {
/* 						!CALL ON GDT? */
	gdt_();
/* 						!YES, INVOKE. */
	goto L100;
/* 						!ONWARD. */
    }

#endif /* ALLOW_GDT */

    ++state_1.moves;
    prsvec_1.prswon = parse_(input_1.inbuf, 1);
    if (! prsvec_1.prswon) {
	goto L400;
    }
/* 						!PARSE LOSES? */
    if (xvehic_(1)) {
	goto L400;
    }
/* 						!VEHICLE HANDLE? */

    if (prsvec_1.prsa == vindex_1.tellw) {
	goto L2000;
    }
/* 						!TELL? */
L300:
    if (prsvec_1.prso == oindex_1.valua || prsvec_1.prso == oindex_1.every) {
	goto L900;
    }
    if (! vappli_(prsvec_1.prsa)) {
	goto L400;
    }
/* 						!VERB OK? */
L350:
    if (! findex_1.echof && play_1.here == rindex_1.echor) {
	goto L1000;
    }
    f = rappli_(rooms_1.ractio[play_1.here - 1]);

L400:
    xendmv_(play_1.telflg);
/* 						!DO END OF MOVE. */
    if (! lit_(play_1.here)) {
	prsvec_1.prscon = 1;
    }
    goto L100;

L900:
    valuac_(oindex_1.valua);
    goto L350;
/* GAME, PAGE 3 */

/* SPECIAL CASE-- ECHO ROOM. */
/* IF INPUT IS NOT 'ECHO' OR A DIRECTION, JUST ECHO. */

L1000:
    rdline_(input_1.inbuf, 0);
    ++state_1.moves;
/* 						!CHARGE FOR MOVES. */
    if (strcmp(input_1.inbuf, "ECHO") != 0)
	goto L1300;

    rspeak_(571);
/* 						!KILL THE ECHO. */
    findex_1.echof = TRUE_;
    objcts_1.oflag2[oindex_1.bar - 1] &= ~ SCRDBT;
    prsvec_1.prswon = TRUE_;
/* 						!FAKE OUT PARSER. */
    prsvec_1.prscon = 1;
/* 						!FORCE NEW INPUT. */
    goto L400;

L1300:
    prsvec_1.prswon = parse_(input_1.inbuf, 0);
    if (! prsvec_1.prswon || prsvec_1.prsa != vindex_1.walkw) {
	goto L1400;
    }
    if (findxt_(prsvec_1.prso, play_1.here)) {
	goto L300;
    }
/* 						!VALID EXIT? */

L1400:
    more_output(input_1.inbuf);
    play_1.telflg = TRUE_;
/* 						!INDICATE OUTPUT. */
    goto L1000;
/* 						!MORE ECHO ROOM. */
/* GAME, PAGE 4 */

/* SPECIAL CASE-- TELL <ACTOR>, NEW COMMAND */
/* NOTE THAT WE CANNOT BE IN THE ECHO ROOM. */

L2000:
    if ((objcts_1.oflag2[prsvec_1.prso - 1] & ACTRBT) != 0) {
	goto L2100;
    }
    rspeak_(602);
/* 						!CANT DO IT. */
    goto L350;
/* 						!VAPPLI SUCCEEDS. */

L2100:
    play_1.winner = oactor_(prsvec_1.prso);
/* 						!NEW PLAYER. */
    play_1.here = advs_1.aroom[play_1.winner - 1];
/* 						!NEW LOCATION. */
    if (prsvec_1.prscon <= 1) {
	goto L2700;
    }
/* 						!ANY INPUT? */
    if (parse_(input_1.inbuf, 1)) {
	goto L2150;
    }
L2700:
    i = 341;
/* 						!FAILS. */
    if (play_1.telflg) {
	i = 604;
    }
/* 						!GIVE RESPONSE. */
    rspeak_(i);
L2600:
    play_1.winner = aindex_1.player;
/* 						!RESTORE STATE. */
    play_1.here = advs_1.aroom[play_1.winner - 1];
    goto L350;

L2150:
    if (aappli_(advs_1.aactio[play_1.winner - 1])) {
	goto L2400;
    }
/* 						!ACTOR HANDLE? */
    if (xvehic_(1)) {
	goto L2400;
    }
/* 						!VEHICLE HANDLE? */
    if (prsvec_1.prso == oindex_1.valua || prsvec_1.prso == oindex_1.every) {
	goto L2900;
    }
    if (! vappli_(prsvec_1.prsa)) {
	goto L2400;
    }
/* 						!VERB HANDLE? */
/* L2350: */
    f = rappli_(rooms_1.ractio[play_1.here - 1]);

L2400:
    xendmv_(play_1.telflg);
/* 						!DO END OF MOVE. */
    goto L2600;
/* 						!DONE. */

L2900:
    valuac_(oindex_1.valua);
/* 						!ALL OR VALUABLES. */
    goto L350;

} /* game_ */

/* XENDMV-	EXECUTE END OF MOVE FUNCTIONS. */

/* DECLARATIONS */

static void xendmv_(flag)
logical flag;
{
    /* Local variables */
    logical f;

    if (! (flag)) {
	rspeak_(341);
    }
/* 						!DEFAULT REMARK. */
    if (hack_1.thfact) {
	thiefd_();
    }
/* 						!THIEF DEMON. */
    if (prsvec_1.prswon) {
	fightd_();
    }
/* 						!FIGHT DEMON. */
    if (hack_1.swdact) {
	swordd_();
    }
/* 						!SWORD DEMON. */
    if (prsvec_1.prswon) {
	f = clockd_();
    }
/* 						!CLOCK DEMON. */
    if (prsvec_1.prswon) {
	f = xvehic_(2);
    }
/* 						!VEHICLE READOUT. */
} /* xendmv_ */

/* XVEHIC- EXECUTE VEHICLE FUNCTION */

/* DECLARATIONS */

static logical xvehic_(n)
integer n;
{
    /* System generated locals */
    logical ret_val;

    /* Local variables */
    integer av;

    ret_val = FALSE_;
/* 						!ASSUME LOSES. */
    av = advs_1.avehic[play_1.winner - 1];
/* 						!GET VEHICLE. */
    if (av != 0) {
	ret_val = oappli_(objcts_1.oactio[av - 1], n);
    }
    return ret_val;
} /* xvehic_ */
