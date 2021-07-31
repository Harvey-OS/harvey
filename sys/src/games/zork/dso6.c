/* GHERE--	IS GLOBAL ACTUALLY IN THIS ROOM? */

/*COPYRIGHT 1980, INFOCOM COMPUTERS AND COMMUNICATIONS, CAMBRIDGE MA. 02142*/
/* ALL RIGHTS RESERVED, COMMERCIAL USAGE STRICTLY PROHIBITED */
/* WRITTEN BY R. M. SUPNIK */

#include "funcs.h"
#include "vars.h"

logical ghere_(obj, rm)
integer obj;
integer rm;
{
    /* System generated locals */
    logical ret_val;

    ret_val = TRUE_;
/* 						!ASSUME WINS. */
    switch (obj - star_1.strbit) {
	case 1:  goto L1000;
	case 2:  goto L1000;
	case 3:  goto L1000;
	case 4:  goto L1000;
	case 5:  goto L1000;
	case 6:  goto L1000;
	case 7:  goto L1000;
	case 8:  goto L1000;
	case 9:  goto L1000;
	case 10:  goto L1000;
	case 11:  goto L1000;
	case 12:  goto L2000;
	case 13:  goto L3000;
	case 14:  goto L4000;
	case 15:  goto L5000;
	case 16:  goto L5000;
	case 17:  goto L5000;
	case 18:  goto L6000;
	case 19:  goto L7000;
	case 20:  goto L8000;
	case 21:  goto L9000;
	case 22:  goto L9100;
	case 23:  goto L8000;
	case 24:  goto L10000;
	case 25:  goto L11000;
    }
    bug_(60, obj);

/* 1000--	STARS ARE ALWAYS HERE */

L1000:
    return ret_val;

/* 2000--	BIRD */

L2000:
    ret_val = rm >= rindex_1.fore1 && rm < rindex_1.clear || rm == 
	    rindex_1.mtree;
    return ret_val;

/* 3000--	TREE */

L3000:
    ret_val = rm >= rindex_1.fore1 && rm < rindex_1.clear && rm != 
	    rindex_1.fore3;
    return ret_val;

/* 4000--	NORTH WALL */

L4000:
    ret_val = rm >= rindex_1.bkvw && rm <= rindex_1.bkbox || rm == 
	    rindex_1.cpuzz;
    return ret_val;

/* 5000--	EAST, SOUTH, WEST WALLS */

L5000:
    ret_val = rm >= rindex_1.bkvw && rm < rindex_1.bkbox || rm == 
	    rindex_1.cpuzz;
    return ret_val;

/* 6000--	GLOBAL WATER */

L6000:
    ret_val = (rooms_1.rflag[rm - 1] & RWATER + RFILL) != 0;
    return ret_val;

/* 7000--	GLOBAL GUARDIANS */

L7000:
    ret_val = rm >= rindex_1.mrc && rm <= rindex_1.mrd || rm >= 
	    rindex_1.mrce && rm <= rindex_1.mrdw || rm == rindex_1.inmir;
    return ret_val;

/* 8000--	ROSE/CHANNEL */

L8000:
    ret_val = rm >= rindex_1.mra && rm <= rindex_1.mrd || rm == 
	    rindex_1.inmir;
    return ret_val;

/* 9000--	MIRROR */
/* 9100		PANEL */

L9100:
    if (rm == rindex_1.fdoor) {
	return ret_val;
    }
/* 						!PANEL AT FDOOR. */
L9000:
    ret_val = rm >= rindex_1.mra && rm <= rindex_1.mrc || rm >= 
	    rindex_1.mrae && rm <= rindex_1.mrcw;
    return ret_val;

/* 10000--	MASTER */

L10000:
    ret_val = rm == rindex_1.fdoor || rm == rindex_1.ncorr || rm == 
	    rindex_1.parap || rm == rindex_1.cell;
    return ret_val;

/* 11000--	LADDER */

L11000:
    ret_val = rm == rindex_1.cpuzz;
    return ret_val;

} /* ghere_ */

/* MRHERE--	IS MIRROR HERE? */

/* DECLARATIONS */

integer mrhere_(rm)
integer rm;
{
    /* System generated locals */
    integer ret_val, i__1;

    if (rm < rindex_1.mrae || rm > rindex_1.mrdw) {
	goto L100;
    }

/* RM IS AN E-W ROOM, MIRROR MUST BE N-S (MDIR= 0 OR 180) */

    ret_val = 1;
/* 						!ASSUME MIRROR 1 HERE. */
    if ((rm - rindex_1.mrae) % 2 == findex_1.mdir / 180) {
	ret_val = 2;
    }
    return ret_val;

/* RM IS NORTH OR SOUTH OF MIRROR.  IF MIRROR IS N-S OR NOT */
/* WITHIN ONE ROOM OF RM, LOSE. */

L100:
    ret_val = 0;
    if ((i__1 = findex_1.mloc - rm, abs(i__1)) != 1 || findex_1.mdir % 180 ==
	     0) {
	return ret_val;
    }

/* RM IS WITHIN ONE OF MLOC, AND MDIR IS E-W */

    ret_val = 1;
    if (rm < findex_1.mloc && findex_1.mdir < 180 || rm > findex_1.mloc && 
	    findex_1.mdir > 180) {
	ret_val = 2;
    }
    return ret_val;
} /* mrhere_ */
