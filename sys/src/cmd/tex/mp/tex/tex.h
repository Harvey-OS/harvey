/*
 * Main include file for TeX in C.
 * You shouldn't have to change anything in this file except the initial
 * #define's/#undef's, and the default search paths.
 *
 * Tim Morgan   December 23, 1987
 */

#include <stdio.h>
#include "site.h"

/* These are used in the change files and in macros defined in tex.h */
#define	inputpathspec	1
#define	readpathspec	2
#define	fontpathspec	3
#define	fmtpathspec	4
#define	poolpathspec	5

/*
 * If we're running on an ASCII system, there is no need to use the
 * xchr[] array to convert characters to the external encoding.
 */
#ifdef	NONASCII
#define	Xchr(x)		xchr[x]
#else
#define	Xchr(x)		((char) (x))
#endif

#ifndef	BSD
#include <string.h>
#else
#include <strings.h>
#endif

/*
 * Some types we'll need.
 */
typedef FILE	*alphafile, *bytefile, *wordfile, *file_ptr;
typedef char	*ccharpointer;

/*
 * Global routines implemented as macros, plus some type coercion stuff.
 */

/* This is a workaround to a casting bug in the Sequent Dynix 2.1 C compiler */
#ifndef	sequent
#define	toint(x)	((integer) (x))
#else
#define	toint(x)	ztoint((integer)(x))
extern integer	ztoint();
#endif

/*
 * Next, efficiency for Unix systems: use write() instead of stdio
 * to write to the dvi file.
 */
#ifndef	unix
#define	writedvi(a,b)	(void) fwrite((char *) &dvibuf[a], sizeof(dvibuf[a]), (int)(b-a+1), dvifile)
#define	dumpcore()	exit(1)
#else	/* unix */
#define	writedvi(a,b)	(void) write(fileno(dvifile), (char *) &dvibuf[a], (int)(b-a+1))
#define	dumpcore	abort
#endif	/* unix */

#define readln(f)	{register c; while ((c=getc(f))!='\n' && c!=EOF); }
#define	abs(x)		((x>=0)?(x):(-(x)))
#define	fabs(x)		((x>=0.0)?(x):(-(x)))
#define	Fputs(stream, s)	(void) fputs(s, stream)
#define	vstrcpy(a,b)		(void) strcpy((a), (b))
#define	inputln(stream, flag)	zinputln(stream)
char zinputln();	/* Get function return type correct=>boolean */
#define	dumpthings(base,len)	(void) fwrite((char *) &(base), sizeof(base), (int)(len), fmtfile)
#define	undumpthings(base,len)	(void) fread((char *) &(base), sizeof(base), (int)(len), fmtfile)
#define	termflush(t)	(void) fflush(t)
#define	eof		feof
#define	incr(x)		++(x)
#define	decr(x)		--(x)
#define	uexit(x)	exit((int) (x))
#define	odd(x)		((x) % 2)
#define	aopenin(f, p)	Openin(&(f), p)
#define	bopenin(f)	Openin(&(f), fontpathspec) /* Only used for TFMs */
#define	wopenin(f)	Openin(&(f), fmtpathspec)  /* Only used for FMTs */
#define	aopenout(f)	Openout(&(f)) /* Always open outputs in cwd */
#define	bopenout(f)	Openout(&(f))
#define	wopenout(f)	Openout(&(f))
#define	aclose(f)	if (f) (void) fclose(f)
#define	bclose(f)	if (f) (void) fclose(f)
#define	wclose(f)	if (f) (void) fclose(f)
#define	read(f, c)	(c = getc(f))
#define	true		1
#define	false		0
#define	chr(x)		(x)
#define	round(x)	toint(x + 0.5)
#define	dateandtime(i,j,k,l)	get_date_and_time(&(i), &(j), &(k), &(l))

#ifndef SIXTEENBIT
#define	putfmtint(x)	(void) putw((int)(x), fmtfile)
#define	getfmtint(x)	x=getw(fmtfile)
#else /* ! SIXTEENBIT */
#define	putfmtint(x)	{integer x_val=(x); (void) fwrite((char *)&x_val,\
				sizeof(x_val), 1, fmtfile);}
#define	getfmtint(x)	fread((char *) &x, sizeof(x), 1, fmtfile)
#endif /* SIXTEENBIT */

#define	genericputfmt(x)(void) fwrite((char *) &(x), sizeof(x), 1, fmtfile)
#define	genericgetfmt(x)(void) fread((char *) &(x), sizeof(x), 1, fmtfile)
#define putfmtword	genericputfmt
#define getfmtword	genericgetfmt
#define putfmthh	genericputfmt
#define getfmthh	genericgetfmt
#define putfmtqqqq   	genericputfmt
#define	getfmtqqqq	genericgetfmt

#define	amakenamestring(f)	makenamestring()
#define	bmakenamestring(f)	makenamestring()
#define	wmakenamestring(f)	makenamestring()
