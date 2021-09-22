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

#include    "mkind.h"
#include    "scanst.h"

static int sty_lc = 0;                 /* line count */
static int sty_tc = 0;                 /* total count */
static int sty_ec = 0;                 /* error count */

char    idx_keyword[ARRAY_MAX] = IDX_KEYWORD;
char    idx_aopen = IDX_AOPEN;
char    idx_aclose = IDX_ACLOSE;
char    idx_level = IDX_LEVEL;
char    idx_ropen = IDX_ROPEN;
char    idx_rclose = IDX_RCLOSE;
char    idx_quote = IDX_QUOTE;
char    idx_actual = IDX_ACTUAL;
char    idx_encap = IDX_ENCAP;
char    idx_escape = IDX_ESCAPE;

char    preamble[ARRAY_MAX] = PREAMBLE_DEF;
char    postamble[ARRAY_MAX] = POSTAMBLE_DEF;
int     prelen = PREAMBLE_LEN;
int     postlen = POSTAMBLE_LEN;

char    setpage_open[ARRAY_MAX] = SETPAGEOPEN_DEF;
char    setpage_close[ARRAY_MAX] = SETPAGECLOSE_DEF;
int     setpagelen = SETPAGE_LEN;

char    group_skip[ARRAY_MAX] = GROUPSKIP_DEF;
int     skiplen = GROUPSKIP_LEN;

int     headings_flag = HEADINGSFLAG_DEF;
char    heading_pre[ARRAY_MAX] = HEADINGPRE_DEF;
char    heading_suf[ARRAY_MAX] = HEADINGSUF_DEF;
int     headprelen = HEADINGPRE_LEN;
int     headsuflen = HEADINGSUF_LEN;

char    symhead_pos[ARRAY_MAX] = SYMHEADPOS_DEF;
char    symhead_neg[ARRAY_MAX] = SYMHEADNEG_DEF;

char    numhead_pos[ARRAY_MAX] = NUMHEADPOS_DEF;
char    numhead_neg[ARRAY_MAX] = NUMHEADNEG_DEF;

char    item_r[FIELD_MAX][ARRAY_MAX] = {ITEM0_DEF, ITEM1_DEF, ITEM2_DEF};
char    item_u[FIELD_MAX][ARRAY_MAX] = {"", ITEM1_DEF, ITEM2_DEF};
char    item_x[FIELD_MAX][ARRAY_MAX] = {"", ITEM1_DEF, ITEM2_DEF};

int     ilen_r[FIELD_MAX] = {ITEM_LEN, ITEM_LEN, ITEM_LEN};
int     ilen_u[FIELD_MAX] = {0, ITEM_LEN, ITEM_LEN};
int     ilen_x[FIELD_MAX] = {0, ITEM_LEN, ITEM_LEN};

char    delim_p[FIELD_MAX][ARRAY_MAX] = {DELIM_DEF, DELIM_DEF, DELIM_DEF};
char    delim_n[ARRAY_MAX] = DELIM_DEF; /* page number separator */
char    delim_r[ARRAY_MAX] = DELIMR_DEF;/* page range designator */
char    delim_t[ARRAY_MAX] = DELIMT_DEF;/* page list terminating delimiter */

char    suffix_2p[ARRAY_MAX] = "";      /* suffix for two page ranges */
char    suffix_3p[ARRAY_MAX] = "";      /* suffix for three page ranges */
char    suffix_mp[ARRAY_MAX] = "";      /* suffix for multiple page ranges */

char    encap_p[ARRAY_MAX] = ENCAP0_DEF;/* encapsulator prefix */
char    encap_i[ARRAY_MAX] = ENCAP1_DEF;/* encapsulator infix */
char    encap_s[ARRAY_MAX] = ENCAP2_DEF;/* encapsulator postfix */

int     linemax = LINE_MAX;

int     indent_length = INDENTLEN_DEF;
char    indent_space[ARRAY_MAX] = INDENTSPC_DEF;

char    page_comp[ARRAY_MAX] = COMPOSITOR_DEF;
int     page_offset[PAGETYPE_MAX] = {
0,
ROMAN_LOWER_OFFSET,
ROMAN_LOWER_OFFSET + ARABIC_OFFSET,
ROMAN_LOWER_OFFSET + ARABIC_OFFSET + ALPHA_LOWER_OFFSET,
ROMAN_LOWER_OFFSET + ARABIC_OFFSET + ALPHA_LOWER_OFFSET + ROMAN_UPPER_OFFSET
};
static char page_prec[ARRAY_MAX] = PRECEDENCE_DEF;

static int put_dot;

static	int	count_lfd ARGS((char *str));
static	int	next_nonblank ARGS((void));
static	int	process_precedence ARGS((void));
static	int	scan_char ARGS((char *c));
static	int	scan_spec ARGS((char *spec));
static	int	scan_string ARGS((char *str));

void
scan_sty(VOID_ARG)
{
    char    spec[STRING_MAX];
    int     tmp;

    MESSAGE("Scanning style file %s", sty_fn);
    while (scan_spec(spec)) {
	sty_tc++;
	put_dot = TRUE;

	/* output pre- and post-ambles */
	if (STREQ(spec, PREAMBLE)) {
	    (void) scan_string(preamble);
	    prelen = count_lfd(preamble);
	} else if (STREQ(spec, POSTAMBLE)) {
	    (void) scan_string(postamble);
	    postlen = count_lfd(postamble);
	} else if (STREQ(spec, GROUP_SKIP)) {
	    (void) scan_string(group_skip);
	    skiplen = count_lfd(group_skip);
	} else if (STREQ(spec, HEADINGS_FLAG)) {
	    SCAN_NO(&headings_flag);
	} else if (STREQ(spec, HEADING_PRE)) {
	    (void) scan_string(heading_pre);
	    headprelen = count_lfd(heading_pre);
	} else if (STREQ(spec, HEADING_SUF)) {
	    (void) scan_string(heading_suf);
	    headsuflen = count_lfd(heading_suf);
	} else if (STREQ(spec, SYMHEAD_POS)) {
	    (void) scan_string(symhead_pos);
	} else if (STREQ(spec, SYMHEAD_NEG)) {
	    (void) scan_string(symhead_neg);
	} else if (STREQ(spec, NUMHEAD_POS)) {
	    (void) scan_string(numhead_pos);
	} else if (STREQ(spec, NUMHEAD_NEG)) {
	    (void) scan_string(numhead_neg);
	} else if (STREQ(spec, SETPAGEOPEN)) {
	    (void) scan_string(setpage_open);
	    setpagelen = count_lfd(setpage_open);
	} else if (STREQ(spec, SETPAGECLOSE)) {
	    (void) scan_string(setpage_close);
	    setpagelen = count_lfd(setpage_close);
	    /* output index item commands */
	} else if (STREQ(spec, ITEM_0)) {
	    (void) scan_string(item_r[0]);
	    ilen_r[0] = count_lfd(item_r[0]);
	} else if (STREQ(spec, ITEM_1)) {
	    (void) scan_string(item_r[1]);
	    ilen_r[1] = count_lfd(item_r[1]);
	} else if (STREQ(spec, ITEM_2)) {
	    (void) scan_string(item_r[2]);
	    ilen_r[2] = count_lfd(item_r[2]);
	} else if (STREQ(spec, ITEM_01)) {
	    (void) scan_string(item_u[1]);
	    ilen_u[1] = count_lfd(item_u[1]);
	} else if (STREQ(spec, ITEM_12)) {
	    (void) scan_string(item_u[2]);
	    ilen_u[2] = count_lfd(item_u[2]);
	} else if (STREQ(spec, ITEM_x1)) {
	    (void) scan_string(item_x[1]);
	    ilen_x[1] = count_lfd(item_x[1]);
	} else if (STREQ(spec, ITEM_x2)) {
	    (void) scan_string(item_x[2]);
	    ilen_x[2] = count_lfd(item_x[2]);
	    /* output encapsulators */
	} else if (STREQ(spec, ENCAP_0))
	    (void) scan_string(encap_p);
	else if (STREQ(spec, ENCAP_1))
	    (void) scan_string(encap_i);
	else if (STREQ(spec, ENCAP_2))
	    (void) scan_string(encap_s);
	/* output delimiters */
	else if (STREQ(spec, DELIM_0))
	    (void) scan_string(delim_p[0]);
	else if (STREQ(spec, DELIM_1))
	    (void) scan_string(delim_p[1]);
	else if (STREQ(spec, DELIM_2))
	    (void) scan_string(delim_p[2]);
	else if (STREQ(spec, DELIM_N))
	    (void) scan_string(delim_n);
	else if (STREQ(spec, DELIM_R))
	    (void) scan_string(delim_r);
	else if (STREQ(spec, DELIM_T))
	    (void) scan_string(delim_t);
	else if (STREQ(spec, SUFFIX_2P))
	    (void) scan_string(suffix_2p);
	else if (STREQ(spec, SUFFIX_3P))
	    (void) scan_string(suffix_3p);
	else if (STREQ(spec, SUFFIX_MP))
	    (void) scan_string(suffix_mp);
	/* output line width */
	else if (STREQ(spec, LINEMAX)) {
	    SCAN_NO(&tmp);
	    if (tmp > 0)
		linemax = tmp;
	    else
		STY_ERROR2("%s must be positive (got %d)",
			   LINEMAX, tmp);
	    /* output line indentation length */
	} else if (STREQ(spec, INDENT_LENGTH)) {
	    SCAN_NO(&tmp);
	    if (tmp >= 0)
		indent_length = tmp;
	    else
		STY_ERROR2("%s must be nonnegative (got %d)",
			   INDENT_LENGTH, tmp);
	    /* output line indentation */
	} else if (STREQ(spec, INDENT_SPACE)) {
	    (void) scan_string(indent_space);
	    /* composite page delimiter */
	} else if (STREQ(spec, COMPOSITOR)) {
	    (void) scan_string(page_comp);
	    /* page precedence */
	} else if (STREQ(spec, PRECEDENCE)) {
	    (void) scan_string(page_prec);
	    (void) process_precedence();
	    /* index input format */
	} else if (STREQ(spec, KEYWORD))
	    (void) scan_string(idx_keyword);
	else if (STREQ(spec, AOPEN))
	    (void) scan_char(&idx_aopen);
	else if (STREQ(spec, ACLOSE))
	    (void) scan_char(&idx_aclose);
	else if (STREQ(spec, LEVEL))
	    (void) scan_char(&idx_level);
	else if (STREQ(spec, ROPEN))
	    (void) scan_char(&idx_ropen);
	else if (STREQ(spec, RCLOSE))
	    (void) scan_char(&idx_rclose);
	else if (STREQ(spec, QUOTE))
	    (void) scan_char(&idx_quote);
	else if (STREQ(spec, ACTUAL))
	    (void) scan_char(&idx_actual);
	else if (STREQ(spec, ENCAP))
	    (void) scan_char(&idx_encap);
	else if (STREQ(spec, ESCAPE))
	    (void) scan_char(&idx_escape);
	else {
	    (void) next_nonblank();
	    STY_SKIPLINE;
	    STY_ERROR("Unknown specifier %s.\n", spec);
	    put_dot = FALSE;
	}
	if (put_dot) {
	    STY_DOT;
	}
    }

    /* check if quote and escape are distinct */
    if (idx_quote == idx_escape) {
	STY_ERROR(
	       "Quote and escape symbols must be distinct (both `%c' now).\n",
	       idx_quote);
	idx_quote = IDX_QUOTE;
	idx_escape = IDX_ESCAPE;
    }
    DONE(sty_tc - sty_ec, "attributes redefined", sty_ec, "ignored");
    CLOSE(sty_fp);
}

static int
#if STDC
scan_spec(char spec[])
#else
scan_spec(spec)
char    spec[];
#endif
{
    int     i = 0;
    int     c;

    while (TRUE)
	if ((c = next_nonblank()) == -1)
	    return (FALSE);
	else if (c == COMMENT) {
	    STY_SKIPLINE;
	} else
	    break;

    spec[0] = TOLOWER(c);
    while ((i++ < STRING_MAX) && ((c = GET_CHAR(sty_fp)) != SPC) &&
	   (c != TAB) && (c != LFD) && (c != EOF))
	spec[i] = TOLOWER(c);
    if (i < STRING_MAX) {
	spec[i] = NUL;
	if (c == EOF) {
	    STY_ERROR(
		      "No attribute for specifier %s (premature EOF)\n",
		      spec);
	    return (-1);
	}
	if (c == LFD)
	    sty_lc++;
	return (TRUE);
    } else {
	STY_ERROR2("Specifier %s too long (max %d).\n",
		   spec, STRING_MAX);
	return (FALSE);
    }
}


static int
next_nonblank(VOID_ARG)
{
    int     c;

    while (TRUE) {
	switch (c = GET_CHAR(sty_fp)) {
	case EOF:
	    return (-1);

	case LFD:
	    sty_lc++;
	case SPC:
	case TAB:
	    break;
	default:
	    return (c);
	}
    }
#if    IBM_PC_TURBO
    return (-1);		/* not reached, but keeps compiler happy */
#endif
}


static int
#if STDC
scan_string(char str[])
#else
scan_string(str)
char    str[];
#endif
{
    char    clone[ARRAY_MAX];
    int     i = 0;
    int     c;

    switch (c = next_nonblank()) {
    case STR_DELIM:
	while (TRUE)
	    switch (c = GET_CHAR(sty_fp)) {
	    case EOF:
		STY_ERROR("No closing delimiter in %s.\n",
			  clone);
		return (FALSE);
	    case STR_DELIM:
		clone[i] = NUL;
		strcpy(str, clone);
		return (TRUE);
	    case BSH:
		switch (c = GET_CHAR(sty_fp)) {
		case 't':
		    clone[i++] = TAB;
		    break;
		case 'n':
		    clone[i++] = LFD;
		    break;

		default:
		    clone[i++] = (char) c;
		    break;
		}
		break;
	    default:
		if (c == LFD)
		    sty_lc++;
		if (i < ARRAY_MAX)
		    clone[i++] = (char) c;
		else {
		    STY_SKIPLINE;
		    STY_ERROR2(
			       "Attribute string %s too long (max %d).\n",
			       clone, ARRAY_MAX);
		    return (FALSE);
		}
		break;
	    }
	break;
    case COMMENT:
	STY_SKIPLINE;
	break;
    default:
	STY_SKIPLINE;
	STY_ERROR("No opening delimiter.\n", "");
	return (FALSE);
    }
    return (TRUE);                     /* function value no longer used */
}


static int
#if STDC
scan_char(char *c)
#else
scan_char(c)
char   *c;
#endif
{
    int     clone;

    switch (clone = next_nonblank()) {
    case CHR_DELIM:
	switch (clone = GET_CHAR(sty_fp)) {
	case CHR_DELIM:
	    STY_SKIPLINE;
	    STY_ERROR("Premature closing delimiter.\n", "");
	    return (FALSE);
	case LFD:
	    sty_lc++;
	case EOF:
	    STY_ERROR("No character (premature EOF).\n", "");
	    return (FALSE);
	case BSH:
	    clone = GET_CHAR(sty_fp);
	default:
	    if (GET_CHAR(sty_fp) == CHR_DELIM) {
		*c = (char) clone;
		return (TRUE);
	    } else {
		STY_ERROR("No closing delimiter or too many letters.\n", "");
		return (FALSE);
	    }
	}
	/* break; */				/* NOT REACHED */
    case COMMENT:
	STY_SKIPLINE;
	break;
    default:
	STY_SKIPLINE;
	STY_ERROR("No opening delimiter.\n", "");
	return (FALSE);
    }
    return (TRUE);                     /* function value no longer used */
}


static int
#if STDC
count_lfd(char *str)
#else
count_lfd(str)
char   *str;
#endif
{
    int     i = 0;
    int     n = 0;

    while (str[i] != NUL) {
	if (str[i] == LFD)
	    n++;
	i++;
    }
    return (n);
}


static int
process_precedence(VOID_ARG)
{
    int     order[PAGETYPE_MAX];
    int     type[PAGETYPE_MAX];
    int     i = 0;
    int     last;
    int     roml = FALSE;
    int     romu = FALSE;
    int     arab = FALSE;
    int     alpl = FALSE;
    int     alpu = FALSE;

    /* check for llegal specifiers first */
    while ((i < PAGETYPE_MAX) && (page_prec[i] != NUL)) {
	switch (page_prec[i]) {
	case ROMAN_LOWER:
	    if (roml) {
		MULTIPLE(ROMAN_LOWER);
	    } else
		roml = TRUE;
	    break;
	case ROMAN_UPPER:
	    if (romu) {
		MULTIPLE(ROMAN_UPPER);
	    } else
		romu = TRUE;
	    break;
	case ARABIC:
	    if (arab) {
		MULTIPLE(ARABIC);
	    } else
		arab = TRUE;
	    break;
	case ALPHA_LOWER:
	    if (alpl) {
		MULTIPLE(ALPHA_UPPER);
	    } else
		alpl = TRUE;
	    break;
	case ALPHA_UPPER:
	    if (alpu) {
		MULTIPLE(ALPHA_UPPER);
	    } else
		alpu = TRUE;
	    break;
	default:
	    STY_SKIPLINE;
	    STY_ERROR("Unknow type `%c' in page precedence specification.\n",
		      page_prec[i]);
	    return (FALSE);
	}
	i++;
    }
    if (page_prec[i] != NUL) {
	STY_SKIPLINE;
	STY_ERROR("Page precedence specification string too long.\n", "");
	return (FALSE);
    }
    last = i;
    switch (page_prec[0]) {
    case ROMAN_LOWER:
	order[0] = ROMAN_LOWER_OFFSET;
	type[0] = ROML;
	break;
    case ROMAN_UPPER:
	order[0] = ROMAN_UPPER_OFFSET;
	type[0] = ROMU;
	break;
    case ARABIC:
	order[0] = ARABIC_OFFSET;
	type[0] = ARAB;
	break;
    case ALPHA_LOWER:
	order[0] = ALPHA_LOWER_OFFSET;
	type[0] = ALPL;
	break;
    case ALPHA_UPPER:
	order[0] = ALPHA_LOWER_OFFSET;
	type[0] = ALPU;
	break;
    }

    i = 1;
    while (i < last) {
	switch (page_prec[i]) {
	case ROMAN_LOWER:
	    order[i] = order[i - 1] + ROMAN_LOWER_OFFSET;
	    type[i] = ROML;
	    break;
	case ROMAN_UPPER:
	    order[i] = order[i - 1] + ROMAN_UPPER_OFFSET;
	    type[i] = ROMU;
	    break;
	case ARABIC:
	    order[i] = order[i - 1] + ARABIC_OFFSET;
	    type[i] = ARAB;
	    break;
	case ALPHA_LOWER:
	    order[i] = order[i - 1] + ALPHA_LOWER_OFFSET;
	    type[i] = ALPL;
	    break;
	case ALPHA_UPPER:
	    order[i] = order[i - 1] + ALPHA_LOWER_OFFSET;
	    type[i] = ALPU;
	    break;
	}
	i++;
    }

    for (i = 0; i < PAGETYPE_MAX; i++) {
	page_offset[i] = -1;
    }
    page_offset[type[0]] = 0;
    for (i = 1; i < last; i++) {
	page_offset[type[i]] = order[i - 1];
    }
    for (i = 0; i < PAGETYPE_MAX; i++) {
	if (page_offset[i] == -1) {
	    switch (type[last - 1]) {
	    case ROML:
		order[last] = order[last - 1] + ROMAN_LOWER_OFFSET;
		break;
	    case ROMU:
		order[last] = order[last - 1] + ROMAN_UPPER_OFFSET;
		break;
	    case ARAB:
		order[last] = order[last - 1] + ARABIC_OFFSET;
		break;
	    case ALPL:
		order[last] = order[last - 1] + ALPHA_LOWER_OFFSET;
		break;
	    case ALPU:
		order[last] = order[last - 1] + ALPHA_UPPER_OFFSET;
		break;
	    }
	    type[last] = i;
	    page_offset[i] = order[last];
	    last++;
	}
    }
    return (TRUE);                     /* function value no longer used */
}
