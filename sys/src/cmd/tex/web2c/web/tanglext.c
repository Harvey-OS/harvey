/*
 * Hand-coded routines for C TeX.
 * This module should contain any system-dependent code.
 */

#define	CATCHINT		/* Catch ^C's */

#define	EXTERN		/* Actually instantiate globals here */

#include <signal.h>
#ifdef	BSD
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <stdio.h>
#include "site.h"

#define BUF_SIZE 100		/* should agree with tangle.web */

extern char buffer[];		/* 0..BUF_SIZE.  Input goes here */
extern char outbuf[];		/* 0..OUT_BUF_SIZE. Output from here */
extern char xord[], xchr[];	/* character translation arrays */
extern int limit;		

#ifdef	SYSV
#define	index	strchr		/* Sys V compatibility */
extern int sprintf();
#else /* ! SYSV */
#ifndef ANSI /* ANSI stdio.h contains sprintf prototype. */
extern char *sprintf();
#endif /* ANSI */
#endif /* SYSV */

#ifdef ANSI
#define       index   strchr
void main(int iargc,char * *iargv);
void argv(integer n,char *s);
int eoln(FILE *f);
FILE *openf(char *name,char *mode);
extern void main_body(void);
extern void exit(int);
#else
extern char *malloc(), *index(), *getenv();
#endif /* ANSI */

/* C library stuff that we're going to use */
#ifndef	BSD
#include <string.h>
#else
#include <strings.h>
#endif

/* Local stuff */
static int gargc;
integer argc;
static char **gargv;


#define	TRUE	1
#define	FALSE	0

/*
 * Get things going under Unix: set up for rescanning the command line,
 * then call the main body.
 */
void main(iargc, iargv)
int iargc;
char *iargv[];
{
    gargc=argc=iargc;
    gargv=iargv;
    main_body();
    exit(0);
} 

/* Return the nth argument into the string s (which is indexed starting at 1) */
void argv(n,s)
integer n;
char s[];
{
    (void) sprintf(s+1, "%s ", gargv[n]);
}


/* Same as in Pascal --- return TRUE if EOF or next char is newline */
eoln(f)
FILE *f;
{
    register int c;

    if (feof(f)) return(TRUE);
    c = getc(f);
    if (c != EOF)
	(void) ungetc(c, f);
    return (c == '\n' || c == EOF);
}

/* Open a file; don't return if error */
FILE *openf(name, mode)
char *name, *mode;
{
    FILE *result;
    char *index(), *cp;

    cp = index(name, ' ');
    if (cp) *cp = '\0';
    result = fopen(name, mode);
    if (result) return(result);
    perror(name);
    exit(1);
    /*NOTREACHED*/
}
