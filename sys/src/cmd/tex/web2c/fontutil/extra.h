/* Main include file for these programs.

01/20/90 Karl Berry
*/


#include <stdio.h>
#include "site.h"

/* Global constants.  */
#define true		1
#define false		0

/* Path searching.  These numbers are used both as indices and just as
   enumeration values.  */
#define TEXINPUTPATH	0
#define TFMFILEPATH	1
#define TEXFORMATPATH	2
#define TEXPOOLPATH	3
#define GFFILEPATH	4
#define PKFILEPATH	5

/* Global routines implemented as macros.  */
#define	abs(x)		((x>=0)?(x):(-(x)))
#define	chr(x)		(x)
#define eof(f)		testeof(f)
#define	fabs(x)		( (x >= 0.0) ? (x) : -(x) )
#define	Fputs(f, s)	(void) fputs(s, f)  /* fixwrites outputs this.  */
#define	odd(x)		((x) % 2)
#define	putbyte(x,f)	putc((char) ((x)&255), f)
#define read(f, b)	b = getc(f)
#define	readln(f)	{register c; while ((c=getc(f)) != '\n' && c != EOF);}
#define	round(x)	zround((double)(x))
#define	reset(f,n)	((f)?fclose(f),(f=openf(n, "r")):(f=openf(n,"r")))
#define rewrite(f,n)	f = openf(n, "w")
#define	toint(x)	((integer) (x))
#define	uexit		exit

/* C doesn't distinguish between text files and other files.  */
typedef FILE *text, *file_ptr;

/* For some initializations of constant strings.  */
typedef char *ccharpointer;

extern integer argc;

/* Routines in extra.c.  */

extern void argv ();
extern boolean eoln ();
extern FILE *openf ();
extern void printreal ();
extern boolean testof ();
extern integer zround ();
