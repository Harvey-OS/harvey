/* Copyright 1990, AT&T Bell Labs */

#include "fsort.h"

FILE *
fileopen(char *name, char *mode)
{
	FILE *f;

	if(strcmp(name,"-") == 0)
		if(strcmp(mode, "r") == 0)
			f = stdin;
		else
			f = stdout;
	else
		f = fopen(name, mode);
	if(f == 0)
		fatal("can't open", name, 0);
	return f;
}

void
fileclose(FILE *f, char *name)
{
	if(fclose(f)==EOF && name!=0)
		fatal("error on", name, 0);
}

void
cleanfiles(int n)
{
	char *name;

	while(--n >= 0) {
		name = filename(n);
		remove(name);
		free(name);
	}
}

char*
filename(int number)
{
	char name[50];
	char *s;
	int i;

	for(i=0; (s=tname[i])!=0; i++)
		if(access(s, OREAD) != -1)
			break;
	if(s == 0)
		fatal("no accessible temp directory", "", 0);
	sprintf(name, "%s/stm%.5d.%.4d", s, getpid(), number);
	s = strdup(name);
	if(s == 0)
		fatal("out of malloc space", "", 0);
	return s;
}

int
getline(Rec *r, uchar *bound, FILE *input)
{
	uchar *dp = data(r);
	uchar *cp = dp;
	int c;

	if(feof(input))		/* newline was appended */
		return -1;
	for(;;) {
		if(cp >= bound) {	/* truncated record */
			r->dlen = cp - dp;
			return -2;
		}
		switch(c = getc(input)) {
		case EOF:
			if(cp == dp)
				return -1;
			warn("newline appended", "", 0);
		case '\n':
			r->dlen = cp - dp;
			return 0;
		default:
			*cp++ = c;
		}
	}
	return 0;		/* dummy to shut up compiler */
}

static	char*	level = "warning";

void
warn(char *m, char *s, int l)
{
	fprintf(stderr, "sort: %s: %s %.*s\n",
		level, m, l==0?strlen(s):l, s);
}

void
fatal(char *m, char *s, int l)
{
	level = "error";
	warn(m, s, l);
	perror("");
	exits(s);
}

int
overwrite(int nfile, char **names)
{
	int i;
	Dir sb1, sb2;

	if(strcmp(oname, "-") == 0)
		return 0;
	if(dirstat(oname, &sb1) == -1)
		return 0;
	for(i=0; i<nfile; i++) {
		if(dirstat(names[i], &sb2) == -1)
			fatal("cannot locate", names[i], 0);
		if(sb1.qid.path==sb2.qid.path &&
		   sb1.qid.vers==sb2.qid.vers &&
		   sb1.dev==sb2.dev && sb1.type==sb2.type)
			return 1;
	}
	return 0;
}
