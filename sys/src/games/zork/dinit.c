/* INIT-- DUNGEON INITIALIZATION SUBROUTINE */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include <stdio.h>

#ifdef __AMOS__
#include <amos.h>
#endif

#include "funcs.h"
#include "vars.h"

/* This is here to avoid depending on the existence of <stdlib.h> */

extern void srand P((unsigned int));

FILE *dbfile;

#define TEXTFILE "/sys/games/lib/zork/dtextc.dat"

#ifndef LOCALTEXTFILE
#define LOCALTEXTFILE "dtextc.dat"
#endif

/* Read a single two byte integer from the index file */

#define rdint(indxfile) \
    (ch = getc(indxfile), \
     ((ch > 127) ? (ch - 256) : (ch)) * 256 + getc(indxfile))

/* Read a number of two byte integers from the index file */

static void rdints(c, pi, indxfile)
integer c;
integer *pi;
FILE *indxfile;
{
    integer ch;	/* Local variable for rdint */

    while (c-- != 0)
	*pi++ = rdint(indxfile);
}

/* Read a partial array of integers.  These are stored as index,value
 * pairs.
 */

static void rdpartialints(c, pi, indxfile)
integer c;
integer *pi;
FILE *indxfile;
{
    integer ch;	/* Local variable for rdint */

    while (1) {
	int i;

	if (c < 255) {
	    i = getc(indxfile);
	    if (i == 255)
		return;
	}
	else {
	    i = rdint(indxfile);
	    if (i == -1)
		return;
	}

	pi[i] = rdint(indxfile);
    }
}

/* Read a number of one byte flags from the index file */

static void rdflags(c, pf, indxfile)
integer c;
logical *pf;
FILE *indxfile;
{
    while (c-- != 0)
	*pf++ = getc(indxfile);
}

logical init_()
{
    /* System generated locals */
    integer i__1;
    logical ret_val;

    /* Local variables */
    integer xmax, r2max, dirmax, recno;
    integer i, j, k;
    register integer ch;
    register FILE *indxfile;
    integer mmax, omax, rmax, vmax, amax, cmax, fmax, smax;

    more_init();

/* FIRST CHECK FOR PROTECTION VIOLATION */

    if (protected()) {
	goto L10000;
    }
/* 						!PROTECTION VIOLATION? */
    more_output("There appears before you a threatening figure clad all over");
    more_output("in heavy black armor.  His legs seem like the massive trunk");
    more_output("of the oak tree.  His broad shoulders and helmeted head loom");
    more_output("high over your own puny frame, and you realize that his powerful");
    more_output("arms could easily crush the very life from your body.  There");
    more_output("hangs from his belt a veritable arsenal of deadly weapons:");
    more_output("sword, mace, ball and chain, dagger, lance, and trident.");
    more_output("He speaks with a commanding voice:");
    more_output("");
    more_output("                    \"You shall not pass.\"");
    more_output("");
    more_output("As he grabs you by the neck all grows dim about you.");
    exit_();

/* NOW START INITIALIZATION PROPER */

L10000:
    ret_val = FALSE_;
/* 						!ASSUME INIT FAILS. */
    mmax = 1050;
/* 						!SET UP ARRAY LIMITS. */
    omax = 220;
    rmax = 200;
    vmax = 4;
    amax = 4;
    cmax = 25;
    fmax = 46;
    smax = 22;
    xmax = 900;
    r2max = 20;
    dirmax = 15;

    rmsg_1.mlnt = 0;
/* 						!INIT ARRAY COUNTERS. */
    objcts_1.olnt = 0;
    rooms_1.rlnt = 0;
    vill_1.vlnt = 0;
    advs_1.alnt = 0;
    cevent_1.clnt = 0;
    exits_1.xlnt = 1;
    oroom2_1.r2lnt = 0;

    state_1.ltshft = 10;
/* 						!SET UP STATE VARIABLES. */
    state_1.mxscor = state_1.ltshft;
    state_1.egscor = 0;
    state_1.egmxsc = 0;
    state_1.mxload = 100;
    state_1.rwscor = 0;
    state_1.deaths = 0;
    state_1.moves = 0;
    time_1.pltime = 0;
    state_1.mungrm = 0;
    state_1.hs = 0;
    prsvec_1.prsa = 0;
/* 						!CLEAR PARSE VECTOR. */
    prsvec_1.prsi = 0;
    prsvec_1.prso = 0;
    prsvec_1.prscon = 1;
    orphs_1.oflag = 0;
/* 						!CLEAR ORPHANS. */
    orphs_1.oact = 0;
    orphs_1.oslot = 0;
    orphs_1.oprep = 0;
    orphs_1.oname = 0;
    hack_1.thfflg = FALSE_;
/* 						!THIEF NOT INTRODUCED BUT */
    hack_1.thfact = TRUE_;
/* 						!IS ACTIVE. */
    hack_1.swdact = FALSE_;
/* 						!SWORD IS INACTIVE. */
    hack_1.swdsta = 0;
/* 						!SWORD IS OFF. */

    recno = 1;
/* 						!INIT DB FILE POINTER. */
    star_1.mbase = 0;
/* 						!INIT MELEE BASE. */
/* INIT, PAGE 3 */

/* INIT ALL ARRAYS. */

    i__1 = cmax;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR CLOCK EVENTS */
	cevent_1.cflag[i - 1] = FALSE_;
	cevent_1.ctick[i - 1] = 0;
	cevent_1.cactio[i - 1] = 0;
/* L5: */
    }

    i__1 = fmax;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR FLAGS. */
	flags[i - 1] = FALSE_;
/* L10: */
    }
    findex_1.buoyf = TRUE_;
/* 						!SOME START AS TRUE. */
    findex_1.egyptf = TRUE_;
    findex_1.cagetf = TRUE_;
    findex_1.mr1f = TRUE_;
    findex_1.mr2f = TRUE_;
    findex_1.follwf = TRUE_;
    i__1 = smax;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR SWITCHES. */
	switch_[i - 1] = 0;
/* L12: */
    }
    findex_1.ormtch = 4;
/* 						!NUMBER OF MATCHES. */
    findex_1.lcell = 1;
    findex_1.pnumb = 1;
    findex_1.mdir = 270;
    findex_1.mloc = rindex_1.mrb;
    findex_1.cphere = 10;

    i__1 = r2max;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR ROOM 2 ARRAY. */
	oroom2_1.rroom2[i - 1] = 0;
	oroom2_1.oroom2[i - 1] = 0;
/* L15: */
    }

    i__1 = xmax;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR TRAVEL ARRAY. */
	exits_1.travel[i - 1] = 0;
/* L20: */
    }

    i__1 = vmax;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR VILLAINS ARRAYS. */
	vill_1.vopps[i - 1] = 0;
	vill_1.vprob[i - 1] = 0;
	vill_1.villns[i - 1] = 0;
	vill_1.vbest[i - 1] = 0;
	vill_1.vmelee[i - 1] = 0;
/* L30: */
    }

    i__1 = omax;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR OBJECT ARRAYS. */
	objcts_1.odesc1[i - 1] = 0;
	objcts_1.odesc2[i - 1] = 0;
	objcts_1.odesco[i - 1] = 0;
	objcts_1.oread[i - 1] = 0;
	objcts_1.oactio[i - 1] = 0;
	objcts_1.oflag1[i - 1] = 0;
	objcts_1.oflag2[i - 1] = 0;
	objcts_1.ofval[i - 1] = 0;
	objcts_1.otval[i - 1] = 0;
	objcts_1.osize[i - 1] = 0;
	objcts_1.ocapac[i - 1] = 0;
	objcts_1.ocan[i - 1] = 0;
	objcts_1.oadv[i - 1] = 0;
	objcts_1.oroom[i - 1] = 0;
/* L40: */
    }

    i__1 = rmax;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR ROOM ARRAYS. */
	rooms_1.rdesc1[i - 1] = 0;
	rooms_1.rdesc2[i - 1] = 0;
	rooms_1.ractio[i - 1] = 0;
	rooms_1.rflag[i - 1] = 0;
	rooms_1.rval[i - 1] = 0;
	rooms_1.rexit[i - 1] = 0;
/* L50: */
    }

    i__1 = mmax;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR MESSAGE DIRECTORY. */
	rmsg_1.rtext[i - 1] = 0;
/* L60: */
    }

    i__1 = amax;
    for (i = 1; i <= i__1; ++i) {
/* 						!CLEAR ADVENTURER'S ARRAYS. */
	advs_1.aroom[i - 1] = 0;
	advs_1.ascore[i - 1] = 0;
	advs_1.avehic[i - 1] = 0;
	advs_1.aobj[i - 1] = 0;
	advs_1.aactio[i - 1] = 0;
	advs_1.astren[i - 1] = 0;
	advs_1.aflag[i - 1] = 0;
/* L70: */
    }

    debug_1.dbgflg = 0;
    debug_1.prsflg = 0;
    debug_1.gdtflg = 0;

#ifdef ALLOW_GDT

/* allow setting gdtflg true if user id matches wizard id */
/* this way, the wizard doesn't have to recompile to use gdt */

    if (wizard()) {
	debug_1.gdtflg = 1;
    }

#endif /* ALLOW_GDT */

    screen_1.fromdr = 0;
/* 						!INIT SCOL GOODIES. */
    screen_1.scolrm = 0;
    screen_1.scolac = 0;
/* INIT, PAGE 4 */

/* NOW RESTORE FROM EXISTING INDEX FILE. */

#ifdef __AMOS__
    if ((dbfile = fdopen(ropen(LOCALTEXTFILE, 0), BINREAD)) == NULL &&
	(dbfile = fdopen(ropen(TEXTFILE, 0), BINREAD)) == NULL)
#else
    if ((dbfile = fopen(LOCALTEXTFILE, BINREAD)) == NULL &&
	(dbfile = fopen(TEXTFILE, BINREAD)) == NULL)
#endif
	goto L1950;

    indxfile = dbfile;

    i = rdint(indxfile);
    j = rdint(indxfile);
    k = rdint(indxfile);

/* 						!GET VERSION. */
    if (i != vers_1.vmaj || j != vers_1.vmin) {
	goto L1925;
    }

    state_1.mxscor = rdint(indxfile);
    star_1.strbit = rdint(indxfile);
    state_1.egmxsc = rdint(indxfile);

    rooms_1.rlnt = rdint(indxfile);
    rdints(rooms_1.rlnt, &rooms_1.rdesc1[0], indxfile);
    rdints(rooms_1.rlnt, &rooms_1.rdesc2[0], indxfile);
    rdints(rooms_1.rlnt, &rooms_1.rexit[0], indxfile);
    rdpartialints(rooms_1.rlnt, &rooms_1.ractio[0], indxfile);
    rdpartialints(rooms_1.rlnt, &rooms_1.rval[0], indxfile);
    rdints(rooms_1.rlnt, &rooms_1.rflag[0], indxfile);

    exits_1.xlnt = rdint(indxfile);
    rdints(exits_1.xlnt, &exits_1.travel[0], indxfile);

    objcts_1.olnt = rdint(indxfile);
    rdints(objcts_1.olnt, &objcts_1.odesc1[0], indxfile);
    rdints(objcts_1.olnt, &objcts_1.odesc2[0], indxfile);
    rdpartialints(objcts_1.olnt, &objcts_1.odesco[0], indxfile);
    rdpartialints(objcts_1.olnt, &objcts_1.oactio[0], indxfile);
    rdints(objcts_1.olnt, &objcts_1.oflag1[0], indxfile);
    rdpartialints(objcts_1.olnt, &objcts_1.oflag2[0], indxfile);
    rdpartialints(objcts_1.olnt, &objcts_1.ofval[0], indxfile);
    rdpartialints(objcts_1.olnt, &objcts_1.otval[0], indxfile);
    rdints(objcts_1.olnt, &objcts_1.osize[0], indxfile);
    rdpartialints(objcts_1.olnt, &objcts_1.ocapac[0], indxfile);
    rdints(objcts_1.olnt, &objcts_1.oroom[0], indxfile);
    rdpartialints(objcts_1.olnt, &objcts_1.oadv[0], indxfile);
    rdpartialints(objcts_1.olnt, &objcts_1.ocan[0], indxfile);
    rdpartialints(objcts_1.olnt, &objcts_1.oread[0], indxfile);

    oroom2_1.r2lnt = rdint(indxfile);
    rdints(oroom2_1.r2lnt, &oroom2_1.oroom2[0], indxfile);
    rdints(oroom2_1.r2lnt, &oroom2_1.rroom2[0], indxfile);

    cevent_1.clnt = rdint(indxfile);
    rdints(cevent_1.clnt, &cevent_1.ctick[0], indxfile);
    rdints(cevent_1.clnt, &cevent_1.cactio[0], indxfile);
    rdflags(cevent_1.clnt, &cevent_1.cflag[0], indxfile);

    vill_1.vlnt = rdint(indxfile);
    rdints(vill_1.vlnt, &vill_1.villns[0], indxfile);
    rdpartialints(vill_1.vlnt, &vill_1.vprob[0], indxfile);
    rdpartialints(vill_1.vlnt, &vill_1.vopps[0], indxfile);
    rdints(vill_1.vlnt, &vill_1.vbest[0], indxfile);
    rdints(vill_1.vlnt, &vill_1.vmelee[0], indxfile);

    advs_1.alnt = rdint(indxfile);
    rdints(advs_1.alnt, &advs_1.aroom[0], indxfile);
    rdpartialints(advs_1.alnt, &advs_1.ascore[0], indxfile);
    rdpartialints(advs_1.alnt, &advs_1.avehic[0], indxfile);
    rdints(advs_1.alnt, &advs_1.aobj[0], indxfile);
    rdints(advs_1.alnt, &advs_1.aactio[0], indxfile);
    rdints(advs_1.alnt, &advs_1.astren[0], indxfile);
    rdpartialints(advs_1.alnt, &advs_1.aflag[0], indxfile);

    star_1.mbase = rdint(indxfile);
    rmsg_1.mlnt = rdint(indxfile);
    rdints(rmsg_1.mlnt, &rmsg_1.rtext[0], indxfile);

/* Save location of start of message text */
    rmsg_1.mrloc = ftell(indxfile);

/* 						!INIT DONE. */

/* INIT, PAGE 5 */

/* THE INTERNAL DATA BASE IS NOW ESTABLISHED. */
/* SET UP TO PLAY THE GAME. */

    itime_(&time_1.shour, &time_1.smin, &time_1.ssec);
    srand(time_1.shour ^ (time_1.smin ^ time_1.ssec));

    play_1.winner = aindex_1.player;
    last_1.lastit = advs_1.aobj[aindex_1.player - 1];
    play_1.here = advs_1.aroom[play_1.winner - 1];
    hack_1.thfpos = objcts_1.oroom[oindex_1.thief - 1];
    state_1.bloc = objcts_1.oroom[oindex_1.ballo - 1];
    ret_val = TRUE_;

    return ret_val;
/* INIT, PAGE 6 */

/* ERRORS-- INIT FAILS. */

L1925:
    more_output(NULL);
    printf("%s is version %1d.%1d%c.\n", TEXTFILE, i, j, k);
    more_output(NULL);
    printf("I require version %1d.%1d%c.\n", vers_1.vmaj, vers_1.vmin,
	   vers_1.vedit);
    goto L1975;
L1950:
    more_output(NULL);
    printf("I can't open %s.\n", TEXTFILE);
L1975:
    more_output("Suddenly a sinister, wraithlike figure appears before you,");
    more_output("seeming to float in the air.  In a low, sorrowful voice he says,");
    more_output("\"Alas, the very nature of the world has changed, and the dungeon");
    more_output("cannot be found.  All must now pass away.\"  Raising his oaken staff");
    more_output("in farewell, he fades into the spreading darkness.  In his place");
    more_output("appears a tastefully lettered sign reading:");
    more_output("");
    more_output("                       INITIALIZATION FAILURE");
    more_output("");
    more_output("The darkness becomes all encompassing, and your vision fails.");
    return ret_val;

} /* init_ */
