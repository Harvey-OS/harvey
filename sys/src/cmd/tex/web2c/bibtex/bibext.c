/*
 * Auxiliary routines for BibTeX in C.
 *
 * Tim Morgan 2/15/88
 * Eduardo Krell 4/21/88
 */

#include <stdio.h>
#include "site.h"

#ifndef	BSD
#include <string.h>
#else
#include <strings.h>
#endif

#define TRUE 1
#define FALSE 0
#define filenamesize 1024
extern char realnameoffile[];
extern schar xord[], buffer[];
extern char nameoffile[];
char **gargv;
int gargc;

static char input_path[filenamesize], bib_path[filenamesize];

FILE *openf(name, mode)
char *name, *mode;
{
    FILE *result;

    result = fopen(name, mode);
    if (!result) {
	perror(name);
	exit(1);
    }
    return(result);
}

int eoln(f)
FILE *f;
{
    register int c;

    if (feof(f)) return(1);
    c = getc(f);
    if (c != EOF)
	(void) ungetc(c, f);
    return (c == '\n' || c == EOF);
}

void lineread(f, size)
FILE *f;
int size;
{
    extern long last;
    register int in;

    last = 0;
    while (last < size && (in = getc(f)) != EOF && in != '\n') {
#ifdef	NONASCII
	buffer[last++] = xord[in];
#else
	buffer[last++] = in;
#endif
    }
    while (in != EOF && in != '\n')	/* Skip past eoln if buffer full */
	(void) getc(f);
}

void setpaths()
{
    extern char *getenv();
    char *p;

    if (p = getenv("TEXINPUTS"))
	(void) strcpy(input_path, p);
    else
	(void) strcpy(input_path, TEXINPUTS);
    if (p = getenv("BIBINPUTS"))
	(void) strcpy(bib_path, p);
    else
	(void) strcpy(bib_path, BIBINPUTS);
}


int ztestaccess(wam, filepath)
int wam, filepath;
{
    char *path = NULL;
    register int ok;
    int f;

    if (filepath == 1)
	path = input_path;
    else if (filepath == 2)
	path = bib_path;
    if (nameoffile[0] == '/')
	path = NULL;
    do {
	packrealnameoffile(&path);
	if (wam == 4)
	    if (access(realnameoffile, 4) == 0) ok = TRUE;
	    else ok = FALSE;
        else {
	    f = creat(realnameoffile, 0666);
	    if (f >= 0) ok = TRUE;
	    else ok = FALSE;
	    if (ok)
		(void) close(f);
	}
    }
    while (!ok && path != NULL);
    return(ok);
}

packrealnameoffile(cpp)
char **cpp;
{
    register char *p, *realname;

    realname = realnameoffile;
    if ((p = *cpp)) {
	while ((*p != ':') && *p) {
	    *realname++ = *p++;
	    if (realname == &(realnameoffile[filenamesize-1]))
		break;
	}
	if (*p == '\0') *cpp = NULL;
	else *cpp = p+1;
	*realname++ = '/';
    }
    p = nameoffile;
    while (*p != ' ') {
	if (realname >= &(realnameoffile[filenamesize-1])) {
	    fprintf(stderr, "! Full file name is too long\n");
	    break;
	}
	*realname++ = *p++;
    }
    *realname = '\0';
}

int
getcmdline(buf, n)
char *buf;
{
	strncpy(buf, gargv[1], n);
	return (strlen(buf));
}

main(argc, argv)
char *argv[];
{
    extern schar history;
    extern void main_body();
    int status;

    gargc = argc;
    gargv = argv;
    setpaths();
    main_body();
    status = (history != 0);
    exit(status);
}
