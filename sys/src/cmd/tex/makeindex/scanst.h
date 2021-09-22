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

#define COMMENT   '%'
#define STR_DELIM '"'
#define CHR_DELIM '\''

#define KEYWORD "keyword"
#define AOPEN   "arg_open"
#define ACLOSE  "arg_close"
#define ROPEN   "range_open"
#define RCLOSE  "range_close"
#define LEVEL   "level"
#define QUOTE   "quote"
#define ACTUAL  "actual"
#define ENCAP   "encap"
#define ESCAPE  "escape"

#define IDX_KEYWORD "\\indexentry"
#define IDX_AOPEN   '{'
#define IDX_ACLOSE  '}'
#define IDX_ROPEN   '('
#define IDX_RCLOSE  ')'
#define IDX_LEVEL   '!'
#define IDX_QUOTE   '"'
#define IDX_ACTUAL  '@'
#define IDX_ENCAP   '|'
#define IDX_ESCAPE  '\\'

#define COMPOSITOR         "page_compositor"
#define COMPOSITOR_DEF     "-"
#define PRECEDENCE         "page_precedence"
#define PRECEDENCE_DEF     "rnaRA"
#define ROMAN_LOWER        'r'
#define ROMAN_UPPER        'R'
#define ARABIC             'n'
#define ALPHA_LOWER        'a'
#define ALPHA_UPPER        'A'
#define ROMAN_LOWER_OFFSET 10000
#define ROMAN_UPPER_OFFSET 10000
#define ARABIC_OFFSET      10000
#define ALPHA_LOWER_OFFSET 26
#define ALPHA_UPPER_OFFSET 26

#define PREAMBLE      "preamble"
#define PREAMBLE_DEF  "\\begin{theindex}\n"
#define PREAMBLE_LEN  1
#define POSTAMBLE     "postamble"
#define POSTAMBLE_DEF "\n\n\\end{theindex}\n"
#define POSTAMBLE_LEN 3

#define SETPAGEOPEN  "setpage_prefix"
#define SETPAGECLOSE "setpage_suffix"

#if    KCC_20
/* KCC preprocessor bug collapses multiple blanks to single blank */
#define SETPAGEOPEN_DEF "\n\040\040\\setcounter{page}{"
#else
#define SETPAGEOPEN_DEF "\n  \\setcounter{page}{"
#endif                                 /* KCC_20 */

#define SETPAGECLOSE_DEF "}\n"
#define SETPAGE_LEN      2

#define GROUP_SKIP        "group_skip"
#if    KCC_20
/* KCC preprocessor bug collapses multiple blanks to single blank */
#define GROUPSKIP_DEF "\n\n\040\040\\indexspace\n"
#else
#define GROUPSKIP_DEF "\n\n  \\indexspace\n"
#endif                                 /* KCC_20 */
#define GROUPSKIP_LEN 3

#define HEADINGS_FLAG    "headings_flag"
#define HEADINGSFLAG_DEF 0
#define HEADING_PRE      "heading_prefix"
#define HEADINGPRE_DEF   ""
#define HEADINGPRE_LEN   0
#define HEADING_SUF      "heading_suffix"
#define HEADINGSUF_DEF   ""
#define HEADINGSUF_LEN   0
#define SYMHEAD_POS      "symhead_positive"
#define SYMHEADPOS_DEF   "Symbols"
#define SYMHEAD_NEG      "symhead_negative"
#define SYMHEADNEG_DEF   "symbols"
#define NUMHEAD_POS      "numhead_positive"
#define NUMHEADPOS_DEF   "Numbers"
#define NUMHEAD_NEG      "numhead_negative"
#define NUMHEADNEG_DEF   "numbers"

#define ITEM_0  "item_0"
#define ITEM_1  "item_1"
#define ITEM_2  "item_2"
#define ITEM_01 "item_01"
#define ITEM_x1 "item_x1"
#define ITEM_12 "item_12"
#define ITEM_x2 "item_x2"

#if    KCC_20
/* KCC preprocessor bug collapses multiple blanks to single blank */
#define ITEM0_DEF "\n\040\040\\item\040"
#define ITEM1_DEF "\n\040\040\040\040\\subitem\040"
#define ITEM2_DEF "\n\040\040\040\040\040\040\\subsubitem\040"
#else
#define ITEM0_DEF "\n  \\item "
#define ITEM1_DEF "\n    \\subitem "
#define ITEM2_DEF "\n      \\subsubitem "
#endif                                 /* KCC_20 */

#define ITEM_LEN 1

#define DELIM_0    "delim_0"
#define DELIM_1    "delim_1"
#define DELIM_2    "delim_2"
#define DELIM_N    "delim_n"
#define DELIM_R    "delim_r"
#define DELIM_T    "delim_t"
#define DELIM_DEF  ", "
#define DELIMR_DEF "--"
#define DELIMT_DEF ""

#define SUFFIX_2P  "suffix_2p"
#define SUFFIX_3P  "suffix_3p"
#define SUFFIX_MP  "suffix_mp"

#define ENCAP_0    "encap_prefix"
#define ENCAP_1    "encap_infix"
#define ENCAP_2    "encap_suffix"
#define ENCAP0_DEF "\\"
#define ENCAP1_DEF "{"
#define ENCAP2_DEF "}"

#define LINEMAX       "line_max"
#define INDENT_SPACE  "indent_space"
#define INDENT_LENGTH "indent_length"

#if    (OS_BS2000 | OS_MVSXA)
#define INDENTSPC_DEF "                "
#else
#define INDENTSPC_DEF "\t\t"
#endif                                 /* OS_BS2000 | OS_MVSXA */

#define INDENTLEN_DEF 16

#define STY_ERROR(F, D) { \
    if (idx_dot) { \
	fprintf(ilg_fp, "\n"); \
	idx_dot = FALSE; \
    } \
    fprintf(ilg_fp, "** Input style error (file = %s, line = %d):\n   -- ", \
	    sty_fn, sty_lc); \
    fprintf(ilg_fp, F, D); \
    sty_ec++; \
    put_dot = FALSE; \
}

#if    KCC_20
/* KCC preprocessor bug collapses multiple blanks to single blank */
#define STY_ERROR2(F, D1, D2) { \
     if (idx_dot) { \
	fprintf(ilg_fp, "\n"); \
	idx_dot = FALSE; \
    } \
    fprintf(ilg_fp, \
	    "** Input style error (file = %s, line = %d):\n\040\040 -- ", \
	    sty_fn, sty_lc); \
    fprintf(ilg_fp, F, D1, D2); \
    sty_ec++; \
    put_dot = FALSE; \
}
#else
#define STY_ERROR2(F, D1, D2) { \
     if (idx_dot) { \
	fprintf(ilg_fp, "\n"); \
	idx_dot = FALSE; \
    } \
    fprintf(ilg_fp, "** Input style error (file = %s, line = %d):\n   -- ", \
	    sty_fn, sty_lc); \
    fprintf(ilg_fp, F, D1, D2); \
    sty_ec++; \
    put_dot = FALSE; \
}
#endif                                 /* KCC_20 */

#define STY_DOT { \
    idx_dot = TRUE; \
    if (verbose) \
	fprintf(stderr, DOT); \
    fprintf(ilg_fp, DOT); \
}

#define STY_SKIPLINE { \
    int a; \
    while ( ((a = GET_CHAR(sty_fp)) != LFD) && (a != EOF) ); \
    sty_lc++; \
}

#define SCAN_NO(N) { \
    fscanf(sty_fp, "%d", N); \
}

#define MULTIPLE(C) { \
    STY_SKIPLINE; \
STY_ERROR2( \
"Multiple instances of type `%c' in page precedence specification `%s'.\n", \
C, page_prec); \
    return (FALSE); \
}
