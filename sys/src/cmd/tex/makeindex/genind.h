/*
 *
 *  This file is part of
 *	MakeIndex - A formatter and format independent index processor
 *
 *  Copyright (C) 1989 by Chen & Harrison International Systems, Inc.
 *  Copyright (C) 1988 by Olivetti Research Center
 *  Copyright (C) 1987 by Regents of the University of California
 *
 *  Author:
 *	Pehong Chen
 *	Chen & Harrison International Systems, Inc.
 *	Palo Alto, California
 *	USA
 *	(phc@renoir.berkeley.edu or chen@orc.olivetti.com)
 *
 *  Contributors:
 *	Please refer to the CONTRIB file that comes with this release
 *	for a list of people who have contributed to this and/or previous
 *	release(s) of MakeIndex.
 *
 *  All rights reserved by the copyright holders.  See the copyright
 *  notice distributed with this software for a complete description of
 *  the conditions under which it is made available.
 *
 */

#if    KCC_20
/* KCC preprocessor bug collapses multiple blanks to single blank */
#define IND_ERROR(F, D) { \
    if (idx_dot) { \
	fprintf(ilg_fp, "\n"); \
	idx_dot = FALSE; \
    } \
    fprintf(ilg_fp, \
"## Warning (input = %s, line = %d; output = %s, line = %d):\n\040\040 -- ", \
	    curr->fn, curr->lc, ind_fn, ind_lc+1); \
    fprintf(ilg_fp, F, D); \
    ind_ec++; \
}
#else
#define IND_ERROR(F, D) { \
    if (idx_dot) { \
	fprintf(ilg_fp, "\n"); \
	idx_dot = FALSE; \
    } \
    fprintf(ilg_fp, \
    "## Warning (input = %s, line = %d; output = %s, line = %d):\n   -- ", \
	    curr->fn, curr->lc, ind_fn, ind_lc+1); \
    fprintf(ilg_fp, F, D); \
    ind_ec++; \
}
#endif

#define PUTC(C) { \
    fputc(C, ind_fp); \
}

#define PUT(S) { \
    fputs(S, ind_fp); \
}

#define PUTLN(S) { \
    fputs(S, ind_fp); \
    fputc('\n', ind_fp); \
    ind_lc++; \
}

#define SAVE { \
    begin = end = curr; \
    prev_encap = encap; \
}

