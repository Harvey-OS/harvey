/* External procedures for weave */
/*   Written by: H. Trickey, 11/17/83   */

/* Note: these procedures aren't necessary; the default input_ln and
 * flush_buffer routines in tangle/weave work fine on UNIX.
 * However a very big time improvement is acheived by using these.
 *
 * These procedures are the same as for tangle, except for a slight offset
 * in which characters are output from outbuf in linewrite.
 */

#include <stdio.h>
#include "site.h"

#ifdef	SYSV
#include <string.h>
#else
#ifndef ANSI /*ANSI stdio.h contains sprintf prototype*/
extern char *sprintf();
#endif /* ANSI */
#endif /* SYSV */

#ifdef	ANSI
void lineread(FILE *);
void linewrite(FILE *,long);
int testeof(FILE *);
void argv(integer,char *);
void main(int,char * *);
FILE *openf(char *,char *);
extern void main_body(void);
extern void exit(int);
extern char* strcpy(char*,char*);
extern char* strcat(char*,char*);
extern char* strchr(char*,int);
#endif

#define BUF_SIZE 100		/* should agree with tangle.web */
extern schar buffer[];		/* 0..BUF_SIZE.  Input goes here */
integer argc;
extern schar outbuf[];		/* 0..OUT_BUF_SIZE. Output from here */
extern schar xord[];		/* character translation arrays */
extern char xchr[];		/* other direction */
extern integer limit;		/* index into buffer.  Note that limit
				   is 0..long_buf_size in weave.web which
				   differs from the definition in tangle.web */
#define	TRUE	1
#define	FALSE	0


/*
 * lineread reads from the Pascal text file with iorec pointer filep
 * into buffer[0], buffer[1],..., buffer[limit-1] (and
 * setting "limit").
 * Characters are read until a newline is found (which isn't put in the
 * buffer) or until the next character would go into buffer[BUF_SIZE].
 * And trailing blanks are to be ignored, so limit is really set to
 * one past the last non-blank.
 * The characters need to be translated, so really xord[c] is put into
 * the buffer when c is read.
 * If end-of-file is encountered, the funit field of *filep is set
 * appropriately.
 */
void lineread(iop)
FILE *iop;
{
	register c;
	register schar *cs; /* pointer into buffer where next char goes */
	register schar *cnb; /* last non-blank character input */
	register l; /* how many chars allowed before buffer overflow */
	
	cnb = cs = &(buffer[0]);
	l = BUF_SIZE;
	  /* overflow when next char would go into buffer[BUF_SIZE] */
	while (--l>=0 && (c = getc(iop)) != EOF && c!='\n')
	    if ((*cs++ = xord[c])!=' ') cnb = cs;
	limit = cnb - &(buffer[0]);
}

/*
 * linewrite writes to the Pascal text file with iorec pointer filep
 * from outbuf[1], outbuf[1],..., outbuf[cnt].
 * (Note the offset of indices vis a vis the tangext version of this.)
 * Translation is done, so that xchr[c] is output instead of c.
 */
 void linewrite(iop, cnt)
 FILE *iop;
 integer cnt;
 {
	register schar *cs; /* pointer to next character to output */
	register int l; /* how many characters left to output */
	
	cs = &(outbuf[1]);
	l = (int) cnt;
	while (--l>=0) putc(xchr[*cs++], iop);
}
	
/*
**	testeof(filep)
**
**  Test whether or not the Pascal text file with iorec pointer filep
**  has reached end-of-file (when the only I/O on it is done with
**  lineread, above).
**  We may have to read the next character and unget it to see if perhaps
**  the end-of-file is next.
*/

testeof(iop)
FILE *iop;
{
	register int c;
	if (feof(iop))
		return(TRUE);
	else { /* check to see if next is EOF */
		c = getc(iop);
		if (c == EOF)
			return(TRUE);
		else {
			(void) ungetc(c,iop);
			return(FALSE);
		}
	}
}

static char **gargv;
void argv(n, buf)
integer n;
char buf[];
{
#ifndef ANSI
    extern char *strcpy(), *strcat();
#endif

    (void) strcpy(buf+1, gargv[n]);
    (void) strcat(buf+1, " ");
}

void main(ac, av)
char *av[];
{
    argc = ac;
    gargv = av;
    main_body();
    exit(0);
}

/* Open a file; don't return if error */
FILE *openf(name, mode)
char *name, *mode;
{
    FILE *result;
    char *cp;

    /* Sometimes the names will have trailing spaces, which Unix can't hack */
    for (cp=name; *cp && *cp != ' '; ++cp);
    if (*cp == ' ') *cp = '\0';

    result = fopen(name, mode);
    if (result) return(result);
    perror(name);
    exit(1);
    /*NOTREACHED*/
}
