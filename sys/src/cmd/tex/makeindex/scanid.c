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
#include    "scanid.h"

#define CHECK_LENGTH()	if (i > len_field) goto OVERFLOW

int     idx_lc;			       /* line count */
int     idx_tc;			       /* total entry count */
int     idx_ec;			       /* erroneous entry count */
int     idx_dc;			       /* number of dots printed so far */

static int first_entry = TRUE;
static int comp_len;
static char key[ARGUMENT_MAX];
static char no[NUMBER_MAX];
extern char *strchr ARGS((const char* s,int c));

NODE_PTR head;
NODE_PTR tail;

static	void	flush_to_eol ARGS((void));
static	int	make_key ARGS((void));
static	void	make_string ARGS((char **ppstr,int n));
static	int	scan_alpha_lower ARGS((char *no,short *npg,short *count));
static	int	scan_alpha_upper ARGS((char *no,short *npg,short *count));
static	int	scan_arabic ARGS((char *no,short *npg,short *count));
static	int	scan_arg1 ARGS((void));
static	int	scan_arg2 ARGS((void));
static	int	scan_field ARGS((int *n,char field[],int len_field,
				 int ck_level, int ck_encap,int ck_actual));
static	int	scan_key ARGS((struct KFIELD *data));
static	int	scan_no ARGS((char *no,short *npg,short *count,short *type));
static	int	scan_roman_lower ARGS((char *no,short *npg,short *count));
static	int	scan_roman_upper ARGS((char *no,short *npg,short *count));
static	void	search_quote ARGS((char sort_key[],char actual_key[]));

#if     (OS_BS2000 | OS_MVSXA)
char    UPCC[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#endif

void
scan_idx(VOID_ARG)
{
    char    keyword[ARRAY_MAX];
    int     c;
    int     i = 0;
    int     not_eof = TRUE;
    int     arg_count = -1;

    MESSAGE("Scanning input file %s...", idx_fn);
    idx_lc = idx_tc = idx_ec = idx_dc = 0;
    comp_len = strlen(page_comp);
    while (not_eof) {
	switch (c = GET_CHAR(idx_fp)) {
	case EOF:
	    if (arg_count == 2) {
		idx_lc++;
		if (make_key())
		    IDX_DOT(DOT_MAX);
		arg_count = -1;
	    } else
		not_eof = FALSE;
	    break;

	case LFD:
	    idx_lc++;
	    if (arg_count == 2) {
		if (make_key())
		    IDX_DOT(DOT_MAX);
		arg_count = -1;
	    } else if (arg_count > -1) {
		IDX_ERROR("Missing arguments -- need two (premature LFD).\n",
			  NULL);
		arg_count = -1;
	    }
	case TAB:
	case SPC:
	    break;

	default:
	    switch (arg_count) {
	    case -1:
		i = 0;
		keyword[i++] = (char) c;
		arg_count++;
		idx_tc++;
		break;
	    case 0:
		if (c == idx_aopen) {
		    arg_count++;
		    keyword[i] = NUL;
		    if (STREQ(keyword, idx_keyword)) {
			if (!scan_arg1()) {
			    arg_count = -1;
			}
		    } else {
			IDX_SKIPLINE;
			IDX_ERROR("Unknown index keyword %s.\n", keyword);
		    }
		} else {
		    if (i < ARRAY_MAX)
			keyword[i++] = (char) c;
		    else {
			IDX_SKIPLINE;
			IDX_ERROR2("Index keyword %s too long (max %d).\n",
				   keyword, ARRAY_MAX);
		    }
		}
		break;
	    case 1:
		if (c == idx_aopen) {
		    arg_count++;
		    if (!scan_arg2()) {
			arg_count = -1;
		    }
		} else {
		    IDX_SKIPLINE;
IDX_ERROR(
"No opening delimiter for second argument (illegal character `%c').\n", c);
		}
		break;
	    case 2:
		IDX_SKIPLINE;
IDX_ERROR(
"No closing delimiter for second argument (illegal character `%c').\n", c);
		break;
	    }
	    break;
	}
    }

    /* fixup the total counts */
    idx_tt += idx_tc;
    idx_et += idx_ec;

    DONE(idx_tc - idx_ec, "entries accepted", idx_ec, "rejected");
    CLOSE(idx_fp);
}

static void
flush_to_eol(VOID_ARG)
{	/* flush to end-of-line, or end-of-file, whichever is first */
    int a;

    while ( ((a = GET_CHAR(idx_fp)) != LFD) && (a != EOF) )
    	/* NO-OP */;
}

static int
make_key(VOID_ARG)
{
    NODE_PTR ptr;
    int     i;

    /* allocate and initialize a node */

#ifdef DEBUG
	totmem += sizeof(NODE);
	(void)fprintf(stderr,"make_key(): malloc(%d)\ttotmem = %ld\n",
	    sizeof(NODE),totmem);
#endif /* DEBUG */

    if ((ptr = (NODE_PTR) malloc(sizeof(NODE))) == NULL)
	FATAL("Not enough core...abort.\n", "");

    for (i = 0; i < FIELD_MAX; i++)
    {
	ptr->data.sf[i] = "";		/* initialize fields to pointers */;
	ptr->data.af[i] = "";		/* to constant empty strings */
    }
    ptr->data.encap = "";
    ptr->data.lpg[0] = NUL;
    ptr->data.count = 0;
    ptr->data.type = EMPTY;

    /* process index key */
    if (!scan_key(&(ptr->data)))
	return (FALSE);

    /* determine group type */
    ptr->data.group = group_type(ptr->data.sf[0]);

    /* process page number */
    strcpy(ptr->data.lpg, no);
    if (!scan_no(no, ptr->data.npg, &(ptr->data.count), &(ptr->data.type)))
	return (FALSE);

    if (first_entry) {
	head = tail = ptr;
	first_entry = FALSE;
    } else {
	tail->next = ptr;
	tail = ptr;
    }
    ptr->data.lc = idx_lc;
    ptr->data.fn = idx_fn;
    tail->next = NULL;

    return (TRUE);
}

static void
#if STDC
make_string(char **ppstr, int n)
#else
make_string(ppstr,n)			/* allocate n-byte string if *ppstr */
char	**ppstr;			/* points to an empty string */
int	n;
#endif
{
    if ((*ppstr)[0] == NUL)		/* then we have an empty string */
    {
	(*ppstr) = (char*)malloc(n);
	if ((*ppstr) == (char*)NULL)
	    FATAL("Not enough core...abort.\n", "");
	(*ppstr)[0] = NUL;
    }
}

static int
#if STDC
scan_key(FIELD_PTR data)
#else
scan_key(data)
FIELD_PTR data;
#endif
{
    int     i = 0;		       /* current level */
    int     n = 0;		       /* index to the key[] array */
    int     second_round = FALSE;
    int     last = FIELD_MAX - 1;

    while (TRUE) {
	if (key[n] == NUL)
	    break;
	if (key[n] == idx_encap)
	{
	    n++;
	    make_string(&(data->encap),strlen(key) + 1);
	    if (scan_field(&n, data->encap, strlen(key), FALSE, FALSE, FALSE))
		break;
	    else
		return (FALSE);
	}
	if (key[n] == idx_actual) {
	    n++;
	    if (i == last)
	    {
		make_string(&(data->af[i]),strlen(key) + 1);
		if (!scan_field(&n, data->af[i], strlen(key),
				FALSE, TRUE, FALSE))
		    return (FALSE);
	    }
	    else
	    {
		make_string(&(data->af[i]),strlen(key) + 1);
		if (!scan_field(&n, data->af[i], strlen(key),
				TRUE, TRUE, FALSE))
		    return (FALSE);
	    }
	} else {
	    /* Next nesting level */
	    if (second_round) {
		i++;
		n++;
	    }
	    if (i == last)
	    {
		make_string(&(data->sf[i]),strlen(key) + 1);
		if (!scan_field(&n, data->sf[i], strlen(key),
				FALSE, TRUE, TRUE))
		    return (FALSE);
	    }
	    else
	    {
		make_string(&(data->sf[i]),strlen(key) + 1);
		if (!scan_field(&n, data->sf[i], strlen(key),
				TRUE, TRUE, TRUE))
		    return (FALSE);
	    }
	    second_round = TRUE;
	    if (german_sort && strchr(data->sf[i], '"'))
	    {
		make_string(&(data->af[i]),strlen(data->sf[i]) + 1);
		search_quote(data->sf[i], data->af[i]);
	    }
	}
    }

    /* check for empty fields which shouldn't be empty */
    if (*data->sf[0] == NUL) {
	NULL_RTN;
    }
    for (i = 1; i < FIELD_MAX - 1; i++)
	if ((*data->sf[i] == NUL) &&
	    ((*data->af[i] != NUL) || (*data->sf[i + 1] != NUL))) {
	    NULL_RTN;
	}
    /* i == FIELD_MAX-1 */
    if ((*data->sf[i] == NUL) && (*data->af[i] != NUL)) {
	NULL_RTN;
    }
    return (TRUE);
}

static int
#if STDC
scan_field(int *n, char field[], int len_field, int ck_level, int ck_encap, 
	int ck_actual)
#else
scan_field(n, field, len_field, ck_level, ck_encap, ck_actual)
int    *n;
char    field[];
int	len_field;	/* length of field[], EXCLUDING space for final NUL */
int     ck_level;
int     ck_encap;
int     ck_actual;
#endif
{
    int     i = 0;
    int     nbsh;		       /* backslash count */

    if (compress_blanks && ((key[*n] == SPC) || (key[*n] == TAB)))
	++* n;

    while (TRUE) {
	nbsh = 0;
	while (key[*n] == idx_escape)
	{
	    nbsh++;
	    field[i++] = key[*n];
	    CHECK_LENGTH();
	    ++*n;
	}

	if (key[*n] == idx_quote)
	{
	    if (nbsh % 2 == 0)
		field[i++] = key[++*n];
	    else
		field[i++] = key[*n];
	    CHECK_LENGTH();
	}
	else if ((ck_level && (key[*n] == idx_level)) ||
		 (ck_encap && (key[*n] == idx_encap)) ||
		 (ck_actual && (key[*n] == idx_actual)) ||
		 (key[*n] == NUL))
	{
	    if ((i > 0) && compress_blanks && (field[i - 1] == SPC))
		field[i - 1] = NUL;
	    else
		field[i] = NUL;
	    return (TRUE);
	} else {
	    field[i++] = key[*n];
	    CHECK_LENGTH();
	    if ((!ck_level) && (key[*n] == idx_level)) {
		IDX_ERROR2("Extra `%c' at position %d of first argument.\n",
			   idx_level, *n + 1);
		return (FALSE);
	    } else if ((!ck_encap) && (key[*n] == idx_encap)) {
		IDX_ERROR2("Extra `%c' at position %d of first argument.\n",
			   idx_encap, *n + 1);
		return (FALSE);
	    } else if ((!ck_actual) && (key[*n] == idx_actual)) {
		IDX_ERROR2("Extra `%c' at position %d of first argument.\n",
			   idx_actual, *n + 1);
		return (FALSE);
	    }
	}
	/* check if max field length is reached */
	if (i > len_field)
	{
	OVERFLOW:
	    if (!ck_encap) {
		IDX_ERROR("Encapsulator of page number too long (max. %d).\n",
			  len_field);
	    } else if (ck_actual) {
		IDX_ERROR("Index sort key too long (max. %d).\n", len_field);
	    } else {
		IDX_ERROR("Text of key entry too long (max. %d).\n", len_field);
	    }
	    return (FALSE);
	}
	++*n;
    }
#if    IBM_PC_TURBO
    return (FALSE);		/* not reached, but keeps compiler happy */
#endif
}

int
#if STDC
group_type(char *str)
#else
group_type(str)
char   *str;
#endif
{
    int     i = 0;

    while ((str[i] != NUL) && ISDIGIT(str[i]))
	i++;

    if (str[i] == NUL) {
	sscanf(str, "%d", &i);
	return (i);
    } else if (ISSYMBOL(str[0]))
	return (SYMBOL);
    else
	return (ALPHA);
}

static int
#if STDC
scan_no(char no[], short npg[], short *count, short *type)
#else
scan_no(no, npg, count, type)
char    no[];
short   npg[];
short  *count;
short  *type;
#endif
{
    int     i = 1;

    if (isdigit(no[0])) {
	*type = ARAB;
	if (!scan_arabic(no, npg, count))
	    return (FALSE);
	/* simple heuristic to determine if a letter is Roman or Alpha */
    } else if (IS_ROMAN_LOWER(no[0]) && (!IS_COMPOSITOR)) {
	*type = ROML;
	if (!scan_roman_lower(no, npg, count))
	    return (FALSE);
	/* simple heuristic to determine if a letter is Roman or Alpha */
    } else if (IS_ROMAN_UPPER(no[0]) &&
	       ((no[0] == ROMAN_I) || (!IS_COMPOSITOR))) {
	*type = ROMU;
	if (!scan_roman_upper(no, npg, count))
	    return (FALSE);
    } else if (IS_ALPHA_LOWER(no[0])) {
	*type = ALPL;
	if (!scan_alpha_lower(no, npg, count))
	    return (FALSE);
    } else if (IS_ALPHA_UPPER(no[0])) {
	*type = ALPU;
	if (!scan_alpha_upper(no, npg, count))
	    return (FALSE);
    } else {
	IDX_ERROR("Illegal page number %s.\n", no);
	return (FALSE);
    }
    return (TRUE);
}


static int
#if STDC
scan_arabic(char no[], short npg[], short *count)
#else
scan_arabic(no, npg, count)
char    no[];
short   npg[];
short  *count;
#endif
{
    short   i = 0;
    char    str[ARABIC_MAX+1];		/* space for trailing NUL */

    while ((no[i] != NUL) && (i <= ARABIC_MAX) && (!IS_COMPOSITOR)) {
	if (isdigit(no[i])) {
	    str[i] = no[i];
	    i++;
	} else {
IDX_ERROR2("Illegal Arabic digit: position %d in %s.\n", i + 1, no);
	    return (FALSE);
	}
    }
    if (i > ARABIC_MAX) {
	IDX_ERROR2("Arabic page number %s too big (max %d digits).\n",
		   no, ARABIC_MAX);
	return (FALSE);
    }
    str[i] = NUL;

    ENTER(strtoint(str) + page_offset[ARAB]);

    if (IS_COMPOSITOR)
	return (scan_no(&no[i + comp_len], npg, count, &i));
    else
	return (TRUE);
}


static int
#if STDC
scan_roman_lower(char no[], short npg[], short *count)
#else
scan_roman_lower(no, npg, count)
char    no[];
short   npg[];
short  *count;
#endif
{
    short   i = 0;
    int     inp = 0;
    int     prev = 0;
    int     the_new;

    while ((no[i] != NUL) && (i < ROMAN_MAX) && (!IS_COMPOSITOR)) {
	if ((IS_ROMAN_LOWER(no[i])) &&
	    ((the_new = ROMAN_LOWER_VAL(no[i])) != 0)) {
	    if (prev == 0)
		prev = the_new;
	    else {
		if (prev < the_new) {
		    prev = the_new - prev;
		    the_new = 0;
		}
		inp += prev;
		prev = the_new;
	    }
	} else {
IDX_ERROR2("Illegal Roman number: position %d in %s.\n", i + 1, no);
	    return (FALSE);
	}
	i++;
    }
    if (i == ROMAN_MAX) {
	IDX_ERROR2("Roman page number %s too big (max %d digits).\n",
		   no, ROMAN_MAX);
	return (FALSE);
    }
    inp += prev;

    ENTER(inp + page_offset[ROML]);

    if (IS_COMPOSITOR)
	return (scan_no(&no[i + comp_len], npg, count, &i));
    else
	return (TRUE);
}


static int
#if STDC
scan_roman_upper(char no[], short npg[], short *count)
#else
scan_roman_upper(no, npg, count)
char    no[];
short   npg[];
short  *count;
#endif
{
    short   i = 0;
    int     inp = 0;
    int     prev = 0;
    int     the_new;

    while ((no[i] != NUL) && (i < ROMAN_MAX) && (!IS_COMPOSITOR)) {
	if ((IS_ROMAN_UPPER(no[i])) &&
	    ((the_new = ROMAN_UPPER_VAL(no[i])) != 0)) {
	    if (prev == 0)
		prev = the_new;
	    else {
		if (prev < the_new) {
		    prev = the_new - prev;
		    the_new = 0;
		}
		inp += prev;
		prev = the_new;
	    }
	} else {
IDX_ERROR2("Illegal Roman number: position %d in %s.\n", i + 1, no);
	    return (FALSE);
	}
	i++;
    }
    if (i == ROMAN_MAX) {
	IDX_ERROR2("Roman page number %s too big (max %d digits).\n",
		   no, ROMAN_MAX);
	return (FALSE);
    }
    inp += prev;

    ENTER(inp + page_offset[ROMU]);

    if (IS_COMPOSITOR)
	return (scan_no(&no[i + comp_len], npg, count, &i));
    else
	return (TRUE);
}


static int
#if STDC
scan_alpha_lower(char no[], short npg[], short *count)
#else
scan_alpha_lower(no, npg, count)
char    no[];
short   npg[];
short  *count;
#endif
{
    short   i;

    ENTER(ALPHA_VAL(no[0]) + page_offset[ALPL]);

    i = 1;
    if (IS_COMPOSITOR)
	return (scan_no(&no[comp_len + 1], npg, count, &i));
    else
	return (TRUE);
}


static int
#if STDC
scan_alpha_upper(char no[], short npg[], short *count)
#else
scan_alpha_upper(no, npg, count)
char    no[];
short   npg[];
short  *count;
#endif
{
    short   i;

    ENTER(ALPHA_VAL(no[0]) + page_offset[ALPU]);

    i = 1;
    if (IS_COMPOSITOR)
	return (scan_no(&no[comp_len + 1], npg, count, &i));
    else
	return (TRUE);
}


static int
scan_arg1(VOID_ARG)
{
    int     i = 0;
    int     n = 0;		       /* delimiter count */
    int     a;

    if (compress_blanks)
	while (((a = GET_CHAR(idx_fp)) == SPC) || (a == TAB));
    else
	a = GET_CHAR(idx_fp);

    while ((i < ARGUMENT_MAX) && (a != EOF))
    {
	if ((a == idx_quote) || (a == idx_escape))
	{				/* take next character literally */
	    key[i++] = (char) a;	/* but preserve quote or escape */
	    a = GET_CHAR(idx_fp);
	    key[i++] = (char) a;	/* save literal character */
	}
	else if (a == idx_aopen)
	{		/* opening delimiters within the argument list */
	    key[i++] = (char) a;
	    n++;
	}
	else if (a == idx_aclose)
	{
	    if (n == 0)			/* end of argument */
	    {
		if (compress_blanks && key[i - 1] == SPC)
		    key[i - 1] = NUL;
		else
		    key[i] = NUL;
		return (TRUE);
	    }
	    else			/* nested delimiters */
	    {
		key[i++] = (char) a;
		n--;
	    }
	}
	else
	{
	    switch (a)
	    {
	    case LFD:
		idx_lc++;
		IDX_ERROR("Incomplete first argument (premature LFD).\n", "");
		return (FALSE);
	    case TAB:
	    case SPC:
		/* compress successive SPC's to one SPC */
		if (compress_blanks)
		{
		    if ((i > 0) && (key[i - 1] != SPC) && (key[i - 1] != TAB))
			key[i++] = SPC;
		    break;
		}
	    default:
		key[i++] = (char) a;
		break;
	    }
	}
	a = GET_CHAR(idx_fp);
    }

    flush_to_eol();			/* Skip to end of line */
    idx_lc++;
    IDX_ERROR("First argument too long (max %d).\n", ARGUMENT_MAX);
    return (FALSE);
}


static int
scan_arg2(VOID_ARG)
{
    int     i = 0;
    int     a;
    int     hit_blank = FALSE;

    while (((a = GET_CHAR(idx_fp)) == SPC) || (a == TAB));

    while (i < NUMBER_MAX) {
	if (a == idx_aclose) {
	    no[i] = NUL;
	    return (TRUE);
	} else
	    switch (a) {
	    case LFD:
		idx_lc++;
IDX_ERROR("Incomplete second argument (premature LFD).\n", "");
		return (FALSE);
	    case TAB:
	    case SPC:
		hit_blank = TRUE;
		break;
	    default:
		if (hit_blank) {
		    flush_to_eol();	/* Skip to end of line */
		    idx_lc++;
IDX_ERROR("Illegal space within numerals in second argument.\n", "");
		    return (FALSE);
		}
		no[i++] = (char) a;
		break;
	    }
	a = GET_CHAR(idx_fp);
    }
    flush_to_eol();	/* Skip to end of line */
    idx_lc++;
    IDX_ERROR("Second argument too long (max %d).\n", NUMBER_MAX);
    return (FALSE);
}


static void
#if STDC
search_quote(char sort_key[], char actual_key[])
#else
search_quote(sort_key, actual_key)
char    sort_key[];
char    actual_key[];
#endif
{
    char   *ptr;		       /* pointer to sort_key */
    char   *sort;		       /* contains sorting text */
    int     char_found = FALSE;

    strcpy(actual_key, sort_key);
    ptr = strchr(sort_key, '"');       /* look for first double quote */
    while (ptr != (char*)NULL)
    {
	sort = "";
	switch (*(ptr + 1))
	{				/* skip to umlaut or sharp S */
	case 'a':
	case 'A':
	    sort = isupper(*(ptr + 1)) ? "Ae" : "ae";
	    break;
	case 'o':
	case 'O':
	    sort = isupper(*(ptr + 1)) ? "Oe" : "oe";
	    break;
	case 'u':
	case 'U':
	    sort = isupper(*(ptr + 1)) ? "Ue" : "ue";
	    break;
	case 's':
	    sort = "ss";
	    break;
	default:
	    break;
	}
	if (sort[0] != NUL)
	{
	    char_found = TRUE;
	    *ptr = sort[0];	       /* write to sort_key */
	    *(ptr + 1) = sort[1];
	}
	ptr = strchr(ptr + 1, '"');    /* next double quote */
    }
    if (!char_found)		       /* reset actual_key ? */
	actual_key[0] = NUL;
    return;
}
