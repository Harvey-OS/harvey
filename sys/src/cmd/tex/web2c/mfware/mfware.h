/*
 * Main include file for MFware programs
 *
 * Tim Morgan  2/15/89
 */

#include <stdio.h>
#include "site.h"

/*
 * Global routines implemented as macros, plus some type coercion stuff.
 */

#define	writedvi(a,b)	(void) fwrite((char *) &dvibuf[a], sizeof(dvibuf[a]), (int)(b-a+1), dvifile)
#define	toint(x)	((integer) (x))
#define	odd(x)		((x) % 2)
#define	putbyte(x,f)	putc(((x)&255), f)
#define	round(x)	zround((double)(x))
#define incr(x)		++x
#define	decr(x)		--x
#define	trunc(x)	( (integer) (x) )
#define	readln(f)	{register c; while ((c=getc(f)) != '\n' && c != EOF);}
#define	read(f, b)	b = getc(f)
#define	input3ints(a,b,c)	zinput_3ints(&a, &b, &c)
#define zfseek(f,n,w)	(void) fseek(f, (long) n, (int) w)
#define eof(f)		testeof(f)
#define	abs(x)		((x>=0)?(x):(-(x)))
#define	Fputs(stream, s)	(void) fputs(s, stream)
#define rewrite(f,n)	((f)?fclose(f),(f=openf(n+1, "w")):(f=openf(n+1,"w")))
#define	reset(f,n)	((f)?fclose(f),(f=openf(n+1, "r")):(f=openf(n+1,"r")))
#define flush(f)	(void) fflush(f)
#define	true		1
#define	false		0
#define	chr(x)		(x)
#define	ord(x)		(x)
#define	vgetc(f)	(void) getc(f)
#define	vstrcpy(a,b)	(void) strcpy(a, b)
#define	uexit(x)	exit(x)

#ifdef	ANSI
FILE *openf(char *, char *);
#else
FILE *openf();
void main_body();
#endif

/*
 * Global types, data, and routines.
 */
typedef FILE	*text, *file_ptr;
typedef char	*ccharpointer;

extern integer argc;

extern integer inputint();
#ifdef	ANSI
extern integer zround(double f);
#else
extern integer zround();
#endif
