/*
Copyright (c) 1989 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

#define DEBUG
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include "awk.h"
#include "y.tab.h"

#define	getfval(p)	(((p)->tval & (ARR|FLD|REC|NUM)) == NUM ? (p)->fval : r_getfval(p))
#define	getsval(p)	(((p)->tval & (ARR|FLD|REC|STR)) == STR ? (p)->sval : r_getsval(p))

FILE	*infile	= NULL;
uchar	*file	= (uchar*) "";
uchar	recdata[RECSIZE];
uchar	*record	= recdata;
uchar	fields[RECSIZE];

#define	MAXFLD	200
int	donefld;	/* 1 = implies rec broken into fields */
int	donerec;	/* 1 = record is valid (no flds have changed) */

#define	FINIT	{ OCELL, CFLD, NULL, (uchar*) "", 0.0, FLD|STR|DONTFREE }

Cell fldtab[MAXFLD] = {		/* room for fields */
	{ OCELL, CFLD, (uchar*) "$0", recdata, 0.0, REC|STR|DONTFREE},
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
	FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT, FINIT,
};
int	maxfld	= 0;	/* last used field */
int	argno	= 1;	/* current input argument number */
extern	Awkfloat *ARGC;

void initgetrec(void)
{
	int i;
	uchar *p;

	for (i = 1; i < *ARGC; i++) {
		if (!isclvar(p = getargv(i)))	/* find 1st real filename */
			return;
		setclvar(p);	/* a commandline assignment before filename */
		argno++;
	}
	infile = stdin;		/* no filenames, so use stdin */
	/* *FILENAME = file = (uchar*) "-"; */
}

getrec(uchar *buf)
{
	int c;
	static int firsttime = 1;

	if (firsttime) {
		firsttime = 0;
		initgetrec();
	}
	dprintf( ("RS=<%s>, FS=<%s>, ARGC=%g, FILENAME=%s\n",
		*RS, *FS, *ARGC, *FILENAME) );
	donefld = 0;
	donerec = 1;
	buf[0] = 0;
	while (argno < *ARGC || infile == stdin) {
		dprintf( ("argno=%d, file=|%s|\n", argno, file) );
		if (infile == NULL) {	/* have to open a new file */
			file = getargv(argno);
			if (*file == '\0') {	/* it's been zapped */
				argno++;
				continue;
			}
			if (isclvar(file)) {	/* a var=value arg */
				setclvar(file);
				argno++;
				continue;
			}
			*FILENAME = file;
			dprintf( ("opening file %s\n", file) );
			if (*file == '-' && *(file+1) == '\0')
				infile = stdin;
			else if ((infile = fopen((char *)file, "r")) == NULL)
				ERROR "can't open file %s", file FATAL;
			setfval(fnrloc, 0.0);
		}
		c = readrec(buf, RECSIZE, infile);
		if (c != 0 || buf[0] != '\0') {	/* normal record */
			if (buf == record) {
				if (!(recloc->tval & DONTFREE))
					xfree(recloc->sval);
				recloc->sval = record;
				recloc->tval = REC | STR | DONTFREE;
				if (isnumber(recloc->sval)) {
					recloc->fval = atof(recloc->sval);
					recloc->tval |= NUM;
				}
			}
			setfval(nrloc, nrloc->fval+1);
			setfval(fnrloc, fnrloc->fval+1);
			return 1;
		}
		/* EOF arrived on this file; set up next */
		if (infile != stdin)
			fclose(infile);
		infile = NULL;
		argno++;
	}
	return 0;	/* true end of file */
}

readrec(uchar *buf, int bufsize, FILE *inf)	/* read one record into buf */
{
	register int sep, c;
	register uchar *rr;
	register int nrr;

	if ((sep = **RS) == 0) {
		sep = '\n';
		while ((c=getc(inf)) == '\n' && c != EOF)	/* skip leading \n's */
			;
		if (c != EOF)
			ungetc(c, inf);
	}
	for (rr = buf, nrr = bufsize; ; ) {
		for (; (c=getc(inf)) != sep && c != EOF; *rr++ = c)
			if (--nrr < 0)
				ERROR "input record `%.30s...' too long", buf FATAL;
		if (**RS == sep || c == EOF)
			break;
		if ((c = getc(inf)) == '\n' || c == EOF) /* 2 in a row */
			break;
		*rr++ = '\n';
		*rr++ = c;
	}
	if (rr > buf + bufsize)
		ERROR "input record `%.30s...' too long", buf FATAL;
	*rr = 0;
	dprintf( ("readrec saw <%s>, returns %d\n", buf, c == EOF && rr == buf ? 0 : 1) );
	return c == EOF && rr == buf ? 0 : 1;
}

uchar *getargv(int n)	/* get ARGV[n] */
{
	Cell *x;
	uchar *s, temp[10];
	extern Array *ARGVtab;

	sprintf((char *)temp, "%d", n);
	x = setsymtab(temp, "", 0.0, STR, ARGVtab);
	s = getsval(x);
	dprintf( ("getargv(%d) returns |%s|\n", n, s) );
	return s;
}

void setclvar(uchar *s)	/* set var=value from s */
{
	uchar *p;
	Cell *q;

	for (p=s; *p != '='; p++)
		;
	*p++ = 0;
	p = qstring(p, '\0');
	q = setsymtab(s, p, 0.0, STR, symtab);
	setsval(q, p);
	if (isnumber(q->sval)) {
		q->fval = atof(q->sval);
		q->tval |= NUM;
	}
	dprintf( ("command line set %s to |%s|\n", s, p) );
}


void fldbld(void)
{
	register uchar *r, *fr, sep;
	Cell *p;
	int i;

	if (donefld)
		return;
	if (!(recloc->tval & STR))
		getsval(recloc);
	r = recloc->sval;	/* was record! */
	fr = fields;
	i = 0;	/* number of fields accumulated here */
	if (strlen(*FS) > 1) {	/* it's a regular expression */
		i = refldbld(r, *FS);
	} else if ((sep = **FS) == ' ') {
		for (i = 0; ; ) {
			while (*r == ' ' || *r == '\t' || *r == '\n')
				r++;
			if (*r == 0)
				break;
			i++;
			if (i >= MAXFLD)
				break;
			if (!(fldtab[i].tval & DONTFREE))
				xfree(fldtab[i].sval);
			fldtab[i].sval = fr;
			fldtab[i].tval = FLD | STR | DONTFREE;
			do
				*fr++ = *r++;
			while (*r != ' ' && *r != '\t' && *r != '\n' && *r != '\0');
			*fr++ = 0;
		}
		*fr = 0;
	} else if (*r != 0) {	/* if 0, it's a null field */
		for (;;) {
			i++;
			if (i >= MAXFLD)
				break;
			if (!(fldtab[i].tval & DONTFREE))
				xfree(fldtab[i].sval);
			fldtab[i].sval = fr;
			fldtab[i].tval = FLD | STR | DONTFREE;
			while (*r != sep && *r != '\n' && *r != '\0')	/* \n always a separator */
				*fr++ = *r++;
			*fr++ = 0;
			if (*r++ == 0)
				break;
		}
		*fr = 0;
	}
	if (i >= MAXFLD)
		ERROR "record `%.20s...' has too many fields", record FATAL;
	/* clean out junk from previous record */
	cleanfld(i, maxfld);
	maxfld = i;
	donefld = 1;
	for (p = fldtab+1; p <= fldtab+maxfld; p++) {
		if(isnumber(p->sval)) {
			p->fval = atof(p->sval);
			p->tval |= NUM;
		}
	}
	setfval(nfloc, (Awkfloat) maxfld);
	if (dbg)
		for (p = fldtab; p <= fldtab+maxfld; p++)
			printf("field %d: |%s|\n", p-fldtab, p->sval);
}

void cleanfld(int n1, int n2)	/* clean out fields n1..n2 inclusive */
{
	static uchar *nullstat = (uchar *) "";
	register Cell *p, *q;

	for (p = &fldtab[n2], q = &fldtab[n1]; p > q; p--) {
		if (!(p->tval & DONTFREE))
			xfree(p->sval);
		p->tval = FLD | STR | DONTFREE;
		p->sval = nullstat;
	}
}

void newfld(int n)	/* add field n (after end) */
{
	if (n >= MAXFLD)
		ERROR "creating too many fields", record FATAL;
	cleanfld(maxfld, n);
	maxfld = n;
	setfval(nfloc, (Awkfloat) n);
}

refldbld(uchar *rec, uchar *fs)	/* build fields from reg expr in FS */
{
	uchar *fr;
	void *p;
	int i;

	fr = fields;
	*fr = '\0';
	if (*rec == '\0')
		return 0;
	p = compre(fs);
	dprintf( ("into refldbld, rec = <%s>, pat = <%s>\n", rec, fs) );
	for (i = 1; i < MAXFLD; i++) {
		if (!(fldtab[i].tval & DONTFREE))
			xfree(fldtab[i].sval);
		fldtab[i].tval = FLD | STR | DONTFREE;
		fldtab[i].sval = fr;
		dprintf( ("refldbld: i=%d\n", i) );
		if (nematch(p, rec, rec)) {
			dprintf( ("match %s (%d chars)\n", patbeg, patlen) );
			strncpy(fr, rec, patbeg-rec);
			fr += patbeg - rec + 1;
			*(fr-1) = '\0';
			rec = patbeg + patlen;
		} else {
			dprintf( ("no match %s\n", rec) );
			strcpy(fr, rec);
			break;
		}
	}
	return i;		
}

void recbld(void)
{
	int i;
	register uchar *r, *p;
	static uchar rec[RECSIZE];	/* ought to be dynamic */

	if (donerec == 1)
		return;
	r = rec;
	for (i = 1; i <= *NF; i++) {
		p = getsval(&fldtab[i]);
		while (r < rec+RECSIZE-1 && (*r = *p++))
			r++;
		if (i < *NF)
			for (p = *OFS; r < rec+RECSIZE-1 && (*r = *p++); )
				r++;
	}
	if (r > rec + RECSIZE - 1)
		ERROR "built giant record `%.20s...'", record FATAL;
	*r = '\0';
	dprintf( ("in recbld FS=%o, recloc=%o\n", **FS, recloc) );
	recloc->tval = REC | STR | DONTFREE;
	recloc->sval = record = rec;
	dprintf( ("in recbld FS=%o, recloc=%o\n", **FS, recloc) );
	dprintf( ("recbld = |%s|\n", record) );
	donerec = 1;
}

Cell *fieldadr(int n)
{
	if (n < 0 || n >= MAXFLD)
		ERROR "trying to access field %d", n FATAL;
	return(&fldtab[n]);
}

int	errorflag	= 0;
char	errbuf[200];

void yyerror(uchar *s)
{
	extern uchar *cmdname, *curfname;
	static int been_here = 0;

	if (been_here++ > 2)
		return;
	fprintf(stderr, "%s: %s", cmdname, s);
	fprintf(stderr, " at source line %d", lineno);
	if (curfname != NULL)
		fprintf(stderr, " in function %s", curfname);
	fprintf(stderr, "\n");
	errorflag = 2;
	eprint();
}

void fpecatch(int n)
{
	ERROR "floating point exception %d", n FATAL;
}

extern int bracecnt, brackcnt, parencnt;

void bracecheck(void)
{
	int c;
	static int beenhere = 0;

	if (beenhere++)
		return;
	while ((c = input()) != EOF && c != '\0')
		bclass(c);
	bcheck2(bracecnt, '{', '}');
	bcheck2(brackcnt, '[', ']');
	bcheck2(parencnt, '(', ')');
}

void bcheck2(int n, int c1, int c2)
{
	if (n == 1)
		fprintf(stderr, "\tmissing %c\n", c2);
	else if (n > 1)
		fprintf(stderr, "\t%d missing %c's\n", n, c2);
	else if (n == -1)
		fprintf(stderr, "\textra %c\n", c2);
	else if (n < -1)
		fprintf(stderr, "\t%d extra %c's\n", -n, c2);
}

void error(int f, char *s)
{
	extern Node *curnode;
	extern uchar *cmdname;

	fflush(stdout);
	fprintf(stderr, "%s: ", cmdname);
	fprintf(stderr, "%s", s);
	fprintf(stderr, "\n");
	if (compile_time != 2 && NR && *NR > 0) {
		fprintf(stderr, " input record number %g", *FNR);
		if (strcmp(*FILENAME, "-") != 0)
			fprintf(stderr, ", file %s", *FILENAME);
		fprintf(stderr, "\n");
	}
	if (compile_time != 2 && curnode)
		fprintf(stderr, " source line number %d\n", curnode->lineno);
	else if (compile_time != 2 && lineno)
		fprintf(stderr, " source line number %d\n", lineno);
	eprint();
	if (f) {
		if (dbg)
			abort();
		exit(2);
	}
}

void eprint(void)	/* try to print context around error */
{
	uchar *p, *q;
	int c;
	static int been_here = 0;
	extern uchar ebuf[300], *ep;

	if (compile_time == 2 || compile_time == 0 || been_here++ > 0)
		return;
	p = ep - 1;
	if (p > ebuf && *p == '\n')
		p--;
	for ( ; p > ebuf && *p != '\n' && *p != '\0'; p--)
		;
	while (*p == '\n')
		p++;
	fprintf(stderr, " context is\n\t");
	for (q=ep-1; q>=p && *q!=' ' && *q!='\t' && *q!='\n'; q--)
		;
	for ( ; p < q; p++)
		if (*p)
			putc(*p, stderr);
	fprintf(stderr, " >>> ");
	for ( ; p < ep; p++)
		if (*p)
			putc(*p, stderr);
	fprintf(stderr, " <<< ");
	if (*ep)
		while ((c = input()) != '\n' && c != '\0' && c != EOF) {
			putc(c, stderr);
			bclass(c);
		}
	putc('\n', stderr);
	ep = ebuf;
}

void bclass(int c)
{
	switch (c) {
	case '{': bracecnt++; break;
	case '}': bracecnt--; break;
	case '[': brackcnt++; break;
	case ']': brackcnt--; break;
	case '(': parencnt++; break;
	case ')': parencnt--; break;
	}
}

double errcheck(double x, uchar *s)
{
	extern int errno;

	if (errno == EDOM) {
		errno = 0;
		ERROR "%s argument out of domain", s WARNING;
		x = 1;
	} else if (errno == ERANGE) {
		errno = 0;
		ERROR "%s result out of range", s WARNING;
		x = 1;
	}
	return x;
}

isclvar(uchar *s)	/* is s of form var=something? */
{
	uchar *os = s;

	if (!isalpha(*s) && *s != '_')
		return 0;
	for ( ; *s; s++)
		if (!(isalnum(*s) || *s == '_'))
			break;
	return *s == '=' && s > os && *(s+1) != '=';
}

#define	MAXEXPON	38	/* maximum exponent for fp number */

isnumber(uchar *s)
{
	register int d1, d2;
	int point;
	uchar *es;

	d1 = d2 = point = 0;
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(0);	/* empty stuff isn't number */
	if (*s == '+' || *s == '-')
		s++;
	if (!isdigit(*s) && *s != '.')
		return(0);
	if (isdigit(*s)) {
		do {
			d1++;
			s++;
		} while (isdigit(*s));
	}
	if (*s == '.') {
		point++;
		s++;
	}
	if (isdigit(*s)) {
		d2++;
		do {
			s++;
		} while (isdigit(*s));
	}
	if (!(d1 || point && d2))
		return(0);
	if (*s == 'e' || *s == 'E') {
		s++;
		if (*s == '+' || *s == '-')
			s++;
		if (!isdigit(*s))
			return(0);
		es = s;
		do {
			s++;
		} while (isdigit(*s));
		if (s - es > 2)
			return(0);
		else if (s - es == 2 && (int)(10 * (*es-'0') + *(es+1)-'0') >= MAXEXPON)
			return(0);
	}
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(1);
	else
		return(0);
}
