/* GDT- GAME DEBUGGING TOOL */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include <stdio.h>
#include <ctype.h>
#include "funcs.h"
#include "vars.h"
#include "parse.h"

#ifdef ALLOW_GDT

void gdt_()
{
    /* Initialized data */

    const integer cmdmax = 38;
    const char *dbgcmd =
	    "DRDODADCDXDHDLDVDFDSAFHENRNTNCNDRRRTRCRDTKEXARAOAAACAXAVD2DNANDMDTAHDPPDDZAZ";
    static const integer argtyp[38] = { 2,2,2,2,2,0,0,2,2,0,1,0,0,0,0,0,0,
	    0,0,0,1,0,3,3,3,3,1,3,2,2,1,2,1,0,0,0,0,1 };

    /* System generated locals */
    integer i__1, i__2;

    /* Local variables */
    integer i, j, k, l, l1;
    char cmd[3];
    integer fmax, smax;
    char buf[80];
    char *z;

/* FIRST, VALIDATE THAT THE CALLER IS AN IMPLEMENTER. */

    fmax = 46;
/* 						!SET ARRAY LIMITS. */
    smax = 22;

    if (debug_1.gdtflg != 0) {
	goto L2000;
    }
/* 						!IF OK, SKIP. */
    more_output("You are not an authorized user.");
/* 						!NOT AN IMPLEMENTER. */
    return;
/* 						!BOOT HIM OFF */

/* GDT, PAGE 2A */

/* HERE TO GET NEXT COMMAND */

L2000:
    printf("GDT>");
/* 						!OUTPUT PROMPT. */
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    cmd[0] = ' ';
    cmd[1] = ' ';
    sscanf(buf, "%2s", cmd);
/* 						!GET COMMAND. */
    if (cmd[0] == '\0')
	goto L2000;
/* 						!IGNORE BLANKS. */
    if (islower(cmd[0]))
	cmd[0] = toupper(cmd[0]);
    if (islower(cmd[1]))
	cmd[1] = toupper(cmd[1]);
    i__1 = cmdmax;
    for (i = 1; i <= i__1; ++i) {
/* 						!LOOK IT UP. */
	if (cmd[0] == dbgcmd[(i - 1) << 1] &&
	    cmd[1] == dbgcmd[((i - 1) << 1) + 1]) {
	    goto L2300;
	}
/* 						!FOUND? */
/* L2100: */
    }
L2200:
    more_output("?");
/* 						!NO, LOSE. */
    goto L2000;

/* L230: */
/* L240: */
/* L225: */
/* L235: */
/* L245: */

L2300:
    switch (argtyp[i - 1] + 1) {
	case 1:  goto L2400;
	case 2:  goto L2500;
	case 3:  goto L2600;
	case 4:  goto L2700;
    }
/* 						!BRANCH ON ARG TYPE. */
    goto L2200;
/* 						!ILLEGAL TYPE. */

L2700:
    printf("Idx,Ary:  ");
/* 						!TYPE 3, REQUEST ARRAY COORDS. */
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    for (z = buf; *z != '\0'; z++)
	if (*z == ',')
	    *z = ' ';
    j = 0;
    k = 0;
    sscanf(buf, "%d %d", &j, &k);
    goto L2400;

L2600:
    printf("Limits:   ");
/* 						!TYPE 2, READ BOUNDS. */
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    for (z = buf; *z != '\0'; z++)
	if (*z == ',')
	    *z = ' ';
    j = 0;
    k = 0;
    sscanf(buf, "%d %d", &j, &k);
    if (k == 0) {
	k = j;
    }
    goto L2400;

L2500:
    printf("Entry:    ");
/* 						!TYPE 1, READ ENTRY NO. */
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    j = 0;
    sscanf(buf, "%d", &j);

L2400:
    switch (i) {
	case 1:  goto L10000;
	case 2:  goto L11000;
	case 3:  goto L12000;
	case 4:  goto L13000;
	case 5:  goto L14000;
	case 6:  goto L15000;
	case 7:  goto L16000;
	case 8:  goto L17000;
	case 9:  goto L18000;
	case 10:  goto L19000;
	case 11:  goto L20000;
	case 12:  goto L21000;
	case 13:  goto L22000;
	case 14:  goto L23000;
	case 15:  goto L24000;
	case 16:  goto L25000;
	case 17:  goto L26000;
	case 18:  goto L27000;
	case 19:  goto L28000;
	case 20:  goto L29000;
	case 21:  goto L30000;
	case 22:  goto L31000;
	case 23:  goto L32000;
	case 24:  goto L33000;
	case 25:  goto L34000;
	case 26:  goto L35000;
	case 27:  goto L36000;
	case 28:  goto L37000;
	case 29:  goto L38000;
	case 30:  goto L39000;
	case 31:  goto L40000;
	case 32:  goto L41000;
	case 33:  goto L42000;
	case 34:  goto L43000;
	case 35:  goto L44000;
	case 36:  goto L45000;
	case 37:  goto L46000;
	case 38:  goto L47000;
    }
    goto L2200;
/* 						!WHAT??? */
/* GDT, PAGE 3 */

/* DR-- DISPLAY ROOMS */

L10000:
    if (! (j > 0 && j <= rooms_1.rlnt && (k > 0 && k <= rooms_1.rlnt) && j <= 
	    k)) {
	goto L2200;
    }
/* 						!ARGS VALID? */
    more_output("RM#  DESC1  DESC2  EXITS ACTION  VALUE  FLAGS");
/* 						!COL HDRS. */
    i__1 = k;
    for (i = j; i <= i__1; ++i) {
	more_output(NULL);
	printf("%3d", i);
	for (l = 1; l <= 6; ++l)
	    printf(" %6d", eqr[i + l * 200 - 201]);
	printf("\n");

/* L10100: */
    }
    goto L2000;


/* DO-- DISPLAY OBJECTS */

L11000:
    if (! (j > 0 && j <= objcts_1.olnt && (k > 0 && k <= objcts_1.olnt) && j 
	    <= k)) {
	goto L2200;
    }
/* 						!ARGS VALID? */
    more_output("OB# DESC1 DESC2 DESCO ACT FLAGS1 FLAGS2 FVL TVL	  SIZE CAPAC ROOM ADV CON  READ");
/* 						!COL HDRS */
    i__1 = k;
    for (i = j; i <= i__1; ++i) {
	more_output(NULL);
	printf("%3d%6d%6d%6d%4d%7d%7d%4d%4d%6d%6d %4d%4d%4d%6d\n",
		i, eqo[i + 1 * 220 - 221], eqo[i + 2 * 220 - 221],
		eqo[i + 3 * 220 - 221], eqo[i + 4 * 220 - 221],
		eqo[i + 5 * 220 - 221], eqo[i + 6 * 220 - 221],
		eqo[i + 7 * 220 - 221], eqo[i + 8 * 220 - 221],
		eqo[i + 9 * 220 - 221], eqo[i + 10 * 220 - 221],
		eqo[i + 11 * 220 - 221], eqo[i + 12 * 220 - 221],
		eqo[i + 13 * 220 - 221], eqo[i + 14 * 220 - 221]);

/* L11100: */
    }
    goto L2000;


/* DA-- DISPLAY ADVENTURERS */

L12000:
    if (! (j > 0 && j <= advs_1.alnt && (k > 0 && k <= advs_1.alnt) && j <= k)
	    ) {
	goto L2200;
    }
/* 						!ARGS VALID? */
    more_output("AD#   ROOM  SCORE  VEHIC OBJECT ACTION  STREN  FLAGS");
    i__1 = k;
    for (i = j; i <= i__1; ++i) {
	more_output(NULL);
	printf("%3d", i);
	for (l = 1; l <= 7; ++l)
	    printf(" %6d", eqa[i + (l << 2) -  5]);
	printf("\n");
/* L12100: */
    }
    goto L2000;


/* DC-- DISPLAY CLOCK EVENTS */

L13000:
    if (! (j > 0 && j <= cevent_1.clnt && (k > 0 && k <= cevent_1.clnt) && j 
	    <= k)) {
	goto L2200;
    }
/* 						!ARGS VALID? */
    more_output("CL#   TICK ACTION  FLAG");
    i__1 = k;
    for (i = j; i <= i__1; ++i) {
	more_output(NULL);
	printf("%3d %6d %6d     %c\n", i, eqc[i + 1 * 25 - 26],
		eqc[i + 2 * 25 - 26],
		cevent_1.cflag[i - 1] ? 'T' : 'F');
/* L13100: */
    }
    goto L2000;


/* DX-- DISPLAY EXITS */

L14000:
    if (! (j > 0 && j <= exits_1.xlnt && (k > 0 && k <= exits_1.xlnt) && j <= 
	    k)) {
	goto L2200;
    }
/* 						!ARGS VALID? */
    more_output("  RANGE   CONTENTS");
/* 						!COL HDRS. */
    i__1 = k;
    for (i = j; i <= i__1; i += 10) {
/* 						!TEN PER LINE. */
/* Computing MIN */
	i__2 = i + 9;
	l = min(i__2,k);
/* 						!COMPUTE END OF LINE. */
	more_output(NULL);
	printf("%3d-%3d  ", i, l);
	for (l1 = i; l1 <= l; ++l1)
	    printf("%7d", exits_1.travel[l1 - 1]);
	printf("\n");
/* L14100: */
    }
    goto L2000;


/* DH-- DISPLAY HACKS */

L15000:
    more_output(NULL);
    printf("THFPOS= %d, THFFLG= %c, THFACT= %c\n",
	   hack_1.thfpos, hack_1.thfflg ? 'T' : 'F',
	   hack_1.thfact ? 'T' : 'F');
    more_output(NULL);
    printf("SWDACT= %c, SWDSTA= %d\n", hack_1.swdact ? 'T' : 'F',
	   hack_1.swdsta);
    goto L2000;


/* DL-- DISPLAY LENGTHS */

L16000:
    more_output(NULL);
    printf("R=%d, X=%d, O=%d, C=%d\n", rooms_1.rlnt, exits_1.xlnt,
	   objcts_1.olnt, cevent_1.clnt);
    more_output(NULL);
    printf("V=%d, A=%d, M=%d, R2=%d\n", vill_1.vlnt, advs_1.alnt,
	   rmsg_1.mlnt, oroom2_1.r2lnt);
    more_output(NULL);
    printf("MBASE=%d, STRBIT=%d\n", star_1.mbase, star_1.strbit);
    goto L2000;


/* DV-- DISPLAY VILLAINS */

L17000:
    if (! (j > 0 && j <= vill_1.vlnt && (k > 0 && k <= vill_1.vlnt) && j <= k)
	    ) {
	goto L2200;
    }
/* 						!ARGS VALID? */
    more_output("VL# OBJECT   PROB   OPPS   BEST  MELEE");
/* 						!COL HDRS */
    i__1 = k;
    for (i = j; i <= i__1; ++i) {
	more_output(NULL);
	printf("%3d", i);
	for (l = 1; l <= 5; ++l)
	    printf(" %6d", eqv[i + (l << 2) - 5]);
	printf("\n");
/* L17100: */
    }
    goto L2000;


/* DF-- DISPLAY FLAGS */

L18000:
    if (! (j > 0 && j <= fmax && (k > 0 && k <= fmax) && j <= k)) {
	goto L2200;
    }
/* 						!ARGS VALID? */
    i__1 = k;
    for (i = j; i <= i__1; ++i) {
	more_output(NULL);
	printf("Flag #%-2d = %c\n", i, flags[i - 1] ? 'T' : 'F');
/* L18100: */
    }
    goto L2000;


/* DS-- DISPLAY STATE */

L19000:
    more_output(NULL);
    printf("Parse vector= %6d %6d %6d      %c %6d\n",
	   prsvec_1.prsa, prsvec_1.prso, prsvec_1.prsi,
	   prsvec_1.prswon ? 'T' : 'F', prsvec_1.prscon);
    more_output(NULL);
    printf("Play vector=  %6d %6d      %c\n", play_1.winner, play_1.here,
	    play_1.telflg ? 'T' : 'F');
    more_output(NULL);
    printf("State vector= %6d %6d %6d %6d %6d %6d %6d %6d %6d\n",
	   state_1.moves, state_1.deaths, state_1.rwscor, state_1.mxscor,
	   state_1.mxload, state_1.ltshft, state_1.bloc, state_1.mungrm,
	   state_1.hs);
    more_output(NULL);
    printf("              %6d %6d\n", state_1.egscor, state_1.egmxsc);
    more_output(NULL);
    printf("Scol vector=  %6d %6d %6d\n", screen_1.fromdr,
	   screen_1.scolrm, screen_1.scolac);
    goto L2000;

/* GDT, PAGE 4 */

/* AF-- ALTER FLAGS */

L20000:
    if (! (j > 0 && j <= fmax)) {
	goto L2200;
    }
/* 						!ENTRY NO VALID? */
    printf("Old= %c      New= ", flags[j - 1] ? 'T' : 'F');
/* 						!TYPE OLD, GET NEW. */
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    for (z = buf; *z != '\0'; z++) {
	if (! isspace(*z)) {
	    if (*z == 't' || *z == 'T')
		flags[j - 1] = 1;
	    else if (*z == 'f' || *z == 'F')
		flags[j - 1] = 0;
	    break;
	}
    }
    goto L2000;


/* 21000-- HELP */

L21000:
    more_output("Valid commands are:");
    more_output("AA- Alter ADVS          DR- Display ROOMS");
    more_output("AC- Alter CEVENT        DS- Display state");
    more_output("AF- Alter FINDEX        DT- Display text");
    more_output("AH- Alter HERE          DV- Display VILLS");
    more_output("AN- Alter switches      DX- Display EXITS");
    more_output("AO- Alter OBJCTS        DZ- Display PUZZLE");
    more_output("AR- Alter ROOMS         D2- Display ROOM2");
    more_output("AV- Alter VILLS         EX- Exit");
    more_output("AX- Alter EXITS         HE- Type this message");
    more_output("AZ- Alter PUZZLE        NC- No cyclops");
    more_output("DA- Display ADVS        ND- No deaths");
    more_output("DC- Display CEVENT      NR- No robber");
    more_output("DF- Display FINDEX      NT- No troll");
    more_output("DH- Display HACKS       PD- Program detail");
    more_output("DL- Display lengths     RC- Restore cyclops");
    more_output("DM- Display RTEXT       RD- Restore deaths");
    more_output("DN- Display switches    RR- Restore robber");
    more_output("DO- Display OBJCTS      RT- Restore troll");
    more_output("DP- Display parser      TK- Take");
    goto L2000;

/* NR-- NO ROBBER */

L22000:
    hack_1.thfflg = FALSE_;
/* 						!DISABLE ROBBER. */
    hack_1.thfact = FALSE_;
    newsta_(oindex_1.thief, 0, 0, 0, 0);
/* 						!VANISH THIEF. */
    more_output("No robber.");
    goto L2000;

/* NT-- NO TROLL */

L23000:
    findex_1.trollf = TRUE_;
    newsta_(oindex_1.troll, 0, 0, 0, 0);
    more_output("No troll.");
    goto L2000;

/* NC-- NO CYCLOPS */

L24000:
    findex_1.cyclof = TRUE_;
    newsta_(oindex_1.cyclo, 0, 0, 0, 0);
    more_output("No cyclops.");
    goto L2000;

/* ND-- IMMORTALITY MODE */

L25000:
    debug_1.dbgflg = 1;
    more_output("No deaths.");
    goto L2000;

/* RR-- RESTORE ROBBER */

L26000:
    hack_1.thfact = TRUE_;
    more_output("Restored robber.");
    goto L2000;

/* RT-- RESTORE TROLL */

L27000:
    findex_1.trollf = FALSE_;
    newsta_(oindex_1.troll, 0, rindex_1.mtrol, 0, 0);
    more_output("Restored troll.");
    goto L2000;

/* RC-- RESTORE CYCLOPS */

L28000:
    findex_1.cyclof = FALSE_;
    findex_1.magicf = FALSE_;
    newsta_(oindex_1.cyclo, 0, rindex_1.mcycl, 0, 0);
    more_output("Restored cyclops.");
    goto L2000;


/* RD-- MORTAL MODE */

L29000:
    debug_1.dbgflg = 0;
    more_output("Restored deaths.");
    goto L2000;

/* GDT, PAGE 5 */

/* TK-- TAKE */

L30000:
    if (! (j > 0 && j <= objcts_1.olnt)) {
	goto L2200;
    }
/* 						!VALID OBJECT? */
    newsta_(j, 0, 0, 0, play_1.winner);
/* 						!YES, TAKE OBJECT. */
    more_output("Taken.");
/* 						!TELL. */
    goto L2000;


/* EX-- GOODBYE */

L31000:
    prsvec_1.prscon = 1;
    return;

/* AR--	ALTER ROOM ENTRY */

L32000:
    if (! (j > 0 && j <= rooms_1.rlnt && (k > 0 && k <= 5))) {
	goto L2200;
    }
/* 						!INDICES VALID? */
    printf("Old = %6d      New = ", eqr[j + k * 200 - 201]);
/* 						!TYPE OLD, GET NEW. */
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &eqr[j + k * 200 - 201]);
    goto L2000;


/* AO-- ALTER OBJECT ENTRY */

L33000:
    if (! (j > 0 && j <= objcts_1.olnt && (k > 0 && k <= 14))) {
	goto L2200;
    }
/* 						!INDICES VALID? */
    printf("Old = %6d      New = ", eqo[j + k * 200 - 201]);
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &eqo[j + k * 220 - 221]);
    goto L2000;

/* AA-- ALTER ADVS ENTRY */

L34000:
    if (! (j > 0 && j <= advs_1.alnt && (k > 0 && k <= 7))) {
	goto L2200;
    }
/* 						!INDICES VALID? */
    printf("Old = %6d      New = ", eqa[j + (k << 2) - 5]);
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &eqa[j + (k << 2) - 5]);
    goto L2000;

/* AC-- ALTER CLOCK EVENTS */

L35000:
    if (! (j > 0 && j <= cevent_1.clnt && (k > 0 && k <= 3))) {
	goto L2200;
    }
/* 						!INDICES VALID? */
    if (k == 3) {
	goto L35500;
    }
/* 						!FLAGS ENTRY? */
    printf("Old = %6d      New = ", eqc[j + k * 25 - 26]);
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &eqc[j + k * 25 - 26]);
    goto L2000;

L35500:
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    for (z = buf; *z != '\0'; z++) {
	if (! isspace(*z)) {
	    if (*z == 't' || *z == 'T')
		cevent_1.cflag[j - 1] = 1;
	    else if (*z == 'f' || *z == 'F')
		cevent_1.cflag[j - 1] = 0;
	    break;
	}
    }
    goto L2000;
/* GDT, PAGE 6 */

/* AX-- ALTER EXITS */

L36000:
    if (! (j > 0 && j <= exits_1.xlnt)) {
	goto L2200;
    }
/* 						!ENTRY NO VALID? */
    printf("Old= %6d     New= ", exits_1.travel[j - 1]);
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &exits_1.travel[j - 1]);
    goto L2000;


/* AV-- ALTER VILLAINS */

L37000:
    if (! (j > 0 && j <= vill_1.vlnt && (k > 0 && k <= 5))) {
	goto L2200;
    }
/* 						!INDICES VALID? */
    printf("Old = %6d      New= ", eqv[j + (k << 2) - 5]);
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &eqv[j + (k << 2) - 5]);
    goto L2000;

/* D2-- DISPLAY ROOM2 LIST */

L38000:
    if (! (j > 0 && j <= oroom2_1.r2lnt && (k > 0 && k <= oroom2_1.r2lnt) && 
	    j <= k)) {
	goto L2200;
    }
    i__1 = k;
    for (i = j; i <= i__1; ++i) {
	more_output(NULL);
	printf("#%2d   Room=%6d   Obj=%6d\n", i,
		oroom2_1.rroom2[i - 1], oroom2_1.oroom2[i - 1]);
/* L38100: */
    }
    goto L2000;


/* DN-- DISPLAY SWITCHES */

L39000:
    if (! (j > 0 && j <= smax && (k > 0 && k <= smax) && j <= k)) {
	goto L2200;
    }
/* 						!VALID? */
    i__1 = k;
    for (i = j; i <= i__1; ++i) {
	more_output(NULL);
	printf("Switch #%-2d = %d\n", i, switch_[i - 1]);
/* L39100: */
    }
    goto L2000;


/* AN-- ALTER SWITCHES */

L40000:
    if (! (j > 0 && j <= smax)) {
	goto L2200;
    }
/* 						!VALID ENTRY? */
    printf("Old= %6d      New= ", switch_[j - 1]);
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &switch_[j - 1]);
    goto L2000;

/* DM-- DISPLAY MESSAGES */

L41000:
    if (! (j > 0 && j <= rmsg_1.mlnt && (k > 0 && k <= rmsg_1.mlnt) && j <= k)
	    ) {
	goto L2200;
    }
/* 						!VALID LIMITS? */
    more_output("  RANGE   CONTENTS");
    i__1 = k;
    for (i = j; i <= i__1; i += 10) {
	more_output(NULL);
/* Computing MIN */
	i__2 = i + 9;
	l = min(i__2,k);
	printf("%3d-%3d  ", i, l);
	for (l1 = i; l1 <= l; ++l1)
	    printf(" %6d", rmsg_1.rtext[l1 - 1]);
	printf("\n");
/* L41100: */
    }
    goto L2000;


/* DT-- DISPLAY TEXT */

L42000:
    rspeak_(j);
    goto L2000;

/* AH--	ALTER HERE */

L43000:
    printf("Old= %6d      New= ", play_1.here);
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &play_1.here);
    eqa[0] = play_1.here;
    goto L2000;

/* DP--	DISPLAY PARSER STATE */

L44000:
    more_output(NULL);
    printf("ORPHS= %7d%7d%7d%7d%7d%7d\n",
	   orp[0], orp[1], orp[2], orp[3], orp[4], last_1.lastit);
    more_output(NULL);
    printf("PV=    %7d%7d%7d%7d%7d\n",
	   pvec[0], pvec[1], pvec[2], pvec[3], pvec[4]);
    more_output(NULL);
    printf("SYN=   %7d%7d%7d%7d%7d%7d\n",
	   syn[0], syn[1], syn[2], syn[3], syn[4], syn[5]);
    more_output(NULL);
    printf("              %7d%7d%7d%7d%7d\n",
	   syn[6], syn[7], syn[8], syn[9], syn[10]);
    goto L2000;


/* PD--	PROGRAM DETAIL DEBUG */

L45000:
    printf("Old= %6d      New= ", debug_1.prsflg);
/* 						!TYPE OLD, GET NEW. */
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &debug_1.prsflg);
    goto L2000;

/* DZ--	DISPLAY PUZZLE ROOM */

L46000:
    for (i = 1; i <= 64; i += 8) {
/* 						!DISPLAY PUZZLE */
	more_output(NULL);
	printf(" ");
	for (j = i; j <= i + 7; ++j)
	     printf("%3d", puzzle_1.cpvec[j - 1]);
	printf("\n");
/* L46100: */
    }
    goto L2000;


/* AZ--	ALTER PUZZLE ROOM */

L47000:
    if (! (j > 0 && j <= 64)) {
	goto L2200;
    }
/* 						!VALID ENTRY? */
    printf("Old= %6d      New= ", puzzle_1.cpvec[j - 1]);
/* 						!OUTPUT OLD, */
    (void) fflush(stdout);
    (void) fgets(buf, sizeof buf, stdin);
    more_input();
    sscanf(buf, "%d", &puzzle_1.cpvec[j - 1]);
    goto L2000;

} /* gdt_ */

#endif /* ALLOW_GDT */
