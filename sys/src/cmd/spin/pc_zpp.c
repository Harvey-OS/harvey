/***** spin: pc_zpp.c *****/

/* Copyright (c) 1997-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

/* pc_zpp.c is only used in the PC version of Spin                        */
/* it is included to avoid too great a reliance on an external cpp        */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef PC
enum cstate { PLAIN, IN_STRING, IN_QUOTE, S_COMM, COMMENT, E_COMM };

#define MAXNEST	32
#define MAXDEF	128
#define MAXLINE	512
#define GENEROUS 4096

#define debug(x,y)	if (verbose) printf(x,y)

static FILE *outpp /* = stdout */;

static int if_truth[MAXNEST];
static int printing[MAXNEST];
static int if_depth, nr_defs, verbose = 0;
static enum cstate state = PLAIN;
static char Out1[GENEROUS], Out2[GENEROUS];

static struct Defines {
	int exists;
	char *src, *trg;
} d[MAXDEF];

static int process(char *, int, char *);
static int zpp_do(char *);

extern char *emalloc(int);	/* main.c */

static int
do_define(char *p)
{	char *q, *r, *s;

	for (q = p+strlen(p)-1; q > p; q--)
		if (*q == '\n' || *q == '\t' || *q == ' ')
			*q = '\0';
		else
			break;

	q = p + strspn(p, " \t");
	if (!(r = strchr(q, '\t')))
		r = strchr(q, ' ');
	if (!r) { s = ""; goto adddef; }
	s = r + strspn(r, " \t");
	*r = '\0';
	if (strchr(q, '('))
	{	debug("zpp: #define with arguments %s\n", q);
		return 0;
	}
	for (r = q+strlen(q)-1; r > q; r--)
		if (*r == ' ' || *r == '\t')
			*r = '\0';
		else
			break;
	if (nr_defs >= MAXDEF)
	{	debug("zpp: too many #defines (max %d)\n", nr_defs);
		return 0;
	}
	if (strcmp(q, s) != 0)
	{	int j;
adddef:		for (j = 0; j < nr_defs; j++)
			if (!strcmp(d[j].src, q))
				d[j].exists = 0;
		d[nr_defs].src = emalloc(strlen(q)+1);
		d[nr_defs].trg = emalloc(strlen(s)+1);
		strcpy(d[nr_defs].src, q);
		strcpy(d[nr_defs].trg, s);
		d[nr_defs++].exists = 1;
	}
	return 1;
}

static int
isvalid(int c)
{
	return (isalnum(c) || c == '_');
}

static char *
apply(char *p0)
{	char *out, *in1, *in2, *startat;
	int i, j;

	startat = in1 = Out2; strcpy(Out2, p0);
	out = Out1; *out = '\0';

	for (i = nr_defs-1; i >= 0; i--)
	{	if (!d[i].exists) continue;
		j = (int) strlen(d[i].src);
more:		in2 = strstr(startat, d[i].src);
		if (!in2)	/* no more matches */
		{	startat = in1;
			continue;
		}
		if ((in2 == in1 || !isvalid(*(in2-1)))
		&&  (in2+j == '\0' || !isvalid(*(in2+j))))
		{	*in2 = '\0';

			if (strlen(in1)+strlen(d[i].trg)+strlen(in2+j) >= GENEROUS)
			{
				printf("spin: circular macro expansion %s -> %s ?\n",
					d[i].src, d[i].trg);
				return in1;
			}
			strcat(out, in1);
			strcat(out, d[i].trg);
			strcat(out, in2+j);
			if (in1 == Out2)
			{	startat = in1 = Out1;
				out = Out2;
			} else
			{	startat = in1 = Out2;
				out = Out1;
			}
			*out = '\0';
		} else
		{	startat = in2+1;	/* +1 not +j.. */
		}
		goto more; /* recursive defines */
	}
	return in1;
}

static char *
do_common(char *p)
{	char *q, *s;

	q = p + strspn(p, " \t");
	for (s = (q + strlen(q) - 1); s > q; s--)
		if (*s == ' ' || *s == '\t' || *s == '\n')
			*s = '\0';
		else
			break;
	return q;
}

static int
do_undefine(char *p)
{	int i; char *q = do_common(p);

	for (i = 0; i < nr_defs; i++)
		if (!strcmp(d[i].src, q))
			d[i].exists = 0;
	return 1;
}

static char *
check_ifdef(char *p)
{	int i; char *q = do_common(p);

	for (i = 0; i < nr_defs; i++)
		if (d[i].exists
		&&  !strcmp(d[i].src, q))
			return d[i].trg;
	return (char *) 0;
}

static int
do_ifdef(char *p)
{
	if (++if_depth >= MAXNEST)
	{	debug("zpp: too deeply nested (max %d)\n", MAXNEST);
		return 0;
	}
	if_truth[if_depth] = (check_ifdef(p) != (char *)0);
	printing[if_depth] = printing[if_depth-1]&&if_truth[if_depth];

	return 1;
}

static int
do_ifndef(char *p)
{
	if (++if_depth >= MAXNEST)
	{	debug("zpp: too deeply nested (max %d)\n", MAXNEST);
		return 0;
	}
	if_truth[if_depth] = (check_ifdef(p) == (char *)0);
	printing[if_depth] = printing[if_depth-1]&&if_truth[if_depth];

	return 1;
}

static int
is_simple(char *q)
{
	if (!q) return 0;
	if (strcmp(q, "0") == 0)
		if_truth[if_depth] = 0;
	else if (strcmp(q, "1") == 0)
		if_truth[if_depth] = 1;
	else
		return 0;
	return 1;
}

static int
do_if(char *p)
{	char *q = do_common(p);
	if (++if_depth >= MAXNEST)
	{	debug("zpp: too deeply nested (max %d)\n", MAXNEST);
		return 0;
	}
	if (!is_simple(q)
	&&  !is_simple(check_ifdef(q)))
	{	debug("zpp: cannot handle #if %s\n", q);
		return 0;
	}
	printing[if_depth] = printing[if_depth-1]&&if_truth[if_depth];

	return 1;
}

static int
do_else(char *unused)
{
	if_truth[if_depth] = 1-if_truth[if_depth];
	printing[if_depth] = printing[if_depth-1]&&if_truth[if_depth];

	return 1;
}

static int
do_endif(char *p)
{
	if (--if_depth < 0)
	{	debug("zpp: unbalanced #endif %s\n", p);
		return 0;
	}
	return 1;
}

static int
do_include(char *p)
{	char *r, *q;

	q = strchr(p, '<');
	r = strrchr(p, '>');
	if (!q || !r)
	{	q = strchr (p, '\"');
		r = strrchr(p, '\"');
		if (!q || !r || q == r)
		{	debug("zpp: malformed #include %s", p);
			return 0;
	}	}
	*r = '\0';
	return zpp_do(++q);
}

static int
in_comment(char *p)
{	char *q = p;

	for (q = p; *q != '\n' && *q != '\0'; q++)
		switch (state) {
		case PLAIN:
			switch (*q) {
			case  '"': state = IN_STRING; break;
			case '\'': state = IN_QUOTE; break;
			case  '/': state = S_COMM; break;
			case '\\': q++; break;
			}
			break;
		case IN_STRING:
			if (*q == '"') state = PLAIN;
			else if (*q == '\\') q++;
			break;
		case IN_QUOTE:
			if (*q == '\'') state = PLAIN;
			else if (*q == '\\') q++;
			break;
		case S_COMM:
			if (*q == '*')
			{	*(q-1) = *q = ' ';
				state = COMMENT;
			} else if (*q != '/')
				state = PLAIN;
			break;
		case COMMENT:
			state = (*q == '*') ? E_COMM: COMMENT;
			*q = ' ';
			break;
		case E_COMM:
			if (*q == '/')
				state = PLAIN;
			else if (*q != '*')
				state = COMMENT;
			*q = ' ';
			break;
		}
	if (state == S_COMM) state = PLAIN;
	else if (state == E_COMM) state = COMMENT;
	return (state == COMMENT);
}

static int
zpp_do(char *fnm)
{	char buf[2048], buf2[MAXLINE], *p; int n, on;
	FILE *inp; int lno = 0, nw_lno = 0;

	if ((inp = fopen(fnm, "r")) == NULL)
	{	fprintf(stdout, "spin: error, '%s': No such file\n", fnm);
		return 0;	/* 4.1.2 was stderr */
	}
	printing[0] = if_truth[0] = 1;
	fprintf(outpp, "#line %d \"%s\"\n", lno+1, fnm);
	while (fgets(buf, MAXLINE, inp))
	{	lno++; n = (int) strlen(buf);
		on = 0; nw_lno = 0;
		while (n > 2 && buf[n-2] == '\\')
		{	buf[n-2] = '\0';
feedme:			if (!fgets(buf2, MAXLINE, inp))
			{	debug("zpp: unexpected EOF ln %d\n", lno);
				return 0;	/* switch to cpp */
			}
			lno++;
			if (n + (int) strlen(buf2) >= 2048)
			{	debug("zpp: line %d too long\n", lno);
				return 0;
			}
			strcat(buf, buf2);
			n = (int) strlen(buf);
		}
		if (in_comment(&buf[on]))
		{	buf[n-1] = '\0'; /* eat newline */
			on = n-1; nw_lno = 1;
			goto feedme;
		}
		p = buf + strspn(buf, " \t");
		if (nw_lno && *p != '#')
			fprintf(outpp, "#line %d \"%s\"\n", lno, fnm);
		if (*p == '#')
		{	if (!process(p+1, lno+1, fnm))
				return 0;
		} else if (printing[if_depth])
			fprintf(outpp, "%s", apply(buf));
	}
	fclose(inp);
	return 1;
}

int
try_zpp(char *fnm, char *onm)
{	int r;
	if ((outpp = fopen(onm, "w")) == NULL)
		return 0;
	r = zpp_do(fnm);
	fclose(outpp);
	return r;	/* 1 = ok; 0 = use cpp */
}

static struct Directives {
	int len;
	char *directive;
	int (*handler)(char *);
	int interp;
} s[] = {
	{ 6, "define",	 do_define,	1 },
	{ 4, "else",	 do_else,	0 },
	{ 5, "endif",	 do_endif,	0 },
	{ 5, "ifdef",	 do_ifdef,	0 },
	{ 6, "ifndef",   do_ifndef,	0 },
	{ 2, "if",	 do_if,		0 },
	{ 7, "include",  do_include,	1 },
	{ 8, "undefine", do_undefine,	1 },
};

static int
process(char *q, int lno, char *fnm)
{	char *p; int i, r;

	for (p = q; *p; p++)
		if (*p != ' ' && *p != '\t')
			break;
	for (i = 0; i < (int) (sizeof(s)/sizeof(struct Directives)); i++)
		if (!strncmp(s[i].directive, p, s[i].len))
		{	if (s[i].interp
			&&  !printing[if_depth])
				return 1;
			fprintf(outpp, "#line %d \"%s\"\n", lno, fnm);
			r = s[i].handler(p +  s[i].len);
			if (i == 6)	/* include */
				fprintf(outpp, "#line %d \"%s\"\n", lno, fnm);
			return r;
		}
	
	debug("zpp: unrecognized directive: %s", p);
	return 0;
}
#endif
