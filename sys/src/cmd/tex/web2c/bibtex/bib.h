/*
 * Main include file for BibTeX in C.
 *
 * Tim Morgan  2/15/88
 * Eduardo Krell 4/21/88
 */

#include <stdio.h>
#include <setjmp.h>
#include "site.h"

#undef	STAT
#undef	DEBUG

extern char *strncpy();

typedef FILE *file_ptr;
typedef FILE *palphafile;
#define	incr(x)	++(x)
#define	decr(x)	--(x)
extern int ztestaccess();
#define	testaccess(a,b)	ztestaccess((int)(a),(int)(b))
#define	true	1
#define	false	0
#define	rewrite(f,n)	f=openf(n,"w")
#define	reset(f,n)	f=openf(n,"r")
#define	aclose(f)	if (f) (void) fclose(f)
#define	chr(x)		(x)
extern void lineread(), setpaths();
#define readln(f)	{register c; while ((c=getc(f))!='\n' && c!=EOF); }
extern FILE *openf();
#define	uexit(x)	exit((int) (x))
#define	eof(f)		feof(f)
#define	Fputs(stream, s)	(void) fputs(s, stream)
#define printstr(s,c)	fprintf(logfile,"%s%c",s,c);fprintf(stdout,"%s%c",s,c)

extern char **gargv;
extern int gargc;
