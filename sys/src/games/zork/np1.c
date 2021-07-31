/* SPARSE-	START OF PARSE */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

#undef EXTERN
#define EXTERN
#define INIT

#include "parse.h"

/* THIS ROUTINE DETAILS ON BIT 2 OF PRSFLG */

integer sparse_(lbuf, llnt, vbflag)
const integer *lbuf;
integer llnt;
logical vbflag;
{
    /* Initialized data */

/* 	DATA R50MIN/1RA/,R50WAL/3RWAL/ */
    const integer r50min = 1600;
    const integer r50wal = 36852;

    /* System generated locals */
    integer ret_val, i__1, i__2;

    /* Local variables */
    integer i, j, adj;
    integer obj;
    integer prep, pptr, lbuf1, lbuf2;
    integer buzlnt, prplnt, dirlnt;

    /* Parameter adjustments */
    --lbuf;

    /* Function Body */

/* SET UP FOR PARSING */

    ret_val = -1;
/* 						!ASSUME PARSE FAILS. */
    adj = 0;
/* 						!CLEAR PARTS HOLDERS. */
    pv_1.act = 0;
    prep = 0;
    pptr = 0;
    pv_1.o1 = 0;
    pv_1.o2 = 0;
    pv_1.p1 = 0;
    pv_1.p2 = 0;

    buzlnt = 20;
    prplnt = 48;
    dirlnt = 75;
/* SPARSE, PAGE 8 */

/* NOW LOOP OVER INPUT BUFFER OF LEXICAL TOKENS. */

    i__1 = llnt;
    for (i = 1; i <= i__1; i += 2) {
/* 						!TWO WORDS/TOKEN. */
	lbuf1 = lbuf[i];
/* 						!GET CURRENT TOKEN. */
	lbuf2 = lbuf[i + 1];
	if (lbuf1 == 0) {
	    goto L1500;
	}
/* 						!END OF BUFFER? */

/* CHECK FOR BUZZ WORD */

	i__2 = buzlnt;
	for (j = 1; j <= i__2; j += 2) {
	    if (lbuf1 == buzvoc_1.bvoc[j - 1] && lbuf2 == buzvoc_1.bvoc[j]) {
		goto L1000;
	    }
/* L50: */
	}

/* CHECK FOR ACTION OR DIRECTION */

	if (pv_1.act != 0) {
	    goto L75;
	}
/* 						!GOT ACTION ALREADY? */
	j = 1;
/* 						!CHECK FOR ACTION. */
L125:
	if (lbuf1 == vvoc[j - 1] && lbuf2 == vvoc[j]) {
	    goto L3000;
	}
/* L150: */
	j += 2;
/* 						!ADV TO NEXT SYNONYM. */
	if (! (vvoc[j - 1] > 0 && vvoc[j - 1] < r50min)) {
	    goto L125;
	}
/* 						!ANOTHER VERB? */
	j = j + vvoc[j - 1] + 1;
/* 						!NO, ADVANCE OVER SYNTAX. */
	if (vvoc[j - 1] != -1) {
	    goto L125;
	}
/* 						!TABLE DONE? */

L75:
	if (pv_1.act != 0 && (vvoc[pv_1.act - 1] != r50wal || prep != 0)) {
	    goto L200;
	}
	i__2 = dirlnt;
	for (j = 1; j <= i__2; j += 3) {
/* 						!THEN CHK FOR DIR. */
	    if (lbuf1 == dirvoc_1.dvoc[j - 1] && lbuf2 == dirvoc_1.dvoc[j]) {
		goto L2000;
	    }
/* L100: */
	}

/* NOT AN ACTION, CHECK FOR PREPOSITION, ADJECTIVE, OR OBJECT. */

L200:
	i__2 = prplnt;
	for (j = 1; j <= i__2; j += 3) {
/* 						!LOOK FOR PREPOSITION. */
	    if (lbuf1 == prpvoc_1.pvoc[j - 1] && lbuf2 == prpvoc_1.pvoc[j]) {
		goto L4000;
	    }
/* L250: */
	}

	j = 1;
/* 						!LOOK FOR ADJECTIVE. */
L300:
	if (lbuf1 == avoc[j - 1] && lbuf2 == avoc[j]) {
	    goto L5000;
	}
	++j;
L325:
	++j;
/* 						!ADVANCE TO NEXT ENTRY. */
	if (avoc[j - 1] > 0 && avoc[j - 1] < r50min) {
	    goto L325;
	}
/* 						!A RADIX 50 CONSTANT? */
	if (avoc[j - 1] != -1) {
	    goto L300;
	}
/* 						!POSSIBLY, END TABLE? */

	j = 1;
/* 						!LOOK FOR OBJECT. */
L450:
	if (lbuf1 == ovoc[j - 1] && lbuf2 == ovoc[j]) {
	    goto L600;
	}
	++j;
L500:
	++j;
	if (ovoc[j - 1] > 0 && ovoc[j - 1] < r50min) {
	    goto L500;
	}
	if (ovoc[j - 1] != -1) {
	    goto L450;
	}

/* NOT RECOGNIZABLE */

	if (vbflag) {
	    rspeak_(601);
	}
	return ret_val;
/* SPARSE, PAGE 9 */

/* OBJECT PROCESSING (CONTINUATION OF DO LOOP ON PREV PAGE) */

L600:
	obj = getobj_(j, adj, 0);
/* 						!IDENTIFY OBJECT. */
	if (obj <= 0) {
	    goto L6000;
	}
/* 						!IF LE, COULDNT. */
	if (obj != oindex_1.itobj) {
	    goto L650;
	}
/* 						!"IT"? */
	obj = getobj_(0, 0, last_1.lastit);
/* 						!FIND LAST. */
	if (obj <= 0) {
	    goto L6000;
	}
/* 						!IF LE, COULDNT. */

L650:
	if (prep == 9) {
	    goto L8000;
	}
/* 						!"OF" OBJ? */
	if (pptr == 2) {
	    goto L7000;
	}
/* 						!TOO MANY OBJS? */
	++pptr;
	objvec[pptr - 1] = obj;
/* 						!STUFF INTO VECTOR. */
	prpvec[pptr - 1] = prep;
L700:
	prep = 0;
	adj = 0;
/* Go to end of do loop (moved "1000 CONTINUE" to end of module, to av
oid */
/* complaints about people jumping back into the doloop.) */
	goto L1000;
/* SPARSE, PAGE 10 */

/* SPECIAL PARSE PROCESSORS */

/* 2000--	DIRECTION */

L2000:
	prsvec_1.prsa = vindex_1.walkw;
	prsvec_1.prso = dirvoc_1.dvoc[j + 1];
	ret_val = 1;
	return ret_val;

/* 3000--	ACTION */

L3000:
	pv_1.act = j;
	orphs_1.oact = 0;
	goto L1000;

/* 4000--	PREPOSITION */

L4000:
	if (prep != 0) {
	    goto L4500;
	}
	prep = prpvoc_1.pvoc[j + 1];
	adj = 0;
	goto L1000;

L4500:
	if (vbflag) {
	    rspeak_(616);
	}
	return ret_val;

/* 5000--	ADJECTIVE */

L5000:
	adj = j;
	j = orphs_1.oname & orphs_1.oflag;
	if (j != 0 && i >= llnt) {
	    goto L600;
	}
	goto L1000;

/* 6000--	UNIDENTIFIABLE OBJECT (INDEX INTO OVOC IS J) */

L6000:
	if (obj < 0) {
	    goto L6100;
	}
	j = 579;
	if (lit_(play_1.here)) {
	    j = 618;
	}
	if (vbflag) {
	    rspeak_(j);
	}
	return ret_val;

L6100:
	if (obj != -10000) {
	    goto L6200;
	}
	if (vbflag) {
	    rspsub_(620, objcts_1.odesc2[advs_1.avehic[play_1.winner - 1]
		     - 1]);
	}
	return ret_val;

L6200:
	if (vbflag) {
	    rspeak_(619);
	}
	if (pv_1.act == 0) {
	    pv_1.act = orphs_1.oflag & orphs_1.oact;
	}
	orphan_(- 1, pv_1.act, pv_1.o1, prep, j);
	return ret_val;

/* 7000--	TOO MANY OBJECTS. */

L7000:
	if (vbflag) {
	    rspeak_(617);
	}
	return ret_val;

/* 8000--	RANDOMNESS FOR "OF" WORDS */

L8000:
	if (objvec[pptr - 1] == obj) {
	    goto L700;
	}
	if (vbflag) {
	    rspeak_(601);
	}
	return ret_val;

/* End of do-loop. */

L1000:
	;
    }
/* 						!AT LAST. */

/* NOW SOME MISC CLEANUP -- We fell out of the do-loop */

L1500:
    if (pv_1.act == 0) {
	pv_1.act = orphs_1.oflag & orphs_1.oact;
    }
    if (pv_1.act == 0) {
	goto L9000;
    }
/* 						!IF STILL NONE, PUNT. */
    if (adj != 0) {
	goto L10000;
    }
/* 						!IF DANGLING ADJ, PUNT. */

    if (orphs_1.oflag != 0 && orphs_1.oprep != 0 && prep == 0 && pv_1.o1 != 0 
	    && pv_1.o2 == 0 && pv_1.act == orphs_1.oact) {
	goto L11000;
    }

    ret_val = 0;
/* 						!PARSE SUCCEEDS. */
    if (prep == 0) {
	goto L1750;
    }
/* 						!IF DANGLING PREP, */
    if (pptr == 0 || prpvec[pptr - 1] != 0) {
	goto L12000;
    }
    prpvec[pptr - 1] = prep;
/* 						!CVT TO 'PICK UP FROB'. */

/* 1750--	RETURN A RESULT */

L1750:
/* 						!WIN. */
    return ret_val;
/* 						!LOSE. */

/* 9000--	NO ACTION, PUNT */

L9000:
    if (pv_1.o1 == 0) {
	goto L10000;
    }
/* 						!ANY DIRECT OBJECT? */
    if (vbflag) {
	rspsub_(621, objcts_1.odesc2[pv_1.o1 - 1]);
    }
/* 						!WHAT TO DO? */
    orphan_(- 1, 0, pv_1.o1, 0, 0);
    return ret_val;

/* 10000--	TOTAL CHOMP */

L10000:
    if (vbflag) {
	rspeak_(622);
    }
/* 						!HUH? */
    return ret_val;

/* 11000--	ORPHAN PREPOSITION.  CONDITIONS ARE */
/* 		O1.NE.0, O2=0, PREP=0, ACT=OACT */

L11000:
    if (orphs_1.oslot != 0) {
	goto L11500;
    }
/* 						!ORPHAN OBJECT? */
    pv_1.p1 = orphs_1.oprep;
/* 						!NO, JUST USE PREP. */
    goto L1750;

L11500:
    pv_1.o2 = pv_1.o1;
/* 						!YES, USE AS DIRECT OBJ. */
    pv_1.p2 = orphs_1.oprep;
    pv_1.o1 = orphs_1.oslot;
    pv_1.p1 = 0;
    goto L1750;

/* 12000--	TRUE HANGING PREPOSITION. */
/* 		ORPHAN FOR LATER. */

L12000:
    orphan_(- 1, pv_1.act, 0, prep, 0);
/* 						!ORPHAN PREP. */
    goto L1750;

} /* sparse_ */
