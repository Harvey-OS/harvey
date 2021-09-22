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

#define MKIND_C	1
#include    "mkind.h"
#undef MKIND_C

int     letter_ordering = FALSE;
int     compress_blanks = FALSE;
int     merge_page = TRUE;
int     init_page = FALSE;
int     even_odd = -1;
int     verbose = TRUE;
int     german_sort = FALSE;
int     fn_no = -1;		       /* total number of files */
int     idx_dot = TRUE;		       /* flag which shows dot in ilg being
					* active */
int     idx_tt = 0;		       /* total entry count (all files) */
int     idx_et = 0;		       /* erroneous entry count (all files) */
int     idx_gt = 0;		       /* good entry count (all files) */

FIELD_PTR *idx_key;
FILE   *log_fp;
FILE   *sty_fp;
FILE   *idx_fp;
FILE   *ind_fp;
FILE   *ilg_fp;

#if OS_BS2000
FILE   *widx_fp;
char    widx_fn[STRING_MAX + 5];
#endif				       /* OS_BS2000 */

char   *pgm_fn;
char    sty_fn[LINE_MAX];
char   *idx_fn;
char    ind[STRING_MAX];
char   *ind_fn;
char    ilg[STRING_MAX];
char   *ilg_fn;
char    pageno[NUMBER_MAX];

static char log_fn[STRING_MAX];
static char base[STRING_MAX];
static int need_version = TRUE;

static	void	check_all ARGS((char *fn,int ind_given,int ilg_given,
			int log_given));
static	void	check_idx ARGS((char *fn,int open_fn));
static	void	find_pageno ARGS((void));
static	void	open_sty ARGS((char *fn));
static	void	prepare_idx ARGS((void));
static	void	process_idx ARGS((char * *fn,int use_stdin,int sty_given,
			int ind_given,int ilg_given,int log_given));

#if    OS_PCDOS
#if    IBM_PC_TURBO
unsigned	_stklen = 0xf000;	/* Turbo C ignores size set in
					.exe file by LINK or EXEMOD,
					sigh... */
#endif					/* IBM_PC_TURBO */
#endif					/* OS_PCDOS */

#if OS_ATARI
long    _stksize = 20000L;	       /* make the stack larger than 2KB */
/* size must  be given */
#endif				       /* OS_ATARI */

#ifdef DEBUG
long totmem = 0L;			/* for debugging memory usage */
#endif /* DEBUG */

int
#if STDC
main(int argc, char *argv[])
#else
main(argc, argv)
int     argc;
char   *argv[];
#endif
{
    char   *fns[ARRAY_MAX];
    char   *ap;
    int     use_stdin = FALSE;
    int     sty_given = FALSE;
    int     ind_given = FALSE;
    int     ilg_given = FALSE;
    int     log_given = FALSE;

    /* determine program name */
#if (OS_ATARI | OS_VAXVMS | OS_BS2000 | OS_MVSXA | OS_VMCMS)
    pgm_fn = "MakeIndex";	       /* Use symbolic name on some systems */
#else
    pgm_fn = strrchr(*argv, DIR_DELIM);
    if (pgm_fn == NULL)
	pgm_fn = *argv;
    else
	pgm_fn++;

#if OS_PCDOS
    {
	register char *ext = pgm_fn + strlen(pgm_fn)-4;
	if ( *ext == '.' )
	    *ext = NUL;			/* cut off ".EXE" */
    }
    (void)strlwr(pgm_fn);		/* lower program name    */
#endif					/* OS_PCDOS */

#endif				       /* OS_VAXVMS | OS_BS2000 | OS_MVSXA */

    /* process command line options */
    while (--argc > 0) {
	if (**++argv == SW_PREFIX) {
	    if (*(*argv + 1) == NUL)
		break;
	    for (ap = ++*argv; *ap != NUL; ap++)
#if  (OS_BS2000 | OS_MVSXA | OS_VAXVMS)
		switch (tolower(*ap)) {
#else
		switch (*ap) {
#endif /* (OS_BS2000 | OS_MVSXA | OS_VAXVMS) */

		    /* use standard input */
		case 'i':
		    use_stdin = TRUE;
		    break;

		    /* enable letter ordering */
		case 'l':
		    letter_ordering = TRUE;
		    break;

		    /* disable range merge */
		case 'r':
		    merge_page = FALSE;
		    break;

		    /* supress progress message -- quiet mode */
		case 'q':
		    verbose = FALSE;
		    break;

		    /* compress blanks */
		case 'c':
		    compress_blanks = TRUE;
		    break;

		    /* style file */
		case 's':
		    argc--;
		    if (argc <= 0)
			FATAL("Expected -s <stylefile>\n","");
		    open_sty(*++argv);
		    sty_given = TRUE;
		    break;

		    /* output index file name */
		case 'o':
		    argc--;
		    if (argc <= 0)
			FATAL("Expected -o <ind>\n","");
		    ind_fn = *++argv;
		    ind_given = TRUE;
		    break;

		    /* transcript file name */
		case 't':
		    argc--;
		    if (argc <= 0)
			FATAL("Expected -t <logfile>\n","");
		    ilg_fn = *++argv;
		    ilg_given = TRUE;
		    break;

		    /* initial page */
		case 'p':
		    argc--;
		    if (argc <= 0)
			FATAL("Expected -p <num>\n","");
		    strcpy(pageno, *++argv);
		    init_page = TRUE;
		    if (STREQ(pageno, EVEN)) {
			log_given = TRUE;
			even_odd = 2;
		    } else if (STREQ(pageno, ODD)) {
			log_given = TRUE;
			even_odd = 1;
		    } else if (STREQ(pageno, ANY)) {
			log_given = TRUE;
			even_odd = 0;
		    }
		    break;

		    /* enable german sort */
		case 'g':
		    german_sort = TRUE;
		    break;

		    /* bad option */
		default:
		    FATAL("Unknown option -%c.\n", *ap);
		    break;
		}
	} else {
	    if (fn_no < ARRAY_MAX) {
		check_idx(*argv, FALSE);
		fns[++fn_no] = *argv;
	    } else {
		FATAL("Too many input files (max %d).\n", ARRAY_MAX);
	    }
	}
    }

/* changes for 2.12 (May 20, 1993) by Julian Reschke (jr@ms.maus.de):
   if only one input file and no explicit style file was given */
#if OS_BS2000
/* not supported */
#else
	if (fn_no == 0 && !sty_given)
	{
		char tmp[STRING_MAX + 5];
		
		/* base set by last call to check_idx */
		sprintf (tmp, "%s%s", base, INDEX_STY);
		if (0 == access(tmp, R_OK)) {
			open_sty (tmp);
			sty_given = TRUE;
		}
	}
#endif
 
    process_idx(fns, use_stdin, sty_given, ind_given, ilg_given, log_given);
    idx_gt = idx_tt - idx_et;
    ALL_DONE;
    if (idx_gt > 0) {
	prepare_idx();
	sort_idx();
	gen_ind();
	MESSAGE("Output written in %s.\n", ind_fn);
    } else
	MESSAGE("Nothing written in %s.\n", ind_fn);

    MESSAGE("Transcript written in %s.\n", ilg_fn);
    CLOSE(ind_fp);
    CLOSE(ilg_fp);
    EXIT(0);

    return (0);			       /* never executed--avoids complaints */
    /* about no return value */
}


static void
prepare_idx(VOID_ARG)
{
    NODE_PTR ptr = head;
    int     i = 0;

#ifdef DEBUG
    totmem += idx_gt * sizeof(FIELD_PTR);
    (void)fprintf(stderr,"prepare_idx(): calloc(%d,%d)\ttotmem = %ld\n",
	idx_gt,sizeof(FIELD_PTR),totmem);
#endif /* DEBUG */

    if (head == (NODE_PTR)NULL)
	FATAL("No valid index entries collected.\n", "");

    if ((idx_key = (FIELD_PTR *) calloc(idx_gt, sizeof(FIELD_PTR))) == NULL) {
	FATAL("Not enough core...abort.\n", "");
    }
    for (i = 0; i < idx_gt; i++) {
	idx_key[i] = &(ptr->data);
	ptr = ptr->next;
    }
}


static void
#if STDC
process_idx(char *fn[], int use_stdin, int sty_given, int ind_given,
	    int ilg_given, int log_given)
#else
process_idx(fn, use_stdin, sty_given, ind_given, ilg_given, log_given)
char   *fn[];
int     use_stdin;
int     sty_given;
int     ind_given;
int     ilg_given;
int     log_given;
#endif
{
    int     i;

    if (fn_no == -1)
	/* use stdin if no input files specified */
	use_stdin = TRUE;
    else {
	check_all(fn[0], ind_given, ilg_given, log_given);
	PUT_VERSION;
	if (sty_given)
	    scan_sty();
	if (german_sort && (idx_quote == '"'))
FATAL("Option -g invalid, quote character must be different from '%c'.\n",
'"');
	scan_idx();
	ind_given = TRUE;
	ilg_given = TRUE;
	for (i = 1; i <= fn_no; i++) {
	    check_idx(fn[i], TRUE);
	    scan_idx();
	}
    }

    if (use_stdin) {
	idx_fn = "stdin";
	idx_fp = stdin;

	if (ind_given) {
	    if (!ind_fp && ((ind_fp = OPEN_OUT(ind_fn)) == NULL))
		FATAL("Can't create output index file %s.\n", ind_fn);
	} else {
	    ind_fn = "stdout";
	    ind_fp = stdout;
	}

	if (ilg_given) {
	    if (!ilg_fp && ((ilg_fp = OPEN_OUT(ilg_fn)) == NULL))
		FATAL("Can't create transcript file %s.\n", ilg_fn);
	} else {
	    ilg_fn = "stderr";
	    ilg_fp = stderr;
	    verbose = FALSE;
	}

	if ((fn_no == -1) && (sty_given))
	    scan_sty();
	if (german_sort && (idx_quote == '"'))
FATAL("Option -g ignored, quote character must be different from '%c'.\n",
'"');

	if (need_version) {
	    PUT_VERSION;
	}
	scan_idx();
	fn_no++;
    }
}


static void
#if STDC
check_idx(char *fn, int open_fn)
#else
check_idx(fn, open_fn)
char   *fn;
int     open_fn;
#endif
{
    char   *ptr = fn;
    char   *ext;
    int     with_ext = FALSE;
    int     i = 0;

#if OS_BS2000
    char    message[256];	       /* error message */
    char    arg[8];		       /* argument for sprintf() */

    ext = strrchr(fn, EXT_DELIM);
    if ((ext != NULL) &&
	(((*fn == CATID) && (*(fn + 3) != DIR_DELIM)) ||
	 (((*fn == DIR_DELIM) || (*fn == CATID)) &&
	  (strchr(fn, EXT_DELIM) < ext)))) {
#else
    ext = strrchr(fn, EXT_DELIM);
    if ((ext != NULL) && (ext != fn) && (*(ext + 1) != DIR_DELIM)) {
#endif				       /* OS_BS2000 */
	with_ext = TRUE;
	while ((ptr != ext) && (i < STRING_MAX))
	    base[i++] = *ptr++;
    } else
	while ((*ptr != NUL) && (i < STRING_MAX))
	    base[i++] = *ptr++;

    if (i < STRING_MAX)
	base[i] = NUL;
    else
	FATAL2("Index file name %s too long (max %d).\n",
	       base, STRING_MAX);

    idx_fn = fn;

#if OS_BS2000			       /* Search both on HOME-PVS and
					* WORK-PVS */
    sprintf(widx_fn, "%s%s", WORK, idx_fn);
    if (((((idx_fp = OPEN_IN(idx_fn)) == NULL) && open_fn) ||
	 !(open_fn || idx_fp)) &&
	((((widx_fp = OPEN_IN(widx_fn)) == NULL) && open_fn) ||
	 !(open_fn || widx_fp)))
	if (with_ext) {
	    sprintf(message, "Input index file %s%%s not found.\n",
		    (strchr(idx_fn, ':') || (strlen(WORK) == 0)) ?
		     NIL : "<" WORK ">");
	    FATAL(message, idx_fn);
	} else {

#ifdef DEBUG
	    totmem += STRING_MAX;
	    (void)fprintf(stderr,"check_idx()-1: malloc(%d)\ttotmem = %ld\n",
		    STRING_MAX,totmem);
#endif /* DEBUG */

	    if ((idx_fn = (char *) malloc(STRING_MAX)) == NULL)
		FATAL("Not enough core...abort.\n", "");
	    sprintf(idx_fn, "%s%s", base, INDEX_IDX);
	    sprintf(widx_fn, "%s%s%s", WORK, base, INDEX_IDX);
	    if (((((idx_fp = OPEN_IN(idx_fn)) == NULL) && open_fn) ||
		 !(open_fn || idx_fp)) &&
		((((widx_fp = OPEN_IN(widx_fn)) == NULL) && open_fn) ||
		 !(open_fn || widx_fp))) {
		sprintf(arg, "%s",
			(strchr(idx_fn, ':') || (strlen(WORK) == 0)) ?
			 NIL : "<" WORK ">");
		sprintf(message,
			"Couldn't find neither input file %s%%s nor %s%%s.\n",
			arg, arg);
		FATAL2(message, base, idx_fn);
	    }
	}
    if (!open_fn) {		       /* Close files if open */
	if (idx_fp)
	    FCLOSE(idx_fp);
	if (widx_fp)
	    FCLOSE(widx_fp);
    }
    if (open_fn && (!idx_fp) && widx_fp) {	/* File was found on PVS W */
	idx_fp = widx_fp;
	idx_fn = widx_fn;
    }
#else				       /* NOT OS_BS2000 */
    if ( ( open_fn && 
	 ((idx_fp = OPEN_IN(idx_fn)) == NULL)
	 ) ||
	((!open_fn) && (access(idx_fn, R_OK) != 0)))
	if (with_ext) {
	    FATAL("Input index file %s not found.\n", idx_fn);
	} else {

#ifdef DEBUG
	    totmem += STRING_MAX;
	    (void)fprintf(stderr,"check_idx()-2: malloc(%d)\ttotmem = %ld\n",
		    STRING_MAX,totmem);
#endif /* DEBUG */

	    if ((idx_fn = (char *) malloc(STRING_MAX)) == NULL)
		FATAL("Not enough core...abort.\n", "");
	    sprintf(idx_fn, "%s%s", base, INDEX_IDX);
	    if ((open_fn && 
	 ((idx_fp = OPEN_IN(idx_fn)) == NULL)
	) ||
		((!open_fn) && (access(idx_fn, R_OK) != 0))) {
		FATAL2("Couldn't find input index file %s nor %s.\n", base,
		       idx_fn);
	    }
	}
#endif				       /* OS_BS2000 */
}


static void
#if STDC
check_all(char *fn, int ind_given, int ilg_given, int log_given)
#else
check_all(fn, ind_given, ilg_given, log_given)
char   *fn;
int     ind_given;
int     ilg_given;
int     log_given;
#endif
{
    check_idx(fn, TRUE);

    /* index output file */
    if (!ind_given) {
	sprintf(ind, "%s%s", base, INDEX_IND);
	ind_fn = ind;
#if OS_BS2000			       /* Look on HOME-, then WORK-PVS */
	if ((ind_fp = OPEN_IN(ind_fn)) == NULL) {
	    sprintf(ind, "%s%s%s", strchr(base, ':') ?
				   NIL : WORK, base, INDEX_IND);
	    ind_fn = ind;
	} else
	    fclose(ind_fp);
#endif				       /* OS_BS2000 */
    }
    if ((ind_fp = OPEN_OUT(ind_fn)) == NULL)
	FATAL("Can't create output index file %s.\n", ind_fn);

    /* index transcript file */
    if (!ilg_given) {
	sprintf(ilg, "%s%s", base, INDEX_ILG);
	ilg_fn = ilg;
#if OS_BS2000			       /* Look on HOME-, then WORK-PVS */
	if ((ilg_fp = OPEN_IN(ilg_fn)) == NULL) {
	    sprintf(ilg, "%s%s%s", strchr(base, ':') ?
				   NIL : WORK, base, INDEX_ILG);
	    ilg_fn = ilg;
	} else
	    fclose(ilg_fp);
#endif				       /* OS_BS2000 */
    }
    if ((ilg_fp = OPEN_OUT(ilg_fn)) == NULL)
	FATAL("Can't create transcript file %s.\n", ilg_fn);

#if OS_BS2000			       /* Search both on HOME-PVS ans
					* WORK-PVS */
    if (log_given) {
	sprintf(log_fn, "%s%s%s", strchr(base, ':') ? NIL : WORK,
		base, INDEX_LOG);
	if ((log_fp = OPEN_IN(log_fn)) == NULL) {
	    if (strchr(base, ':'))
		sprintf(log_fn, "%s%s", strrchr(base, ':') + 1, INDEX_LOG);
	    else
		sprintf(log_fn, "%s%s", base, INDEX_LOG);
	    if ((log_fp = OPEN_IN(log_fn)) == NULL)
		FATAL2("Source log file %s%s not found.\n", WORK, log_fn);
	} else {
	    find_pageno();
	    CLOSE(log_fp);
	}
    }
#else
    if (log_given) {
	sprintf(log_fn, "%s%s", base, INDEX_LOG);
#if OS_PCDOS			       /* open in binary mode (CR-LF) */
	if ((log_fp = OPEN_IN_B(log_fn)) == NULL) {
#else
	if ((log_fp = OPEN_IN(log_fn)) == NULL) {
#endif				       /* OS_PCDOS */
	    FATAL("Source log file %s not found.\n", log_fn);
	} else {
	    find_pageno();
	    CLOSE(log_fp);
	}
    }
#endif				       /* OS_BS2000 */
}


static void
find_pageno(VOID_ARG)
{
    int     i = 0;
    int     p, c;

#if (OS_VAXVMS | OS_BS2000 | OS_MVSXA)
    /*
     * Scan forward through the file for VMS, because fseek doesn't work on
     * variable record files
     */
    c = GET_CHAR(log_fp);
    while (c != EOF) {
	p = c;
	c = GET_CHAR(log_fp);
	if (p == LSQ && isdigit(c)) {
	    i = 0;
	    do {
		pageno[i++] = c;
		c = GET_CHAR(log_fp);
	    } while (isdigit(c));
	    pageno[i] = NUL;
	}
    }
    if (i == 0) {
	fprintf(ilg_fp, "Couldn't find any page number in %s...ignored\n",
		log_fn);
	init_page = FALSE;
    }
#else				       /* NOT (OS_VAXVMS | ...) */
    fseek(log_fp, -1L, 2);
    p = GET_CHAR(log_fp);
    fseek(log_fp, -2L, 1);
    do {
	c = p;
	p = GET_CHAR(log_fp);
    } while (!(((p == LSQ) && isdigit(c)) || (fseek(log_fp, -2L, 1) != 0)));
    if (p == LSQ) {
	while ((c = GET_CHAR(log_fp)) == SPC);
	do {
	    pageno[i++] = (char) c;
	    c = GET_CHAR(log_fp);
	} while (isdigit(c));
	pageno[i] = NUL;
    } else {
	fprintf(ilg_fp, "Couldn't find any page number in %s...ignored\n",
		log_fn);
	init_page = FALSE;
    }
#endif				       /* (OS_VAXVMS | ...) */
}

static void
#if STDC
open_sty(char *fn)
#else
open_sty(fn)
char   *fn;
#endif
{
    char   *path;
    char   *ptr;
    int     i;
    int     len;

    if ((path = getenv(STYLE_PATH)) == NULL) {
	/* style input path not defined */
	strcpy(sty_fn, fn);
	sty_fp = OPEN_IN(sty_fn);
    } else {
#if OS_VAXVMS
	if ((strlen(STYLE_PATH) + strlen(fn) + 1) > ARRAY_MAX) {
	    FATAL2("Path %s too long (max %d).\n", STYLE_PATH, ARRAY_MAX);
	} else {
	    sprintf(sty_fn, "%s:%s", STYLE_PATH, fn);
	    sty_fp = OPEN_IN(sty_fn);
	}
#else				       /* NOT OS_VAXVMS */
	len = ARRAY_MAX - strlen(fn) - 1;
	while (*path != NUL) {
#if OS_XENIX
	    i = 0;
	    while ((*path != ENV_SEPAR) && (*path != NUL) && (i < len))
		sty_fn[i++] = *path++;
#else				       /* NOT OS_XENIX */
	    ptr = strchr(path, ENV_SEPAR);
	    i = 0;
	    if (ptr == (char*)NULL)
	    {
		int j = strlen(path);

		while (i < j)
		    sty_fn[i++] = *path++;
	    }
	    else
	    {
		while ((path != ptr) && (i < len))
		    sty_fn[i++] = *path++;
	    }
#endif				       /* OS_XENIX */
	    if (i == len) {
		FATAL2("Path %s too long (max %d).\n", sty_fn, ARRAY_MAX);
	    } else {
#ifdef OS_VAXVMS
		if (sty_fn[i-1] != ']')
		    sty_fn[i++] = DIR_DELIM;
#else
		sty_fn[i++] = DIR_DELIM;
#endif
		sty_fn[i] = NUL;
		strcat(sty_fn, fn);
#if OS_XENIX
		if (((sty_fp = OPEN_IN(sty_fn)) == NULL) && (*path != NUL))
#else				       /* NOT OS_XENIX */
		if ((sty_fp = OPEN_IN(sty_fn)) == NULL)
#endif				       /* OS_XENIX */
		    path++;
		else
		    break;
	    }
	}
#endif				       /* OS_VAXVMS */
    }

    if (sty_fp == NULL)
	FATAL("Index style file %s not found.\n", fn);
}


int
#if STDC
strtoint(char *str)
#else
strtoint(str)
char   *str;
#endif
{
    int     val = 0;

    while (*str != NUL) {
#if (OS_BS2000 | OS_MVSXA)
	val = 10 * val + *str - '0';
#else
	val = 10 * val + *str - 48;
#endif				       /* (OS_BS2000 | OS_MVSXA) */
	str++;
    }
    return (val);
}
