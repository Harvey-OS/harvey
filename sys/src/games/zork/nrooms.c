/* RAPPL2- SPECIAL PURPOSE ROOM ROUTINES, PART 2 */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

static void ewtell_ P((integer, integer));
static void lookto_ P((integer, integer, integer, integer, integer));

logical rappl2_(ri)
integer ri;
{
    /* Initialized data */

    const integer newrms = 38;

    /* System generated locals */
    integer i__1;
    logical ret_val;

    /* Local variables */
    integer i;
    integer j;

    ret_val = TRUE_;
    switch (ri - newrms + 1) {
	case 1:  goto L38000;
	case 2:  goto L39000;
	case 3:  goto L40000;
	case 4:  goto L41000;
	case 5:  goto L42000;
	case 6:  goto L43000;
	case 7:  goto L44000;
	case 8:  goto L45000;
	case 9:  goto L46000;
	case 10:  goto L47000;
	case 11:  goto L48000;
	case 12:  goto L49000;
	case 13:  goto L50000;
	case 14:  goto L51000;
	case 15:  goto L52000;
	case 16:  goto L53000;
	case 17:  goto L54000;
	case 18:  goto L55000;
	case 19:  goto L56000;
	case 20:  goto L57000;
	case 21:  goto L58000;
	case 22:  goto L59000;
	case 23:  goto L60000;
    }
    bug_(70, ri);
    return ret_val;

/* R38--	MIRROR D ROOM */

L38000:
    if (prsvec_1.prsa == vindex_1.lookw) {
	lookto_(rindex_1.fdoor, rindex_1.mrg, 0, 682, 681);
    }
    return ret_val;

/* R39--	MIRROR G ROOM */

L39000:
    if (prsvec_1.prsa == vindex_1.walkiw) {
	jigsup_(685);
    }
    return ret_val;

/* R40--	MIRROR C ROOM */

L40000:
    if (prsvec_1.prsa == vindex_1.lookw) {
	lookto_(rindex_1.mrg, rindex_1.mrb, 683, 0, 681);
    }
    return ret_val;

/* R41--	MIRROR B ROOM */

L41000:
    if (prsvec_1.prsa == vindex_1.lookw) {
	lookto_(rindex_1.mrc, rindex_1.mra, 0, 0, 681);
    }
    return ret_val;

/* R42--	MIRROR A ROOM */

L42000:
    if (prsvec_1.prsa == vindex_1.lookw) {
	lookto_(rindex_1.mrb, 0, 0, 684, 681);
    }
    return ret_val;
/* RAPPL2, PAGE 3 */

/* R43--	MIRROR C EAST/WEST */

L43000:
    if (prsvec_1.prsa == vindex_1.lookw) {
	ewtell_(play_1.here, 683);
    }
    return ret_val;

/* R44--	MIRROR B EAST/WEST */

L44000:
    if (prsvec_1.prsa == vindex_1.lookw) {
	ewtell_(play_1.here, 686);
    }
    return ret_val;

/* R45--	MIRROR A EAST/WEST */

L45000:
    if (prsvec_1.prsa == vindex_1.lookw) {
	ewtell_(play_1.here, 687);
    }
    return ret_val;

/* R46--	INSIDE MIRROR */

L46000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(688);
/* 						!DESCRIBE */

/* NOW DESCRIBE POLE STATE. */

/* CASES 1,2--	MDIR=270 & MLOC=MRB, POLE IS UP OR IN HOLE */
/* CASES 3,4--	MDIR=0 V MDIR=180, POLE IS UP OR IN CHANNEL */
/* CASE 5--	POLE IS UP */

    i = 689;
/* 						!ASSUME CASE 5. */
    if (findex_1.mdir == 270 && findex_1.mloc == rindex_1.mrb) {
	i = min(findex_1.poleuf,1) + 690;
    }
    if (findex_1.mdir % 180 == 0) {
	i = min(findex_1.poleuf,1) + 692;
    }
    rspeak_(i);
/* 						!DESCRIBE POLE. */
    i__1 = findex_1.mdir / 45 + 695;
    rspsub_(694, i__1);
/* 						!DESCRIBE ARROW. */
    return ret_val;
/* RAPPL2, PAGE 4 */

/* R47--	MIRROR EYE ROOM */

L47000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 704;
/* 						!ASSUME BEAM STOP. */
    i__1 = objcts_1.olnt;
    for (j = 1; j <= i__1; ++j) {
	if (qhere_(j, play_1.here) && j != oindex_1.rbeam) {
	    goto L47200;
	}
/* L47100: */
    }
    i = 703;
L47200:
    rspsub_(i, objcts_1.odesc2[j - 1]);
/* 						!DESCRIBE BEAM. */
    lookto_(rindex_1.mra, 0, 0, 0, 0);
/* 						!LOOK NORTH. */
    return ret_val;

/* R48--	INSIDE CRYPT */

L48000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 46;
/* 						!CRYPT IS OPEN/CLOSED. */
    if ((objcts_1.oflag2[oindex_1.tomb - 1] & OPENBT) != 0) {
	i = 12;
    }
    rspsub_(705, i);
    return ret_val;

/* R49--	SOUTH CORRIDOR */

L49000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    rspeak_(706);
/* 						!DESCRIBE. */
    i = 46;
/* 						!ODOOR IS OPEN/CLOSED. */
    if ((objcts_1.oflag2[oindex_1.odoor - 1] & OPENBT) != 0) {
	i = 12;
    }
    if (findex_1.lcell == 4) {
	rspsub_(707, i);
    }
/* 						!DESCRIBE ODOOR IF THERE. */
    return ret_val;

/* R50--	BEHIND DOOR */

L50000:
    if (prsvec_1.prsa != vindex_1.walkiw) {
	goto L50100;
    }
/* 						!WALK IN? */
    cevent_1.cflag[cindex_1.cevfol - 1] = TRUE_;
/* 						!MASTER FOLLOWS. */
    cevent_1.ctick[cindex_1.cevfol - 1] = -1;
    return ret_val;

L50100:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 46;
/* 						!QDOOR IS OPEN/CLOSED. */
    if ((objcts_1.oflag2[oindex_1.qdoor - 1] & OPENBT) != 0) {
	i = 12;
    }
    rspsub_(708, i);
    return ret_val;
/* RAPPL2, PAGE 5 */

/* R51--	FRONT DOOR */

L51000:
    if (prsvec_1.prsa == vindex_1.walkiw) {
	cevent_1.ctick[cindex_1.cevfol - 1] = 0;
    }
/* 						!IF EXITS, KILL FOLLOW. */
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    lookto_(0, rindex_1.mrd, 709, 0, 0);
/* 						!DESCRIBE SOUTH. */
    i = 46;
/* 						!PANEL IS OPEN/CLOSED. */
    if (findex_1.inqstf) {
	i = 12;
    }
/* 						!OPEN IF INQ STARTED. */
    j = 46;
/* 						!QDOOR IS OPEN/CLOSED. */
    if ((objcts_1.oflag2[oindex_1.qdoor - 1] & OPENBT) != 0) {
	j = 12;
    }
    rspsb2_(710, i, j);
    return ret_val;

/* R52--	NORTH CORRIDOR */

L52000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 46;
    if ((objcts_1.oflag2[oindex_1.cdoor - 1] & OPENBT) != 0) {
	i = 12;
    }
/* 						!CDOOR IS OPEN/CLOSED. */
    rspsub_(711, i);
    return ret_val;

/* R53--	PARAPET */

L53000:
    if (prsvec_1.prsa == vindex_1.lookw) {
	i__1 = findex_1.pnumb + 712;
	rspsub_(712, i__1);
    }
    return ret_val;

/* R54--	CELL */

L54000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 721;
/* 						!CDOOR IS OPEN/CLOSED. */
    if ((objcts_1.oflag2[oindex_1.cdoor - 1] & OPENBT) != 0) {
	i = 722;
    }
    rspeak_(i);
    i = 46;
/* 						!ODOOR IS OPEN/CLOSED. */
    if ((objcts_1.oflag2[oindex_1.odoor - 1] & OPENBT) != 0) {
	i = 12;
    }
    if (findex_1.lcell == 4) {
	rspsub_(723, i);
    }
/*						!DESCRIBE. */
    return ret_val;

/* R55--	PRISON CELL */

L55000:
    if (prsvec_1.prsa == vindex_1.lookw) {
	rspeak_(724);
    }
/* 						!LOOK? */
    return ret_val;

/* R56--	NIRVANA CELL */

L56000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 46;
/* 						!ODOOR IS OPEN/CLOSED. */
    if ((objcts_1.oflag2[oindex_1.odoor - 1] & OPENBT) != 0) {
	i = 12;
    }
    rspsub_(725, i);
    return ret_val;
/* RAPPL2, PAGE 6 */

/* R57--	NIRVANA AND END OF GAME */

L57000:
    if (prsvec_1.prsa != vindex_1.walkiw) {
	return ret_val;
    }
/* 						!WALKIN? */
    rspeak_(726);
    score_(0);
/* moved to exit routine	CLOSE(DBCH) */
    exit_();

/* R58--	TOMB ROOM */

L58000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 46;
/* 						!TOMB IS OPEN/CLOSED. */
    if ((objcts_1.oflag2[oindex_1.tomb - 1] & OPENBT) != 0) {
	i = 12;
    }
    rspsub_(792, i);
    return ret_val;

/* R59--	PUZZLE SIDE ROOM */

L59000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    i = 861;
/* 						!ASSUME DOOR CLOSED. */
    if (findex_1.cpoutf) {
	i = 862;
    }
/* 						!OPEN? */
    rspeak_(i);
/* 						!DESCRIBE. */
    return ret_val;

/* R60--	PUZZLE ROOM */

L60000:
    if (prsvec_1.prsa != vindex_1.lookw) {
	return ret_val;
    }
/* 						!LOOK? */
    if (findex_1.cpushf) {
	goto L60100;
    }
/* 						!STARTED PUZZLE? */
    rspeak_(868);
/* 						!NO, DESCRIBE. */
    if ((objcts_1.oflag2[oindex_1.warni - 1] & TCHBT) != 0) {
	rspeak_(869);
    }
    return ret_val;

L60100:
    cpinfo_(880, findex_1.cphere);
/* 						!DESCRIBE ROOM. */
    return ret_val;

} /* rappl2_ */

/* LOOKTO--	DESCRIBE VIEW IN MIRROR HALLWAY */

/* DECLARATIONS */

static void lookto_(nrm, srm, nt, st, ht)
integer nrm;
integer srm;
integer nt;
integer st;
integer ht;
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    integer i, m1, dir, mrbf;

    rspeak_(ht);
/* 						!DESCRIBE HALL. */
    rspeak_(nt);
/* 						!DESCRIBE NORTH VIEW. */
    rspeak_(st);
/* 						!DESCRIBE SOUTH VIEW. */
    dir = 0;
/* 						!ASSUME NO DIRECTION. */
    if ((i__1 = findex_1.mloc - play_1.here, abs(i__1)) != 1) {
	goto L200;
    }
/* 						!MIRROR TO N OR S? */
    if (findex_1.mloc == nrm) {
	dir = 695;
    }
    if (findex_1.mloc == srm) {
	dir = 699;
    }
/* 						!DIR=N/S. */
    if (findex_1.mdir % 180 != 0) {
	goto L100;
    }
/* 						!MIRROR N-S? */
    rspsub_(847, dir);
/* 						!YES, HE SEES PANEL */
    rspsb2_(848, dir, dir);
/* 						!AND NARROW ROOMS. */
    goto L200;

L100:
    m1 = mrhere_(play_1.here);
/* 						!WHICH MIRROR? */
    mrbf = 0;
/* 						!ASSUME INTACT. */
    if (m1 == 1 && ! findex_1.mr1f || m1 == 2 && ! findex_1.mr2f) {
	mrbf = 1;
    }
    i__1 = mrbf + 849;
    rspsub_(i__1, dir);
/* 						!DESCRIBE. */
    if (m1 == 1 && findex_1.mropnf) {
	i__1 = mrbf + 823;
	rspeak_(i__1);
    }
    if (mrbf != 0) {
	rspeak_(851);
    }

L200:
    i = 0;
/* 						!ASSUME NO MORE TO DO. */
    if (nt == 0 && (dir == 0 || dir == 699)) {
	i = 852;
    }
    if (st == 0 && (dir == 0 || dir == 695)) {
	i = 853;
    }
    if (nt + st + dir == 0) {
	i = 854;
    }
    if (ht != 0) {
	rspeak_(i);
    }
/* 						!DESCRIBE HALLS. */
} /* lookto_ */

/* EWTELL--	DESCRIBE E/W NARROW ROOMS */

/* DECLARATIONS */

static void ewtell_(rm, st)
integer rm;
integer st;
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    integer i;
    logical m1;

/* NOTE THAT WE ARE EAST OR WEST OF MIRROR, AND */
/* MIRROR MUST BE N-S. */

    m1 = findex_1.mdir + (rm - rindex_1.mrae) % 2 * 180 == 180;
    i = (rm - rindex_1.mrae) % 2 + 819;
/* 						!GET BASIC E/W STRING. */
    if (m1 && ! findex_1.mr1f || ! m1 && ! findex_1.mr2f) {
	i += 2;
    }
    rspeak_(i);
    if (m1 && findex_1.mropnf) {
	i__1 = (i - 819) / 2 + 823;
	rspeak_(i__1);
    }
    rspeak_(825);
    rspeak_(st);
} /* ewtell_ */
