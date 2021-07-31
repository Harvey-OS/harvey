/*
Copyright (c) 1989 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

char	*version = "version Feb 20, 1992";

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "awk.h"
#include "y.tab.h"

extern	char	**environ;

int	dbg	= 0;
uchar	*cmdname;	/* gets argv[0] for error messages */
extern	FILE	*yyin;	/* lex input file */
uchar	*lexprog;	/* points to program argument if it exists */
extern	int errorflag;	/* non-zero if any syntax errors; set by yyerror */
int	compile_time = 2;	/* for error printing: */
				/* 2 = cmdline, 1 = compile, 0 = running */

uchar	*pfile[20];	/* program filenames from -f's */
int	npfile = 0;	/* number of filenames */
int	curpfile = 0;	/* current filename */


main(int argc, uchar *argv[])
{
	uchar *fs = NULL;

	cmdname = argv[0];
	if (argc == 1) {
		fprintf(stderr, "Usage: %s [-f programfile | 'program'] [-Ffieldsep] [-v var=value] [files]\n", cmdname);
		exit(1);
	}
	signal(SIGFPE, fpecatch);
	yyin = NULL;
	syminit();
	while (argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		if (strcmp((char *) argv[1], "--") == 0) {	/* explicit end of args */
			argc--;
			argv++;
			break;
		}
		switch (argv[1][1]) {
		case 'f':	/* next argument is program filename */
			argc--;
			argv++;
			if (argc <= 1)
				ERROR "no program filename" FATAL;
			pfile[npfile++] = argv[1];
			break;
		case 'F':	/* set field separator */
			if (argv[1][2] != 0) {	/* arg is -Fsomething */
				if (argv[1][2] == 't' && argv[1][3] == 0)	/* wart: t=>\t */
					fs = (uchar *) "\t";
				else if (argv[1][2] != 0)
					fs = &argv[1][2];
			} else {		/* arg is -F something */
				argc--; argv++;
				if (argc > 1 && argv[1][0] == 't' && argv[1][1] == 0)	/* wart: t=>\t */
					fs = (uchar *) "\t";
				else if (argc > 1 && argv[1][0] != 0)
					fs = &argv[1][0];
			}
			if (fs == NULL || *fs == '\0')
				ERROR "field separator FS is empty" WARNING;
			break;
		case 'v':	/* -v a=1 to be done NOW.  one -v for each */
			if (argv[1][2] == '\0' && --argc > 1 && isclvar((++argv)[1]))
				setclvar(argv[1]);
			break;
		case 'd':
			dbg = atoi(&argv[1][2]);
			if (dbg == 0)
				dbg = 1;
			printf("awk %s\n", version);
			break;
		default:
			ERROR "unknown option %s ignored", argv[1] WARNING;
			break;
		}
		argc--;
		argv++;
	}
	/* argv[1] is now the first argument */
	if (npfile == 0) {	/* no -f; first argument is program */
		if (argc <= 1)
			ERROR "no program given" FATAL;
		dprintf( ("program = |%s|\n", argv[1]) );
		lexprog = argv[1];
		argc--;
		argv++;
	}
	compile_time = 1;
	argv[0] = cmdname;	/* put prog name at front of arglist */
	dprintf( ("argc=%d, argv[0]=%s\n", argc, argv[0]) );
	arginit(argc, argv);
	envinit(environ);
	yyparse();
	if (fs)
		*FS = tostring(qstring(fs, '\0'));
	dprintf( ("errorflag=%d\n", errorflag) );
	if (errorflag == 0) {
		compile_time = 0;
		run(winner);
	} else
		bracecheck();
	return(errorflag);
}

pgetc(void)		/* get program character */
{
	int c;

	for (;;) {
		if (yyin == NULL) {
			if (curpfile >= npfile)
				return EOF;
			if (strcmp((char *) pfile[curpfile], "-") == 0)
				yyin = stdin;
			else if ((yyin = fopen((char *) pfile[curpfile], "r")) == NULL)
				ERROR "can't open file %s", pfile[curpfile] FATAL;
		}
		if ((c = getc(yyin)) != EOF)
			return c;
		fclose(yyin);
		yyin = NULL;
		curpfile++;
	}
}
