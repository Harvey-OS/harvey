/*
 * External procedures for TeXware programs.
 *
 * Tim Morgan, 3/22/88
 */

#include <stdio.h>
#include "site.h"
#ifdef	SYSV
#define       index   strchr          /* Sys V compatibility */
extern int sprintf();
#else	/* BSD */
#ifndef ANSI /*ANSI stdio.h contains sprintf prototype*/
extern char *sprintf();
#endif /*ANSI*/
#endif

#ifdef ANSI
#ifndef	index
#define       index   strchr
#endif /* index */
void argv(integer,char *);
void main(int,char * *);
FILE *openf(char *,char *);
void printreal(double,int,int);
int eoln(FILE *);
long zround(double);
long inputint(FILE *);
void zinput_3ints(long *,long *,long *);
extern char* strcpy(char*,char*);
extern char* strcat(char*,char*);
extern void exit(int);
extern void main_body(void);
extern int atoi(char *);
#endif /* ANSI */

#define	TRUE	1
#define	FALSE	0

integer argc;

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
    extern char *index();

    cp = index(name, ' ');
    if (cp) *cp = '\0';
    result = fopen(name, mode);
    if (result) return(result);
    perror(name);
    exit(1);
    /*NOTREACHED*/
}

/* Print real number r in format n:m */
void printreal(r, n, m)
double r;
int n,m;
{
    char fmt[50];

    (void) sprintf(fmt, "%%%d.%df", n, m);
    (void) printf(fmt, r);
}

/* Print string s in format :n. */
printstring(s, n)
char *s;
int n;
{
    fwrite (s+1, 1, n, stdout);
}

/* Return true on end of line or eof of file, else false */
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

integer zround(f)
double f;
{
    if (f >= 0.0) return((integer)(f + 0.5));
    return((integer)(f - 0.5));
}

/* Read an integer in from file f; read past the subsequent end of line */
integer inputint(f)
FILE *f;
{
    char buffer[20];

    if (fgets(buffer, sizeof(buffer), f)) return(atoi(buffer));
    return(0);
}

void zinput_3ints(a,b,c)
integer *a, *b, *c;
{
    while (scanf("%ld %ld %ld\n", a, b, c) != 3)
	(void) fprintf(stderr, "Please enter three integers\n");
}
