/*
 * Main include file for Metafont in C.
 *
 * Tim Morgan  2/12/88
 */

#include <stdio.h>
#include "site.h"

#ifndef	BSD
#include <string.h>
#else
#include <strings.h>
#endif

/*
 * Some types we're going to need
 */
typedef FILE	*file_ptr;

/*
 * Global routines implemented as macros, plus some type coercion stuff.
 */

/* This is a workaround to a casting bug in the Sequent Dynix 2.1 C compiler */
#ifndef	sequent
#define	toint(x)	((integer) (x))
#else
#define	toint(x)	ztoint((integer)(x))
extern integer ztoint();
#endif

#define	inputln(file,flag)	zinputln(file)
#define	uexit(x)		exit((int) (x))
#define	incr(x)			++(x)
#define	decr(x)			--(x)
#define	flush(f)		(void) fflush(f)
#define	bwritebuf(a,b,c,d)	zbwritebuf(a,b,(integer)(c),(integer)(d))
#define	bwritebyte(f,b)		(void) putc((char)((b)&255), (f))
#define	bwrite2bytes(f,b)	zbwrite2bytes(f,(integer)(b))
#define	bwrite4bytes(f,b)	zbwrite4bytes(f,(integer)(b))
#define	abs(x)		zabs((integer) (x))
integer zabs();
#define	Fputs(stream, s)	(void) fputs(s, stream)
#define eof(f)		feof(f)
#define	read(f, c)	(c = getc(f))
#define readln(f)	{register c; while ((c=getc(f))!='\n' && c!=EOF); }
#define	true		1
#define	false		0
#define	chr(x)		(x)
#define get(x)		(void) getc(x)
#define	close(x)	if (x) (void) fclose(x)
#define	aclose(x)	if (x) (void) fclose(x)
#define	bclose(x)	if (x) (void) fclose(x)
#define	wclose(x)	if (x) (void) fclose(x)
#define	paintrow(a,b,c,d)	zpaintrow((screenrow) a, (pixelcolor) b, c, (screencol) d)
#define odd(x)		((x)%2)

FILE *Fopen();
#define zfopen(f,n,m)	f = Fopen(n+1, m)
#define aopenin(f,p) (testaccess (nameoffile, realnameoffile, 4, (int)(p)) ? zfopen(f, realnameoffile, "r") : false )
#define aopenout(f) (testaccess (nameoffile, realnameoffile, 2, 0) ? zfopen(f, realnameoffile, "w") : false )
#define wopenin(f) (testaccess (nameoffile, realnameoffile, 4, 7) ? zfopen(f, realnameoffile, "r") : false )
#define wopenout(f) (testaccess (nameoffile, realnameoffile, 2, 0) ? zfopen(f, realnameoffile, "w") : false )
#define bopenout(f,n) (testaccess (n, realnameoffile, 2, 0) ? zfopen(f, realnameoffile, "w") : false )

#define	amakenamestring(f)	makenamestring()
#define	bmakenamestring(f)	makenamestring()
#define	wmakenamestring(f)	makenamestring()

#ifdef SIXTEENBIT
#define	dumpint(x)	{integer x_val=(x); (void) fwrite((char *) &x_val, sizeof(x_val), 1, basefile);}
#else
#define	dumpint(x)	(void) putw((int)(x), basefile)
#endif
#define	dumpqqqq(x)	(void) fwrite((char *) &(x), sizeof(x), 1, basefile)
#define	dumpwd(x)	(void) fwrite((char *) &(x), sizeof(x), 1, basefile)
#define	dumphh(x)	(void) fwrite((char *) &(x), sizeof(x), 1, basefile)

#ifdef SIXTEENBIT
#define	undumpint(x)	(void) fread((char *) &(x), sizeof(x), 1, basefile)
#else
#define	undumpint(x)	x = getw(basefile)
#endif
#define	undumpqqqq(x)	(void) fread((char *) &(x), sizeof(x), 1, basefile)
#define	undumpwd(x)	(void) fread((char *) &(x), sizeof(x), 1, basefile)
#define	undumphh(x)	(void) fread((char *) &(x), sizeof(x), 1, basefile)

#define dateandtime(a,b,c,d) ydateandtime(&(a), &(b), &(c), &(d))

/*
 * Global data.
 */

extern int argc;
