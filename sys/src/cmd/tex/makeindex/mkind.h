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

/***********************************************************************

			   INSTALLATION NOTES
			      <11-Nov-1989>

   At compile-time (or here, if compile-time definition is not
   available), set non-zero ONE OS_xxxx operating system flag, and if
   more than one compiler is available, ONE compiler flag:

    Operating Systems        Compiler(s)
    =================        ===========
    OS_ATARI
    OS_BS2000                CCD_2000
    OS_BSD
    OS_MVSXA                 IBM_C370
    OS_VMCMS                 IBM_C370
    OS_PCDOS                 IBM_PC_MICROSOFT
    OS_SYSV
    OS_TOPS20                KCC_20 or PCC_20
    OS_VAXVMS
    OS_XENIX
    =================        ===========

   If no OS_xxxx symbol is defined, OS_BSD is assumed.

   If Standard C prototypes are supported, define the symbol
   STDC_PROTOTYPES in the appropriate OS_xxxx section below, and insert
   #include's for the standard system files which define library
   prototypes.  STDC_PROTOTYPES will be defined automatically if
   __STDC__ is; the latter must be defined by all Standard C conformant
   implementations.

   All function declarations in MakeIndex are contained at the end of
   this file.  If 185STDC_PROTOTYPES is not selected, then all the standard
   library functions must be declared explicitly.

   If the host system restricts external names to 6 characters, set
   SHORTNAMES non-zero in the appropriate OS_xxxx section below.

   Installing MakeIndex under BS2000 requires at least Version 8.5
   supporting Multiple Public Volume Sets (MPVS). Define WORK in this
   file to your CATID (default :w:) for temporary files (.ilg, .ind),
   in case of no MPVS support define WORK to NIL.

*/

/**********************************************************************/

#define STDC	(__STDC__ || __cplusplus)

/*
 * Establish needed operating symbols (defaulting to OS_BSD if none
 * specified at compile time).  If you add one, add it to the check
 * list at the end of this section, too.
 */

#ifndef    OS_ATARI
#define OS_ATARI 0
#endif

#ifndef    OS_PCDOS
#define OS_PCDOS 0
#endif

#ifndef    IBM_PC_MICROSOFT
#define IBM_PC_MICROSOFT 0
#endif

#ifndef    IBM_PC_TURBO
#define IBM_PC_TURBO 0
#endif

#ifdef __TURBOC__
#define IBM_PC_TURBO 1
#endif

#ifndef    OS_SYSV
#define OS_SYSV 0
#endif

#ifndef    OS_TOPS20
#define OS_TOPS20 0
#endif

#ifndef    KCC_20
#define KCC_20 0
#endif

#ifndef    PCC_20
#define PCC_20 0
#endif

#ifndef    OS_BSD
#define OS_BSD 0
#endif

#ifndef    OS_VAXVMS
#define OS_VAXVMS 0
#endif

#ifndef    IBM_C370
#define IBM_C370 0
#endif

#ifndef    CCD_2000
#define CCD_2000 0
#endif

#if    OS_TOPS20
#if    (KCC_20 || PCC_20)
#else
#undef PCC_20
#define PCC_20 1                       /* PCC-20 is default for Tops-20 */
#endif                                 /* KCC_20 || PCC_20) */
#endif                                 /* OS_TOPS20 */

#ifndef    OS_BS2000
#define OS_BS2000 0
#endif

#ifndef    OS_MVSXA
#define OS_MVSXA 0
#endif

#ifndef    OS_VMCMS
#define OS_VMCMS 0
#endif

#ifndef    OS_XENIX
#define OS_XENIX 0
#endif

#if	(OS_ATARI || OS_BSD || OS_BS2000 || OS_MVSXA || OS_PCDOS || OS_VMCMS)
#else
#if	(OS_SYSV || OS_TOPS20 || OS_VAXVMS || OS_XENIX)
#else
#undef OS_BSD
#define OS_BSD 1                      /* Unix is default operating system */
#endif
#endif

#if    (OS_MVSXA || OS_VMCMS)
#undef IBM_C370
#define IBM_C370 1	/* IBM_C370 is default for OS_MVSXA and OS_VMCMS */
#endif

#if    OS_BS2000
#undef CCD_2000
#define CCD_2000 1                     /* CCD_2000 is default for OS_BS2000 */
#endif

#if    (CCD_2000 || OS_MVSXA || OS_TOPS20 || OS_VMCMS)
#define SHORTNAMES 1
#else
#define SHORTNAMES 0
#endif

#define STDC_PROTOTYPES STDC

/**********************************************************************/

#include    <stdio.h>
#include    <ctype.h>

#if    (CCD_2000 || OS_SYSV || OS_PCDOS || STDC || _AIX || ardent )
#include    <string.h>
#else

#if    (OS_ATARI || OS_BSD || PCC_20 || OS_XENIX)
#define strchr  index                  /* convert STDC form to old K&R form */
#define strrchr rindex                 /* convert STDC form to old K&R form */
#endif /* (OS_ATARI || OS_BSD || PCC_20 || OS_XENIX) */

#endif /* (CCD_2000 || OS_SYSV || OS_PCDOS || STDC || _AIX || ardent) */

#if    OS_PCDOS

#if    (IBM_PC_MICROSOFT || IBM_PC_TURBO)
#include    <io.h>                     /* for function declarations */
#endif /* (IBM_PC_MICROSOFT || IBM_PC_TURBO) */

#include    <stddef.h>                 /* for function declarations */
#include    <stdlib.h>                 /* for function declarations */
#undef STDC_PROTOTYPES
#define STDC_PROTOTYPES    1           /* so we get full argument
					  type checking */
#endif					/* OS_PCDOS */

#if    (OS_VMCMS || OS_MVSXA)
#include    <stdefs.h>                 /* for function declarations */
#include    <string.h>                 /* for function declarations */
#undef STDC_PROTOTYPES
#define STDC_PROTOTYPES    1           /* so we get full argument
                                          type checking */
#if 0
#undef __STDC__
#define __STDC__           1
#endif
#endif                                 /* OS_MVSXA || OS_VMCMS */

#if    OS_ATARI

#if    ATARI_ST_TURBO
#define access(fn, mode) 0             /* function not available */
#endif

#if    __GNUC__
#include <string.h>
#include <types.h>
#endif

#undef STDC_PROTOTYPES
#define STDC_PROTOTYPES    1           /* so we get full argument checking */

#endif                     /* OS_ATARI */

#if    IBM_C370
#define access(fn, mode) 0             /* function not available */
#endif

#if    CCD_2000                        /* function not available */
#define getenv(P) 0
#endif                                 /* CCD_2000 */

#if    SHORTNAMES
/*
 * Provide alternate external names which are unique in the first SIX
 * characters as required for portability (and Standard C)
 */
#define check_all          chk_all
#define check_idx          chk_idx
#define check_mixsym       chk_mix
#define compare_one        cmp_one
#define compare_page       cmp_page
#define compare_string     cmp_string
#define delim_n            dlm_n
#define delim_p            dlm_p
#define delim_r            dlm_r
#define delim_t            dlm_t
#define encap_i            ecp_i
#define encap_p            ecp_p
#define encap_range        ecp_range
#define encap_s            ecp_s
#define group_skip         grp_skip
#define group_type         grp_type
#define idx_aclose         idxaclose
#define idx_actual         idxactual
#define idx_keyword        idx_kwd
#define indent_length      ind_length
#define indent_space       ind_space
#define headings_flag      hd_flag
#define heading_pre        hd_pre
#define heading_suf        hd_suf
#define symhead_pos        sym_pos
#define symhead_neg        sym_neg
#define numhead_pos        num_pos
#define numhead_neg        num_neg
#define process_idx        prc_idx
#define process_precedence prc_pre
#define range_ptr          rng_ptr
#define scan_alpha_lower   scnalw
#define scan_alpha_upper   scnaup
#define scan_arabic        scnarabic
#define scan_arg1          scna1
#define scan_arg2          scna2
#define scan_char          scnchr
#define scan_field         scnfld
#define scan_idx           scnidx
#define scan_key           scnkey
#define scan_no            scnno
#define scan_roman_lower   scnrlw
#define scan_roman_upper   scnrup
#define scan_spec          scnspc
#define scan_string        scnstr
#define scan_style         scnsty
#define setpagelen         spg_len
#define setpage_close      spg_close
#define setpage_open       spg_open
#define suffix_2p          suf_2p
#define suffix_3p          suf_3p
#define suffix_mp          suf_mp
#endif                                 /* SHORTNAMES */

#if    OS_VAXVMS
#define EXIT(code) exit( ((code) == 0) ? 1 : (1 << 28) + 2 )
#else                                  /* NOT OS_VAXVMS */
#define EXIT exit
#endif                                 /* OS_VAXVMS */

#undef TRUE
#define TRUE 1

#undef FALSE
#define FALSE 0

#undef NUL
#define NUL '\0'

#undef NIL
#define NIL ""

#define TAB '\t'
#define LFD '\n'
#define SPC ' '
#define LSQ '['
#define RSQ ']'
#define BSH '\\'

#if    OS_ATARI
#define ENV_SEPAR ';'
#define DIR_DELIM '\\'
#endif                                 /* OS_ATARI */

#if    (OS_BSD || OS_SYSV || OS_XENIX)
#define ENV_SEPAR ':'
#define DIR_DELIM '/'
#endif                                 /* (OS_BSD || OS_SYSV || OS_XENIX)*/

#if    OS_BS2000
#define ENV_SEPAR '#'
#define DIR_DELIM '$'
#endif                                 /* OS_BS2000 */

#if    (OS_MVSXA || OS_VMCMS)
#define ENV_SEPAR '.'
#define DIR_DELIM '.'
#endif                                 /* OS_MVSXA || OS_VMCMS */

#if    OS_PCDOS
#define ENV_SEPAR ';'
#define DIR_DELIM '\\'
#endif                                 /* OS_PCDOS */

#if    OS_TOPS20
#define ENV_SEPAR ','
#define DIR_DELIM ':'
#endif                                 /* OS_TOPS20 */

#if    OS_BSD
#define ENV_SEPAR ':'
#define DIR_DELIM '/'
#endif                                 /* OS_BSD */

#if    OS_VAXVMS
#define ENV_SEPAR ','                  /* In fact these aren't used */
#define DIR_DELIM ':'
#endif                                 /* OS_VAXVMS */

#ifndef SW_PREFIX			/* can override at compile time */
#define SW_PREFIX  '-'
#endif

#define EXT_DELIM  '.'
#define ROMAN_SIGN '*'
#define EVEN       "even"
#define ODD        "odd"
#define ANY        "any"

#define GET_CHAR getc

#if    (CCD_2000 || OS_MVSXA)
#define TOASCII(i) (char)((i) + '0')
#else
#define TOASCII(i) (char)((i) + 48)
#endif                                 /* (CCD_2000 || OS_MVSXA) */

/* NB: The typecasts here are CRITICAL for correct sorting of entries
   that use characters in the range 128..255! */
#define TOLOWER(C) (isupper((unsigned char)(C)) ? \
	(unsigned char)tolower((unsigned char)(C)) : (unsigned char)(C))
#define TOUPPER(C) (isupper((unsigned char)(C)) ? \
	(unsigned char)(C) : (unsigned char)toupper((unsigned char)(C)))

#define STREQ(A, B)  (strcmp(A, B) == 0)
#define STRNEQ(A, B) (strcmp(A, B) != 0)

#define MESSAGE(F, S) { \
    if (verbose) \
	fprintf(stderr, F, S); \
    fprintf(ilg_fp, F, S); \
}

#define FATAL(F, S) { \
    fprintf(stderr, F, S); \
    fprintf(stderr, USAGE, pgm_fn); \
    EXIT(1); \
}

#define FATAL2(F, D1, D2) { \
    fprintf(stderr, F, D1, D2); \
    fprintf(stderr, USAGE, pgm_fn); \
    EXIT(1); \
}

#if    OS_PCDOS                        /* open file in binary mode */
#define OPEN_IN_B(FP) fopen(FP, "rb")
#endif

#define OPEN_IN(FP)   fopen(FP, "r")
#define OPEN_OUT(FP)  fopen(FP, "w")
#define CLOSE(FP)     fclose(FP)

#if    (CCD_2000 || OS_MVSXA)
#define ASCII 1
#define ISDIGIT(C)  isdigit(C)
#define ISSYMBOL(C) (! (isdigit(C) || isalpha(C) || iscntrl(C) || C == SPC))
#else
#define ISDIGIT(C)  ('0' <= C && C <= '9')
#define ISSYMBOL(C) (('!' <= C && C <= '@') || \
		     ('[' <= C && C <= '`') || \
		     ('{' <= C && C <= '~'))
#endif                                 /* (CCD_2000 || OS_MVSXA) */

/*====================================================================
Many arrays in MakeIndex are dimensioned [xxx_MAX], where the xxx_MAX
values are defined below.  The use of each of these is described in
comments.  However, no run-time check is made to ensure that these are
consistent, or reasonable!  Therefore, change them only with great
care.

The array sizes should be made generously large: there are a great
many uses of strings in MakeIndex with the strxxx() and sprintf()
functions where no checking is done for adequate target storage sizes.
Although some input checking is done to avoid this possibility, it is
highly likely that there are still several places where storage
violations are possible, with core dumps, or worse, incorrect output,
ensuing.
======================================================================*/

#define ARABIC_MAX    5		/* maximum digits in an Arabic page */
				/* number field */

#define ARGUMENT_MAX  1024	/* maximum length of sort or actual key */
				/* in index entry */

#define ARRAY_MAX     1024	/* maximum length of constant values in */
				/* style file */

#define FIELD_MAX	3	/* maximum levels of index entries (item, */
				/* subitem, subsubitem); cannot be */
				/* increased beyond 3 without */
				/* significant design changes (no field */
				/* names are known beyond 3 levels) */

#ifdef LINE_MAX		/* IBM RS/6000 AIX has this in <sys/limits.h> */
#undef LINE_MAX
#endif
#define LINE_MAX      72	/* maximum output line length (longer */
				/* ones wrap if possible) */

#define NUMBER_MAX    16	/* maximum digits in a Roman or Arabic */
				/* page number */
				/* (MAX(ARABIC_MAX,ROMAN_MAX)) */

#define PAGEFIELD_MAX 10	/* maximum fields in a composite page */
				/* number */

#define PAGETYPE_MAX  5		/* fixed at 5; see use in scanst.c */

#define ROMAN_MAX     16	/* maximum length of Roman page number */
				/* field */

#define STRING_MAX    256	/* maximum length of host filename */

/*====================================================================*/

#define VERSION       "portable version 2.12 [26-May-1993]"

#if 0
#define VERSION       "portable version 2.11 [19-Oct-1991]"
#define VERSION       "portable version 2.10 [05-Jul-1991]"
#define VERSION       "portable version 2.9.1 [21-Jun-1990]"
#define VERSION       "portable version 2.9 [13-Dec-1989]"
#define VERSION       "portable version 2.8 [11-Nov-1989]"
#define VERSION       "portable version 2.7 [01-Oct-1988]"
#define VERSION       "portable version 2.6 [14-Jul-1988]"
#define VERSION       "portable version 2.5 [14-Apr-1988]"
#define VERSION       "portable version 2.4 [20-Mar-1988]"
#define VERSION       "portable version 2.3 [20-Jan-1988]"
#define VERSION       "version 2.2 [29-May-1987]"
#endif

#define PUT_VERSION { \
    MESSAGE("This is %s, ", pgm_fn); \
    MESSAGE("%s.\n", VERSION); \
    need_version = FALSE; \
}

#define USAGE \
   "Usage: %s [-ilqrcg] [-s sty] [-o ind] [-t log] [-p num] [idx0 idx1 ...]\n"

#define STYLE_PATH "INDEXSTYLE"	/* environment variable defining search */
				/* path for style files */
#define INDEX_IDX  ".idx"
#define INDEX_ILG  ".ilg"
#define INDEX_IND  ".ind"
#define INDEX_STY  ".mst"

#if    OS_VAXVMS
#define INDEX_LOG  ".lis"
#else
#define INDEX_LOG  ".log"
#endif

#if    OS_BS2000                       /* Define WORK disk */
#define WORK ":w:"
#define CATID ':'
#endif                                 /* OS_BS2000 */

#define EMPTY     -9999
#define ROML      0
#define ROMU      1
#define ARAB      2
#define ALPL      3
#define ALPU      4
#define DUPLICATE 9999

#define SYMBOL -1
#define ALPHA  -2

#define GERMAN 0

typedef struct KFIELD
{
    char    *sf[FIELD_MAX];		/* sort key */
    char    *af[FIELD_MAX];		/* actual key */
    int     group;			/* key group */
    char    lpg[NUMBER_MAX];		/* literal page */
    short   npg[PAGEFIELD_MAX];		/* page field array */
    short   count;			/* page field count */
    short   type;			/* page number type */
    char    *encap;			/* encapsulator */
    char    *fn;			/* input filename */
    int     lc;				/* line number */
}	FIELD, *FIELD_PTR;

typedef struct KNODE
{
    FIELD   data;
    struct KNODE *next;
}	NODE, *NODE_PTR;

extern int letter_ordering;
extern int compress_blanks;
extern int init_page;
extern int merge_page;
extern int even_odd;
extern int verbose;
extern int german_sort;

extern char idx_keyword[ARRAY_MAX];
extern char idx_aopen;
extern char idx_aclose;
extern char idx_level;
extern char idx_ropen;
extern char idx_rclose;
extern char idx_quote;
extern char idx_actual;
extern char idx_encap;
extern char idx_escape;

extern char page_comp[ARRAY_MAX];
extern int page_offset[PAGETYPE_MAX];

extern char preamble[ARRAY_MAX];
extern char postamble[ARRAY_MAX];
extern char setpage_open[ARRAY_MAX];
extern char setpage_close[ARRAY_MAX];
extern char group_skip[ARRAY_MAX];
extern int headings_flag;
extern char heading_pre[ARRAY_MAX];
extern char heading_suf[ARRAY_MAX];
extern char symhead_pos[ARRAY_MAX];
extern char symhead_neg[ARRAY_MAX];
extern char numhead_pos[ARRAY_MAX];
extern char numhead_neg[ARRAY_MAX];
extern int prelen;
extern int postlen;
extern int skiplen;
extern int headprelen;
extern int headsuflen;
extern int setpagelen;

extern char item_r[FIELD_MAX][ARRAY_MAX];
extern char item_u[FIELD_MAX][ARRAY_MAX];
extern char item_x[FIELD_MAX][ARRAY_MAX];
extern int ilen_r[FIELD_MAX];
extern int ilen_u[FIELD_MAX];
extern int ilen_x[FIELD_MAX];

extern char delim_p[FIELD_MAX][ARRAY_MAX];
extern char delim_n[ARRAY_MAX];
extern char delim_r[ARRAY_MAX];
extern char delim_t[ARRAY_MAX];

extern char suffix_2p[ARRAY_MAX];
extern char suffix_3p[ARRAY_MAX];
extern char suffix_mp[ARRAY_MAX];

extern char encap_p[ARRAY_MAX];
extern char encap_i[ARRAY_MAX];
extern char encap_s[ARRAY_MAX];

extern int linemax;
extern char indent_space[ARRAY_MAX];
extern int indent_length;

extern FILE *idx_fp;
extern FILE *sty_fp;
extern FILE *ind_fp;
extern FILE *ilg_fp;

#if    OS_BS2000
extern FILE *widx_fp;
extern char widx_fn[STRING_MAX + 5];
#endif                                 /* OS_BS2000 */

extern char *idx_fn;
extern char *pgm_fn;
extern char *ind_fn;
extern char *ilg_fn;

#ifndef MKIND_C
extern char sty_fn[];
extern char ind[];
extern char ilg[];
extern char pageno[];

#ifdef DEBUG
extern long totmem;
#endif /* DEBUG */

#endif

extern FIELD_PTR *idx_key;
extern NODE_PTR head;
extern NODE_PTR tail;

extern int idx_dot;
extern int idx_tt;
extern int idx_gt;
extern int idx_et;
extern int idx_dc;

#define DOT     "."
#define DOT_MAX 1000
#define CMP_MAX 1500

#define IDX_DOT(MAX) { \
    idx_dot = TRUE; \
    if (idx_dc++ == 0) { \
	if (verbose) \
	    fprintf(stderr, DOT); \
	fprintf(ilg_fp, DOT); \
    } \
    if (idx_dc == MAX) \
	idx_dc = 0; \
}

#define ALL_DONE { \
    if (fn_no > 0) { \
	if (verbose) \
fprintf(stderr, \
	"Overall %d files read (%d entries accepted, %d rejected).\n", \
	fn_no+1, idx_gt, idx_et); \
fprintf(ilg_fp, \
	"Overall %d files read (%d entries accepted, %d rejected).\n", \
	fn_no+1, idx_gt, idx_et); \
    } \
}

#define DONE(A, B, C, D) { \
    if (verbose) \
	fprintf(stderr, "done (%d %s, %d %s).\n", (A), B, C, D); \
    fprintf(ilg_fp, "done (%d %s, %d %s).\n", (A), B, C, D); \
}

#if    STDC_PROTOTYPES
#define ARGS(arg_list)	arg_list
#define VOIDP		void*
#define VOID_ARG	void
#else
#define ARGS(arg_list)	()
#define const
#define VOIDP		char*
#define VOID_ARG
#endif

extern void gen_ind ARGS((void));
extern int group_type ARGS((char *str));
extern int main ARGS((int argc, char **argv));
extern void qqsort ARGS((char *base, int n, int size,
		int (*compar)ARGS((char*,char*))));
extern void scan_idx ARGS((void));
extern void scan_sty ARGS((void));
extern void sort_idx ARGS((void));
extern int strtoint ARGS((char *str));

#if STDC
#include <stdlib.h>
#undef strchr
#undef strrchr
#if __NeXT__
int	access ARGS((const char *, int));
#else
#include <unistd.h>
#endif
#else
/* Miscellaneous standard library routines */
int	access ARGS((const char *, int));
VOIDP	calloc ARGS((size_t nitems,size_t size));

#if    CCD_2000                        /* function not available */
#define getenv(P) 0
#else
char   *getenv ARGS((const char *name));
#endif

VOIDP	malloc ARGS((size_t size));
char   *strchr ARGS((const char *s,int c));
char   *strrchr ARGS((const char *s,int c));
#endif /* __STDC__ */

#if    PCC_20
#define R_OK 0                         /* PCC-20 access(file,mode) */
#endif                                 /* only understands mode=0  */

#ifndef    R_OK
#define R_OK 4                         /* only symbol from sys/file.h */
#endif
