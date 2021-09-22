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

#define IS_COMPOSITOR (strncmp(&no[i], page_comp, comp_len) == 0)

#define ROMAN_I 'I'
#define ROMAN_V 'V'
#define ROMAN_X 'X'
#define ROMAN_L 'L'
#define ROMAN_C 'C'
#define ROMAN_D 'D'
#define ROMAN_M 'M'

#define ROMAN_i 'i'
#define ROMAN_v 'v'
#define ROMAN_x 'x'
#define ROMAN_l 'l'
#define ROMAN_c 'c'
#define ROMAN_d 'd'
#define ROMAN_m 'm'

#define VAL_I 1
#define VAL_V 5
#define VAL_X 10
#define VAL_L 50
#define VAL_C 100
#define VAL_D 500
#define VAL_M 1000

#define ROMAN_LOWER_VAL(C) \
    ((C == ROMAN_i) ? VAL_I : \
     (C == ROMAN_v) ? VAL_V : \
     (C == ROMAN_x) ? VAL_X : \
     (C == ROMAN_l) ? VAL_L : \
     (C == ROMAN_c) ? VAL_C : \
     (C == ROMAN_d) ? VAL_D : \
     (C == ROMAN_m) ? VAL_M : 0)

#define ROMAN_UPPER_VAL(C) \
    ((C == ROMAN_I) ? VAL_I : \
     (C == ROMAN_V) ? VAL_V : \
     (C == ROMAN_X) ? VAL_X : \
     (C == ROMAN_L) ? VAL_L : \
     (C == ROMAN_C) ? VAL_C : \
     (C == ROMAN_D) ? VAL_D : \
     (C == ROMAN_M) ? VAL_M : 0)

#define IS_ROMAN_LOWER(C) \
    ((C == ROMAN_i) || (C == ROMAN_v) || (C == ROMAN_x) || \
     (C == ROMAN_l) || (C == ROMAN_c) || (C == ROMAN_d) || (C == ROMAN_m))

#define IS_ROMAN_UPPER(C) \
    ((C == ROMAN_I) || (C == ROMAN_V) || (C == ROMAN_X) || \
     (C == ROMAN_L) || (C == ROMAN_C) || (C == ROMAN_D) || (C == ROMAN_M))

#if   (CCD_2000 | OS_MVSXA)
#define ALPHA_VAL(C) ((isalpha(C)) ? (strchr(UPCC,toupper(C))-UPCC) : 0)

#define IS_ALPHA_LOWER(C) (isalpha(C) && islower(C))

#define IS_ALPHA_UPPER(C) (isalpha(C) && isupper(C))
#else
#define ALPHA_VAL(C) \
    ((('A' <= C) && (C <= 'Z')) ? C - 'A' : \
     (('a' <= C) && (C <= 'z')) ? C - 'a' : 0)

#define IS_ALPHA_LOWER(C) \
    (('a' <= C) && (C <= 'z'))

#define IS_ALPHA_UPPER(C) \
    (('A' <= C) && (C <= 'Z'))
#endif                                 /* CCD_2000 | OS_MVSXA */

#define IDX_SKIPLINE { \
    int tmp; \
    while ((tmp = GET_CHAR(idx_fp)) != LFD) \
	if (tmp == EOF) \
	    break; \
    idx_lc++; \
    arg_count = -1; \
}

#define NULL_RTN { \
    IDX_ERROR("Illegal null field.\n", NULL); \
    return (FALSE); \
}

#if   KCC_20
/* KCC preprocessor bug collapses multiple blanks to single blank */
#define IDX_ERROR(F, D) { \
    if (idx_dot) { \
	fprintf(ilg_fp, "\n"); \
	idx_dot = FALSE; \
    } \
    fprintf(ilg_fp, \
	    "!! Input index error (file = %s, line = %d):\n\040\040 -- ", \
	    idx_fn, idx_lc); \
    fprintf(ilg_fp, F, D); \
    idx_ec++; \
}

#define IDX_ERROR2(F, D1, D2) { \
    if (idx_dot) { \
	fprintf(ilg_fp, "\n"); \
	idx_dot = FALSE; \
    } \
    fprintf(ilg_fp, \
	    "!! Input index error (file = %s, line = %d):\n\040\040 -- ", \
	    idx_fn, idx_lc); \
    fprintf(ilg_fp, F, D1, D2); \
    idx_ec++; \
}
#else
#define IDX_ERROR(F, D) { \
    if (idx_dot) { \
	fprintf(ilg_fp, "\n"); \
	idx_dot = FALSE; \
    } \
    fprintf(ilg_fp, "!! Input index error (file = %s, line = %d):\n   -- ", \
	    idx_fn, idx_lc); \
    fprintf(ilg_fp, F, D); \
    idx_ec++; \
}

#define IDX_ERROR2(F, D1, D2) { \
    if (idx_dot) { \
	fprintf(ilg_fp, "\n"); \
	idx_dot = FALSE; \
    } \
    fprintf(ilg_fp, "!! Input index error (file = %s, line = %d):\n   -- ", \
	    idx_fn, idx_lc); \
    fprintf(ilg_fp, F, D1, D2); \
    idx_ec++; \
}
#endif


#define ENTER(V) { \
    if (*count >= PAGEFIELD_MAX) { \
	IDX_ERROR2("Page number %s has too many fields (max. %d).", \
	       no, PAGEFIELD_MAX); \
	return (FALSE); \
    } \
    npg[*count] = (V); \
    ++*count; \
}
