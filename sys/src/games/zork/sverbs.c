/* SVERBS-	SIMPLE VERBS PROCESSOR */
/* 	ALL VERBS IN THIS ROUTINE MUST BE INDEPENDANT */
/* 	OF OBJECT ACTIONS */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include <stdio.h>
#include "funcs.h"
#include "vars.h"

logical sverbs_(ri)
integer ri;
{
    /* Initialized data */

    const integer mxnop = 39;
    const integer mxjoke = 64;
    static const integer jokes[25] = { 4,5,3,304,305,306,307,308,309,310,311,
	    312,313,5314,5319,324,325,883,884,120,120,0,0,0,0 };
    static const integer answer[14] = { 0,1,2,3,4,4,4,4,5,5,5,6,7,7};
    static const char * const ansstr[14] =
	    { "TEMPLE", "FOREST", "30003", "FLASK", "RUB", "FONDLE",
	      "CARRES", "TOUCH", "BONES", "BODY", "SKELE", "RUSTYKNIFE",
	      "NONE", "NOWHER" };

    /* System generated locals */
    integer i__1, i__2;
    logical ret_val;

    /* Local variables */
    logical f;
    const char *z, *z2;
    integer i, j;
    integer k;
    integer l;
    char ch[1*6];
    integer cp, wp;
    char pp1[1*6], pp2[1*6];
    integer odi2 = 0, odo2 = 0;

    ret_val = TRUE_;
/* 						!ASSUME WINS. */
    if (prsvec_1.prso != 0) {
	odo2 = objcts_1.odesc2[prsvec_1.prso - 1];
    }
/* 						!SET UP DESCRIPTORS. */
    if (prsvec_1.prsi != 0) {
	odi2 = objcts_1.odesc2[prsvec_1.prsi - 1];
    }

    if (ri == 0) {
	bug_(7, ri);
    }
/* 						!ZERO IS VERBOTEN. */
    if (ri <= mxnop) {
	return ret_val;
    }
/* 						!NOP? */
    if (ri <= mxjoke) {
	goto L100;
    }
/* 						!JOKE? */
    switch (ri - mxjoke) {
	case 1:  goto L65000;
	case 2:  goto L66000;
	case 3:  goto L67000;
	case 4:  goto L68000;
	case 5:  goto L69000;
	case 6:  goto L1000;
	case 7:  goto L2000;
	case 8:  goto L3000;
	case 9:  goto L4000;
	case 10:  goto L5000;
	case 11:  goto L6000;
	case 12:  goto L7000;
	case 13:  goto L8000;
	case 14:  goto L9000;
	case 15:  goto L10000;
	case 16:  goto L11000;
	case 17:  goto L12000;
	case 18:  goto L13000;
	case 19:  goto L14000;
	case 20:  goto L15000;
	case 21:  goto L16000;
	case 22:  goto L17000;
	case 23:  goto L18000;
	case 24:  goto L19000;
	case 25:  goto L20000;
	case 26:  goto L21000;
	case 27:  goto L22000;
	case 28:  goto L23000;
	case 29:  goto L24000;
	case 30:  goto L25000;
	case 31:  goto L26000;
	case 32:  goto L27000;
    }
    bug_(7, ri);

/* ALL VERB PROCESSORS RETURN HERE TO DECLARE FAILURE. */

/* L10: */
    ret_val = FALSE_;
/* 						!LOSE. */
    return ret_val;

/* JOKE PROCESSOR. */
/* FIND PROPER ENTRY IN JOKES, USE IT TO SELECT STRING TO PRINT. */

L100:
    i = jokes[ri - mxnop - 1];
/* 						!GET TABLE ENTRY. */
    j = i / 1000;
/* 						!ISOLATE # STRINGS. */
    if (j != 0) {
	i = i % 1000 + rnd_(j);
    }
/* 						!IF RANDOM, CHOOSE. */
    rspeak_(i);
/* 						!PRINT JOKE. */
    return ret_val;
/* SVERBS, PAGE 2A */

/* V65--	ROOM */

L65000:
    ret_val = rmdesc_(2);
/* 						!DESCRIBE ROOM ONLY. */
    return ret_val;

/* V66--	OBJECTS */

L66000:
    ret_val = rmdesc_(1);
/* 						!DESCRIBE OBJ ONLY. */
    if (! play_1.telflg) {
	rspeak_(138);
    }
/* 						!NO OBJECTS. */
    return ret_val;

/* V67--	RNAME */

L67000:
    i__1 = rooms_1.rdesc2[play_1.here - 1];
    rspeak_(i__1);
/* 						!SHORT ROOM NAME. */
    return ret_val;

/* V68--	RESERVED */

L68000:
    return ret_val;

/* V69--	RESERVED */

L69000:
    return ret_val;
/* SVERBS, PAGE 3 */

/* V70--	BRIEF.  SET FLAG. */

L1000:
    findex_1.brieff = TRUE_;
/* 						!BRIEF DESCRIPTIONS. */
    findex_1.superf = FALSE_;
    rspeak_(326);
    return ret_val;

/* V71--	VERBOSE.  CLEAR FLAGS. */

L2000:
    findex_1.brieff = FALSE_;
/* 						!LONG DESCRIPTIONS. */
    findex_1.superf = FALSE_;
    rspeak_(327);
    return ret_val;

/* V72--	SUPERBRIEF.  SET FLAG. */

L3000:
    findex_1.superf = TRUE_;
    rspeak_(328);
    return ret_val;

/* V73-- STAY (USED IN ENDGAME). */

L4000:
    if (play_1.winner != aindex_1.amastr) {
	goto L4100;
    }
/* 						!TELL MASTER, STAY. */
    rspeak_(781);
/* 						!HE DOES. */
    cevent_1.ctick[cindex_1.cevfol - 1] = 0;
/* 						!NOT FOLLOWING. */
    return ret_val;

L4100:
    if (play_1.winner == aindex_1.player) {
	rspeak_(664);
    }
/* 						!JOKE. */
    return ret_val;

/* V74--	VERSION.  PRINT INFO. */

L5000:
    more_output(NULL);
    printf("V%1d.%1d%c\n", vers_1.vmaj, vers_1.vmin, vers_1.vedit);
    play_1.telflg = TRUE_;
    return ret_val;

/* V75--	SWIM.  ALWAYS A JOKE. */

L6000:
    i = 330;
/* 						!ASSUME WATER. */
    if ((rooms_1.rflag[play_1.here - 1] & RWATER + RFILL) == 
	    0) {
	i = rnd_(3) + 331;
    }
    rspeak_(i);
    return ret_val;

/* V76--	GERONIMO.  IF IN BARREL, FATAL, ELSE JOKE. */

L7000:
    if (play_1.here == rindex_1.mbarr) {
	goto L7100;
    }
/* 						!IN BARREL? */
    rspeak_(334);
/* 						!NO, JOKE. */
    return ret_val;

L7100:
    jigsup_(335);
/* 						!OVER FALLS. */
    return ret_val;

/* V77--	SINBAD ET AL.  CHASE CYCLOPS, ELSE JOKE. */

L8000:
    if (play_1.here == rindex_1.mcycl && qhere_(oindex_1.cyclo, play_1.here)
	    ) {
	goto L8100;
    }
    rspeak_(336);
/* 						!NOT HERE, JOKE. */
    return ret_val;

L8100:
    newsta_(oindex_1.cyclo, 337, 0, 0, 0);
/* 						!CYCLOPS FLEES. */
    findex_1.cyclof = TRUE_;
/* 						!SET ALL FLAGS. */
    findex_1.magicf = TRUE_;
    objcts_1.oflag2[oindex_1.cyclo - 1] &= ~ FITEBT;
    return ret_val;

/* V78--	WELL.  OPEN DOOR, ELSE JOKE. */

L9000:
    if (findex_1.riddlf || play_1.here != rindex_1.riddl) {
	goto L9100;
    }
/* 						!IN RIDDLE ROOM? */
    findex_1.riddlf = TRUE_;
/* 						!YES, SOLVED IT. */
    rspeak_(338);
    return ret_val;

L9100:
    rspeak_(339);
/* 						!WELL, WHAT? */
    return ret_val;

/* V79--	PRAY.  IF IN TEMP2, POOF */
/* 						! */

L10000:
    if (play_1.here != rindex_1.temp2) {
	goto L10050;
    }
/* 						!IN TEMPLE? */
    if (moveto_(rindex_1.fore1, play_1.winner)) {
	goto L10100;
    }
/* 						!FORE1 STILL THERE? */
L10050:
    rspeak_(340);
/* 						!JOKE. */
    return ret_val;

L10100:
    f = rmdesc_(3);
/* 						!MOVED, DESCRIBE. */
    return ret_val;

/* V80--	TREASURE.  IF IN TEMP1, POOF */
/* 						! */

L11000:
    if (play_1.here != rindex_1.temp1) {
	goto L11050;
    }
/* 						!IN TEMPLE? */
    if (moveto_(rindex_1.treas, play_1.winner)) {
	goto L10100;
    }
/* 						!TREASURE ROOM THERE? */
L11050:
    rspeak_(341);
/* 						!NOTHING HAPPENS. */
    return ret_val;

/* V81--	TEMPLE.  IF IN TREAS, POOF */
/* 						! */

L12000:
    if (play_1.here != rindex_1.treas) {
	goto L12050;
    }
/* 						!IN TREASURE? */
    if (moveto_(rindex_1.temp1, play_1.winner)) {
	goto L10100;
    }
/* 						!TEMP1 STILL THERE? */
L12050:
    rspeak_(341);
/* 						!NOTHING HAPPENS. */
    return ret_val;

/* V82--	BLAST.  USUALLY A JOKE. */

L13000:
    i = 342;
/* 						!DONT UNDERSTAND. */
    if (prsvec_1.prso == oindex_1.safe) {
	i = 252;
    }
/* 						!JOKE FOR SAFE. */
    rspeak_(i);
    return ret_val;

/* V83--	SCORE.  PRINT SCORE. */

L14000:
    score_(0);
    return ret_val;

/* V84--	QUIT.  FINISH OUT THE GAME. */

L15000:
    score_(1);
/* 						!TELLL SCORE. */
    if (! yesno_(343, 0, 0)) {
	return ret_val;
    }
/* 						!ASK FOR Y/N DECISION. */
    exit_();
/* 						!BYE. */
/* SVERBS, PAGE 4 */

/* V85--	FOLLOW (USED IN ENDGAME) */

L16000:
    if (play_1.winner != aindex_1.amastr) {
	return ret_val;
    }
/* 						!TELL MASTER, FOLLOW. */
    rspeak_(782);
    cevent_1.ctick[cindex_1.cevfol - 1] = -1;
/* 						!STARTS FOLLOWING. */
    return ret_val;

/* V86--	WALK THROUGH */

L17000:
    if (screen_1.scolrm == 0 || prsvec_1.prso != oindex_1.scol && (
	    prsvec_1.prso != oindex_1.wnort || play_1.here != rindex_1.bkbox))
	     {
	goto L17100;
    }
    screen_1.scolac = screen_1.scolrm;
/* 						!WALKED THRU SCOL. */
    prsvec_1.prso = 0;
/* 						!FAKE OUT FROMDR. */
    cevent_1.ctick[cindex_1.cevscl - 1] = 6;
/* 						!START ALARM. */
    rspeak_(668);
/* 						!DISORIENT HIM. */
    f = moveto_(screen_1.scolrm, play_1.winner);
/* 						!INTO ROOM. */
    f = rmdesc_(3);
/* 						!DESCRIBE. */
    return ret_val;

L17100:
    if (play_1.here != screen_1.scolac) {
	goto L17300;
    }
/* 						!ON OTHER SIDE OF SCOL? */
    for (i = 1; i <= 12; i += 3) {
/* 						!WALK THRU PROPER WALL? */
	if (screen_1.scolwl[i - 1] == play_1.here && screen_1.scolwl[i] == 
		prsvec_1.prso) {
	    goto L17500;
	}
/* L17200: */
    }

L17300:
    if ((objcts_1.oflag1[prsvec_1.prso - 1] & TAKEBT) != 0) {
	goto L17400;
    }
    i = 669;
/* 						!NO, JOKE. */
    if (prsvec_1.prso == oindex_1.scol) {
	i = 670;
    }
/* 						!SPECIAL JOKE FOR SCOL. */
    rspsub_(i, odo2);
    return ret_val;

L17400:
    i = 671;
/* 						!JOKE. */
    if (objcts_1.oroom[prsvec_1.prso - 1] != 0) {
	i = rnd_(5) + 552;
    }
/* 						!SPECIAL JOKES IF CARRY. */
    rspeak_(i);
    return ret_val;

L17500:
    prsvec_1.prso = screen_1.scolwl[i + 1];
/* 						!THRU SCOL WALL... */
    for (i = 1; i <= 8; i += 2) {
/* 						!FIND MATCHING ROOM. */
	if (prsvec_1.prso == screen_1.scoldr[i - 1]) {
	    screen_1.scolrm = screen_1.scoldr[i];
	}
/* L17600: */
    }
/* 						!DECLARE NEW SCOLRM. */
    cevent_1.ctick[cindex_1.cevscl - 1] = 0;
/* 						!CANCEL ALARM. */
    rspeak_(668);
/* 						!DISORIENT HIM. */
    f = moveto_(rindex_1.bkbox, play_1.winner);
/* 						!BACK IN BOX ROOM. */
    f = rmdesc_(3);
    return ret_val;

/* V87--	RING.  A JOKE. */

L18000:
    i = 359;
/* 						!CANT RING. */
    if (prsvec_1.prso == oindex_1.bell) {
	i = 360;
    }
/* 						!DING, DONG. */
    rspeak_(i);
/* 						!JOKE. */
    return ret_val;

/* V88--	BRUSH.  JOKE WITH OBSCURE TRAP. */

L19000:
    if (prsvec_1.prso == oindex_1.teeth) {
	goto L19100;
    }
/* 						!BRUSH TEETH? */
    rspeak_(362);
/* 						!NO, JOKE. */
    return ret_val;

L19100:
    if (prsvec_1.prsi != 0) {
	goto L19200;
    }
/* 						!WITH SOMETHING? */
    rspeak_(363);
/* 						!NO, JOKE. */
    return ret_val;

L19200:
    if (prsvec_1.prsi == oindex_1.putty && objcts_1.oadv[oindex_1.putty - 1] 
	    == play_1.winner) {
	goto L19300;
    }
    rspsub_(364, odi2);
/* 						!NO, JOKE. */
    return ret_val;

L19300:
    jigsup_(365);
/* 						!YES, DEAD */
/* 						! */
/* 						! */
/* 						! */
/* 						! */
/* 						! */
    return ret_val;
/* SVERBS, PAGE 5 */

/* V89--	DIG.  UNLESS SHOVEL, A JOKE. */

L20000:
    if (prsvec_1.prso == oindex_1.shove) {
	return ret_val;
    }
/* 						!SHOVEL? */
    i = 392;
/* 						!ASSUME TOOL. */
    if ((objcts_1.oflag1[prsvec_1.prso - 1] & TOOLBT) == 0) {
	i = 393;
    }
    rspsub_(i, odo2);
    return ret_val;

/* V90--	TIME.  PRINT OUT DURATION OF GAME. */

L21000:
    gttime_(&k);
/* 						!GET PLAY TIME. */
    i = k / 60;
    j = k % 60;

    more_output(NULL);
    printf("You have been playing Dungeon for ");
    if (i >= 1) {
	printf("%d hour", i);
	if (i >= 2)
	    printf("s");
	printf(" and ");
    }
    printf("%d minute", j);
    if (j != 1)
	printf("s");
    printf(".\n");
    play_1.telflg = TRUE_;
    return ret_val;


/* V91--	LEAP.  USUALLY A JOKE, WITH A CATCH. */

L22000:
    if (prsvec_1.prso == 0) {
	goto L22200;
    }
/* 						!OVER SOMETHING? */
    if (qhere_(prsvec_1.prso, play_1.here)) {
	goto L22100;
    }
/* 						!HERE? */
    rspeak_(447);
/* 						!NO, JOKE. */
    return ret_val;

L22100:
    if ((objcts_1.oflag2[prsvec_1.prso - 1] & VILLBT) == 0) {
	goto L22300;
    }
    rspsub_(448, odo2);
/* 						!CANT JUMP VILLAIN. */
    return ret_val;

L22200:
    if (! findxt_(xsrch_1.xdown, play_1.here)) {
	goto L22300;
    }
/* 						!DOWN EXIT? */
    if (curxt_1.xtype == xpars_1.xno || curxt_1.xtype == xpars_1.xcond && ! 
	    flags[*xflag - 1]) {
	goto L22400;
    }
L22300:
    i__1 = rnd_(5) + 314;
    rspeak_(i__1);
/* 						!WHEEEE */
/* 						! */
    return ret_val;

L22400:
    i__1 = rnd_(4) + 449;
    jigsup_(i__1);
/* 						!FATAL LEAP. */
    return ret_val;
/* SVERBS, PAGE 6 */

/* V92--	LOCK. */

L23000:
    if (prsvec_1.prso == oindex_1.grate && play_1.here == rindex_1.mgrat) {
	goto L23200;
    }
L23100:
    rspeak_(464);
/* 						!NOT LOCK GRATE. */
    return ret_val;

L23200:
    findex_1.grunlf = FALSE_;
/* 						!GRATE NOW LOCKED. */
    rspeak_(214);
    exits_1.travel[rooms_1.rexit[play_1.here - 1]] = 214;
/* 						!CHANGE EXIT STATUS. */
    return ret_val;

/* V93--	UNLOCK */

L24000:
    if (prsvec_1.prso != oindex_1.grate || play_1.here != rindex_1.mgrat) {
	goto L23100;
    }
    if (prsvec_1.prsi == oindex_1.keys) {
	goto L24200;
    }
/* 						!GOT KEYS? */
    rspsub_(465, odi2);
/* 						!NO, JOKE. */
    return ret_val;

L24200:
    findex_1.grunlf = TRUE_;
/* 						!UNLOCK GRATE. */
    rspeak_(217);
    exits_1.travel[rooms_1.rexit[play_1.here - 1]] = 217;
/* 						!CHANGE EXIT STATUS. */
    return ret_val;

/* V94--	DIAGNOSE. */

L25000:
    i = fights_(play_1.winner, 0);
/* 						!GET FIGHTS STRENGTH. */
    j = advs_1.astren[play_1.winner - 1];
/* 						!GET HEALTH. */
/* Computing MIN */
    i__1 = i + j;
    k = min(i__1,4);
/* 						!GET STATE. */
    if (! cevent_1.cflag[cindex_1.cevcur - 1]) {
	j = 0;
    }
/* 						!IF NO WOUNDS. */
/* Computing MIN */
    i__1 = 4, i__2 = abs(j);
    l = min(i__1,i__2);
/* 						!SCALE. */
    i__1 = l + 473;
    rspeak_(i__1);
/* 						!DESCRIBE HEALTH. */
    i = (-j - 1) * 30 + cevent_1.ctick[cindex_1.cevcur - 1];
/* 						!COMPUTE WAIT. */

    if (j != 0) {
	more_output(NULL);
	printf("You will be cured after %d moves.\n", i);
    }

    i__1 = k + 478;
    rspeak_(i__1);
/* 						!HOW MUCH MORE? */
    if (state_1.deaths != 0) {
	i__1 = state_1.deaths + 482;
	rspeak_(i__1);
    }
/* 						!HOW MANY DEATHS? */
    return ret_val;
/* SVERBS, PAGE 7 */

/* V95--	INCANT */

L26000:
    for (i = 1; i <= 6; ++i) {
/* 						!SET UP PARSE. */
	pp1[i - 1] = ' ';
	pp2[i - 1] = ' ';
/* L26100: */
    }
    wp = 1;
/* 						!WORD POINTER. */
    cp = 1;
/* 						!CHAR POINTER. */
    if (prsvec_1.prscon <= 1) {
	goto L26300;
    }
    for (z = input_1.inbuf + prsvec_1.prscon - 1; *z != '\0'; ++z) {
/* 						!PARSE INPUT */
	if (*z == ',')
	    goto L26300;
/* 						!END OF PHRASE? */
	if (*z != ' ')
	    goto L26150;
/* 						!SPACE? */
	if (cp != 1) {
	    ++wp;
	}
	cp = 1;
	goto L26200;
L26150:
	if (wp == 1) {
	    pp1[cp - 1] = *z;
	}
/* 						!STUFF INTO HOLDER. */
	if (wp == 2) {
	    pp2[cp - 1] = *z;
	}
/* Computing MIN */
	i__2 = cp + 1;
	cp = min(i__2,6);
L26200:
	;
    }

L26300:
    prsvec_1.prscon = 1;
/* 						!KILL REST OF LINE. */
    if (pp1[0] != ' ') {
	goto L26400;
    }
/* 						!ANY INPUT? */
    rspeak_(856);
/* 						!NO, HO HUM. */
    return ret_val;

L26400:
    encryp_(pp1, ch);
/* 						!COMPUTE RESPONSE. */
    if (pp2[0] != ' ') {
	goto L26600;
    }
/* 						!TWO PHRASES? */

    if (findex_1.spellf) {
	goto L26550;
    }
/* 						!HE'S TRYING TO LEARN. */
    if ((rooms_1.rflag[rindex_1.tstrs - 1] & RSEEN) == 0) {
	goto L26575;
    }
    findex_1.spellf = TRUE_;
/* 						!TELL HIM. */
    play_1.telflg = TRUE_;
    more_output(NULL);
    printf("A hollow voice replies:  \"%.6s %.6s\".\n", pp1, ch);

    return ret_val;

L26550:
    rspeak_(857);
/* 						!HE'S GOT ONE ALREADY. */
    return ret_val;

L26575:
    rspeak_(858);
/* 						!HE'S NOT IN ENDGAME. */
    return ret_val;

L26600:
    if ((rooms_1.rflag[rindex_1.tstrs - 1] & RSEEN) != 0) {
	goto L26800;
    }
    for (i = 1; i <= 6; ++i) {
	if (pp2[i - 1] != ch[i - 1]) {
	    goto L26575;
	}
/* 						!WRONG. */
/* L26700: */
    }
    findex_1.spellf = TRUE_;
/* 						!IT WORKS. */
    rspeak_(859);
    cevent_1.ctick[cindex_1.cevste - 1] = 1;
/* 						!FORCE START. */
    return ret_val;

L26800:
    rspeak_(855);
/* 						!TOO LATE. */
    return ret_val;
/* SVERBS, PAGE 8 */

/* V96--	ANSWER */

L27000:
    if (prsvec_1.prscon > 1 && play_1.here == rindex_1.fdoor && 
	    findex_1.inqstf) {
	goto L27100;
    }
    rspeak_(799);
/* 						!NO ONE LISTENS. */
    prsvec_1.prscon = 1;
    return ret_val;

L27100:
    for (j = 1; j <= 14; j ++) {
/* 						!CHECK ANSWERS. */
	if (findex_1.quesno != answer[j - 1])
	    goto L27300;
/* 						!ONLY CHECK PROPER ANS. */
	z = ansstr[j - 1];
	z2 = input_1.inbuf + prsvec_1.prscon - 1;
	while (*z != '\0') {
	    while (*z2 == ' ')
		z2++;
/* 						!STRIP INPUT BLANKS. */
	    if (*z++ != *z2++)
		goto L27300;
	}
	goto L27500;
/* 						!RIGHT ANSWER. */
L27300:
	;
    }

    prsvec_1.prscon = 1;
/* 						!KILL REST OF LINE. */
    ++findex_1.nqatt;
/* 						!WRONG, CRETIN. */
    if (findex_1.nqatt >= 5) {
	goto L27400;
    }
/* 						!TOO MANY WRONG? */
    i__1 = findex_1.nqatt + 800;
    rspeak_(i__1);
/* 						!NO, TRY AGAIN. */
    return ret_val;

L27400:
    rspeak_(826);
/* 						!ALL OVER. */
    cevent_1.cflag[cindex_1.cevinq - 1] = FALSE_;
/* 						!LOSE. */
    return ret_val;

L27500:
    prsvec_1.prscon = 1;
/* 						!KILL REST OF LINE. */
    ++findex_1.corrct;
/* 						!GOT IT RIGHT. */
    rspeak_(800);
/* 						!HOORAY. */
    if (findex_1.corrct >= 3) {
	goto L27600;
    }
/* 						!WON TOTALLY? */
    cevent_1.ctick[cindex_1.cevinq - 1] = 2;
/* 						!NO, START AGAIN. */
    findex_1.quesno = (findex_1.quesno + 3) % 8;
    findex_1.nqatt = 0;
    rspeak_(769);
/* 						!ASK NEXT QUESTION. */
    i__1 = findex_1.quesno + 770;
    rspeak_(i__1);
    return ret_val;

L27600:
    rspeak_(827);
/* 						!QUIZ OVER, */
    cevent_1.cflag[cindex_1.cevinq - 1] = FALSE_;
    objcts_1.oflag2[oindex_1.qdoor - 1] |= OPENBT;
    return ret_val;

} /* sverbs_ */
