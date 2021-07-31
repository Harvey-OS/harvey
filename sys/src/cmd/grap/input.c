#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "grap.h"
#include "y.tab.h"

Infile	infile[10];
Infile	*curfile = infile;

#define	MAXSRC	50
Src	src[MAXSRC];	/* input source stack */
Src	*srcp	= src;

void pushsrc(int type, char *ptr)	/* new input source */
{
	if (++srcp >= src + MAXSRC)
		ERROR "inputs nested too deep" FATAL;
	srcp->type = type;
	srcp->sp = ptr;
	if (dbg) {
		printf("\n%3d ", srcp - src);
		switch (srcp->type) {
		case File:
			printf("push file %s\n", ((Infile *)ptr)->fname);
			break;
		case Macro:
			printf("push macro <%s>\n", ptr);
			break;
		case Char:
			printf("push char <%c>\n", *ptr);
			break;
		case Thru:
			printf("push thru\n");
			break;
		case String:
			printf("push string <%s>\n", ptr);
			break;
		case Free:
			printf("push free <%s>\n", ptr);
			break;
		default:
			ERROR "pushed bad type %d", srcp->type FATAL;
		}
	}
}

void popsrc(void)	/* restore an old one */
{
	if (srcp <= src)
		ERROR "too many inputs popped" FATAL;
	if (dbg) {
		printf("%3d ", srcp - src);
		switch (srcp->type) {
		case File:
			printf("pop file\n");
			break;
		case Macro:
			printf("pop macro\n");
			break;
		case Char:
			printf("pop char <%c>\n", *srcp->sp);
			break;
		case Thru:
			printf("pop thru\n");
			break;
		case String:
			printf("pop string\n");
			break;
		case Free:
			printf("pop free\n");
			break;
		default:
			ERROR "pop weird input %d", srcp->type FATAL;
		}
	}
	srcp--;
}

void definition(char *s)	/* collect definition for s and install */
				/* definitions picked up lexically */
{
	char *p;
	Obj *stp;

	p = delimstr("definition");
	stp = lookup(s, 0);
	if (stp != NULL) {	/* it's there before */
		if (stp->type != DEFNAME) {
			ERROR "%s used as variable and definition", s WARNING;
			return;
		}
		free(stp->val);
	} else {
		stp = lookup(s, 1);
		stp->type = DEFNAME;
	}
	stp->val = p;
	dprintf("installing %s as `%s'\n", s, p);
}

char *delimstr(char *s)	/* get body of X ... X */
			/* message if too big */
{
	int c, delim, rdelim, n, deep;
	static char *buf = NULL;
	static int nbuf = 0;
	char *p;

	if (buf == NULL)
		buf = grow(buf, "buf", nbuf += 1000, sizeof(buf[0]));
	while ((delim = input()) == ' ' || delim == '\t' || delim == '\n')
		;
	rdelim = baldelim(delim, "{}");		/* could be "(){}[]`'" */
	deep = 1;
	for (p = buf; ; ) {
		c = input();
		if (c == rdelim)
			if (--deep == 0)
				break;
		if (c == delim)
			deep++;
		if (p >= buf + nbuf) {
			n = p - buf;
			buf = grow(buf, "buf", nbuf += 1000, sizeof(buf[0]));
			p = buf + n;
		}
		if (c == EOF)
			ERROR "end of file in %s %c %.20s... %c", s, delim, buf, delim FATAL;
		*p++ = c;
	}
	*p = '\0';
	dprintf("delimstr %s %c <%s> %c\n", s, delim, buf, delim);
	return tostring(buf);
}

baldelim(int c, char *s)	/* replace c by balancing entry in s */
{
	for ( ; *s; s += 2)
		if (*s == c)
			return s[1];
	return c;
}

Arg	args[10];	/* argument frames */
Arg	*argfp = args;	/* frame pointer */
int	argcnt;		/* number of arguments seen so far */

void dodef(Obj *stp)	/* collect args and switch input to defn */
{
	int i, len;
	char *p;
	Arg *ap;

	ap = argfp+1;
	if (ap >= args+10)
		ERROR "arguments too deep" FATAL;
	argcnt = 0;
	if (input() != '(')
		ERROR "disaster in dodef" FATAL;
	if (ap->argval == 0)
		ap->argval = malloc(1000);
	for (p = ap->argval; (len = getarg(p)) != -1; p += len) {
		ap->argstk[argcnt++] = p;
		if (input() == ')')
			break;
	}
	for (i = argcnt; i < MAXARGS; i++)
		ap->argstk[i] = "";
	if (dbg)
		for (i = 0; i < argcnt; i++)
			printf("arg %d.%d = <%s>\n", ap-args, i+1, ap->argstk[i]);
	argfp = ap;
	pushsrc(Macro, stp->val);
}

getarg(char *p)	/* pick up single argument, store in p, return length */
{
	int n, c, npar;

	n = npar = 0;
	for ( ;; ) {
		c = input();
		if (c == EOF)
			ERROR "end of file in getarg!" FATAL;
		if (npar == 0 && (c == ',' || c == ')'))
			break;
		if (c == '"')	/* copy quoted stuff intact */
			do {
				*p++ = c;
				n++;
			} while ((c = input()) != '"' && c != EOF);
		else if (c == '(')
			npar++;
		else if (c == ')')
			npar--;
		n++;
		*p++ = c;
	}
	*p = 0;
	unput(c);
	return(n + 1);
}

#define	PBSIZE	2000
char	pbuf[PBSIZE];		/* pushback buffer */
char	*pb	= pbuf-1;	/* next pushed back character */

char	ebuf[200];		/* collect input here for error reporting */
char	*ep	= ebuf;

int	begin	= 0;
extern	int	thru;
extern	Obj	*thrudef;
extern	char	*untilstr;

input(void)
{
	register int c;

	if (thru && begin) {
		do_thru();
		begin = 0;
	}
	c = nextchar();
	dprintf(" <%c>", c);
	if (ep >= ebuf + sizeof ebuf)
		ep = ebuf;
	return *ep++ = c;
}

nextchar(void)
{
	register int c;

  loop:
	switch (srcp->type) {
	case Free:	/* free string */
		free(srcp->sp);
		popsrc();
		goto loop;
	case Thru:	/* end of pushed back line */
		begin = 1;
		popsrc();
		c = '\n';
		break;
	case Char:
		if (pb >= pbuf) {
			c = *pb--;
			popsrc();
			break;
		} else {	/* can't happen? */
			popsrc();
			goto loop;
		}
	case String:
		c = *srcp->sp++;
		if (c == '\0') {
			popsrc();
			goto loop;
		} else {
			if (*srcp->sp == '\0')	/* empty, so pop */
				popsrc();
			break;
		}
	case Macro:
		c = *srcp->sp++;
		if (c == '\0') {
			if (--argfp < args)
				ERROR "argfp underflow" FATAL;
			popsrc();
			goto loop;
		} else if (c == '$' && isdigit(*srcp->sp)) {	/* $3 */
			int n = 0;
			while (isdigit(*srcp->sp))
				n = 10 * n + *srcp->sp++ - '0';
			if (n > 0 && n <= MAXARGS)
				pushsrc(String, argfp->argstk[n-1]);
			goto loop;
		}
		break;
	case File:
		c = getc(curfile->fin);
		if (c == EOF) {
			if (curfile == infile)
				ERROR "end of file inside .G1/.G2" FATAL;
			if (curfile->fin != stdin) {
				fclose(curfile->fin);
				free(curfile->fname);	/* assumes allocated */
			}
			curfile--;
			printf(".lf %d %s\n", curfile->lineno, curfile->fname);
			popsrc();
			thru = 0;	/* chicken out */
			thrudef = 0;
			if (untilstr) {
				free(untilstr);
				untilstr = 0;
			}
			goto loop;
		}
		if (c == '\n')
			curfile->lineno++;
		break;
	}
	return c;
}

void do_thru(void)	/* read one line, make into a macro expansion */
{
	int c, i;
	char *p;
	Arg *ap;

	ap = argfp+1;
	if (ap >= args+10)
		ERROR "arguments too deep" FATAL;
	if (ap->argval == NULL)
		ap->argval = malloc(1000);
	p = ap->argval;
	argcnt = 0;
	c = nextchar();
	if (thru == 0) {	/* end of file was seen, so thru is done */
		unput(c);
		return;
	}
	for ( ; c != '\n' && c != EOF; ) {
		if (c == ' ' || c == '\t') {
			c = nextchar();
			continue;
		}
		if (argcnt >= MAXARGS)
			ERROR "too many fields on input line" FATAL;
		ap->argstk[argcnt++] = p;
		if (c == '"') {
			do {
				*p++ = c;
				if ((c = nextchar()) == '\\') {
					*p++ = c;
					*p++ = nextchar();
					c = nextchar();
				}
			} while (c != '"' && c != '\n' && c != EOF);
			*p++ = '"';
			if (c == '"')
				c = nextchar();
		} else {
			do {
				*p++ = c;
			} while ((c = nextchar())!=' ' && c!='\t' && c!='\n' && c!=',' && c!=EOF);
			if (c == ',')
				c = nextchar();
		}
		*p++ = '\0';
	}
	if (c == EOF)
		ERROR "unexpected end of file in do_thru" FATAL;
	if (argcnt == 0) {	/* ignore blank line */
		pushsrc(Thru, (char *) 0);
		return;
	}
	for (i = argcnt; i < MAXARGS; i++)
		ap->argstk[i] = "";
	if (dbg)
		for (i = 0; i < argcnt; i++)
			printf("arg %d.%d = <%s>\n", ap-args, i+1, ap->argstk[i]);
	if (strcmp(ap->argstk[0], ".G2") == 0) {
		thru = 0;
		thrudef = 0;
		pushsrc(String, "\n.G2\n");
		return;
	}
	if (untilstr && strcmp(ap->argstk[0], untilstr) == 0) {
		thru = 0;
		thrudef = 0;
		free(untilstr);
		untilstr = 0;
		return;
	}
	pushsrc(Thru, (char *) 0);
	dprintf("do_thru pushing back <%s>\n", thrudef->val);
	argfp = ap;
	pushsrc(Macro, thrudef->val);
}

unput(int c)
{
	if (++pb >= pbuf + sizeof pbuf)
		ERROR "pushback overflow" FATAL;
	if (--ep < ebuf)
		ep = ebuf + sizeof(ebuf) - 1;
	*pb = c;
	pushsrc(Char, pb);
	return c;
}

void pbstr(char *s)
{
	pushsrc(String, s);
}

double errcheck(double x, char *s)
{
	extern int errno;

	if (errno == EDOM) {
		errno = 0;
		ERROR "%s argument out of domain", s WARNING;
	} else if (errno == ERANGE) {
		errno = 0;
		ERROR "%s result out of range", s WARNING;
	}
	return x;
}

char	errbuf[200];

void yyerror(char *s)
{
	extern char *cmdname;
	int ern = errno;	/* cause some libraries clobber it */

	if (synerr)
		return;
	fflush(stdout);
	fprintf(stderr, "%s: %s", cmdname, s);
	if (ern > 0) {
		errno = ern;
		perror("???");
	}
	fprintf(stderr, " near %s:%d\n",
		curfile->fname, curfile->lineno+1);
	eprint();
	synerr = 1;
	errno = 0;
}

void eprint(void)	/* try to print context around error */
{
	char *p, *q;

	p = ep - 1;
	if (p > ebuf && *p == '\n')
		p--;
	for ( ; p >= ebuf && *p != '\n'; p--)
		;
	while (*p == '\n')
		p++;
	fprintf(stderr, " context is\n\t");
	for (q=ep-1; q>=p && *q!=' ' && *q!='\t' && *q!='\n'; q--)
		;
	for (; p < q; p++)
		if (isprint(*p))
			putc(*p, stderr);
	fprintf(stderr, " >>> ");
	for (; p < q; p++)
		if (isprint(*p))
			putc(*p, stderr);
	fprintf(stderr, " <<< ");
	while (pb >= pbuf)
		putc(*pb--, stderr);
	fgets(ebuf, sizeof ebuf, curfile->fin);
	fprintf(stderr, "%s", ebuf);
	pbstr("\n.G2\n");	/* safety first */
	ep = ebuf;
}

int yywrap(void) {return 1;}

char	*newfile = 0;		/* filename for file copy */
char	*untilstr = 0;		/* string that terminates a thru */
int	thru	= 0;		/* 1 if copying thru macro */
Obj	*thrudef = 0;		/* macro being used */

void copyfile(char *s)	/* remember file to start reading from */
{
	newfile = s;
}

void copydef(Obj *p)	/* remember macro Obj */
{
	thrudef = p;
}

Obj *copythru(char *s)	/* collect the macro name or body for thru */
{
	Obj *p;
	char *q;

	p = lookup(s, 0);
	if (p != NULL) {
		if (p->type == DEFNAME) {
			p->val = addnewline(p->val);
			return p;
		} else
			ERROR "%s used as define and name", s FATAL;
	}
	/* have to collect the definition */
	pbstr(s);	/* first char is the delimiter */
	q = delimstr("thru body");
	p = lookup("nameless", 1);
	if (p != NULL)
		if (p->val)
			free(p->val);
	p->type = DEFNAME;
	p->val = q;
	p->val = addnewline(p->val);
	dprintf("installing nameless as `%s'\n", p->val);
	return p;
}

char *addnewline(char *p)	/* add newline to end of p */
{
	int n;

	n = strlen(p);
	if (p[n-1] != '\n') {
		p = realloc(p, n+2);
		p[n] = '\n';
		p[n+1] = '\0';
	}
	return p;
}

void copyuntil(char *s)	/* string that terminates a thru */
{
	untilstr = s;
}

void copy(void)	/* begin input from file, etc. */
{
	FILE *fin;

	if (newfile) {
		if ((fin = fopen(newfile, "r")) == NULL)
			ERROR "can't open file %s", newfile FATAL;
		curfile++;
		curfile->fin = fin;
		curfile->fname = tostring(newfile);
		curfile->lineno = 0;
		printf(".lf 1 %s\n", curfile->fname);
		pushsrc(File, curfile->fname);
		newfile = 0;
	}
	if (thrudef) {
		thru = 1;
		begin = 1;	/* wrong place */
	}
}

char	shellbuf[1000], *shellp;

void shell_init(void)	/* set up to interpret a shell command */
{
	fprintf(tfd, "# shell cmd...\n");
	sprintf(shellbuf, "rc -c '");
	shellp = shellbuf + strlen(shellbuf);
}

void shell_text(char *s)	/* add string to command being collected */
{
	/* fprintf(tfd, "#add <%s> to <%s>\n", s, shellbuf); */
	while (*s) {
		if (*s == '\'')	{	/* protect interior quotes */
			*shellp++ = '\'';
			*shellp++ = '\\';
			*shellp++ = '\'';
		}
		*shellp++ = *s++;
	}
}

void shell_exec(void)	/* do it */
{
	/* fprintf(tfd, "# run <%s>\n", shellbuf); */
	*shellp++ = '\'';
	*shellp = '\0';
	system(shellbuf);
}
