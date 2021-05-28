/***** spin: spinlex.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include "spin.h"
#include "y.tab.h"

#define MAXINL	16	/* max recursion depth inline fcts */
#define MAXPAR	32	/* max params to an inline call */
#define MAXLEN	512	/* max len of an actual parameter text */

typedef struct IType {
	Symbol *nm;		/* name of the type */
	Lextok *cn;		/* contents */
	Lextok *params;		/* formal pars if any */
	Lextok *rval;		/* variable to assign return value, if any */
	char   **anms;		/* literal text for actual pars */
	char   *prec;		/* precondition for c_code or c_expr */
	int    uiid;		/* unique inline id */
	int    is_expr;		/* c_expr in an ltl formula */
	int    dln, cln;	/* def and call linenr */
	Symbol *dfn, *cfn;	/* def and call filename */
	struct IType *nxt;	/* linked list */
} IType;

typedef struct C_Added {
	Symbol *s;
	Symbol *t;
	Symbol *ival;
	Symbol *fnm;
	int	lno;
	struct C_Added *nxt;
} C_Added;

extern RunList	*X_lst;
extern ProcList	*ready;
extern Symbol	*Fname, *oFname;
extern Symbol	*context, *owner;
extern YYSTYPE	yylval;
extern short	has_last, has_code, has_priority;
extern int	verbose, IArgs, hastrack, separate, in_for;
extern int	implied_semis, ltl_mode, in_seq, par_cnt;

short	has_stack = 0;
int	lineno  = 1;
int	scope_seq[128], scope_level = 0;
char	CurScope[MAXSCOPESZ];
char	yytext[2048];
FILE	*yyin, *yyout;

static C_Added	*c_added, *c_tracked;
static IType	*Inline_stub[MAXINL];
static char	*ReDiRect;
static char	*Inliner[MAXINL], IArg_cont[MAXPAR][MAXLEN];
static unsigned char	in_comment=0;
static int	IArgno = 0, Inlining = -1;
static int	check_name(char *);
static int	last_token = 0;

#define ValToken(x, y)	{ if (in_comment) goto again; \
			yylval = nn(ZN,0,ZN,ZN); \
			yylval->val = x; \
			last_token = y; \
			return y; \
			}

#define SymToken(x, y)	{ if (in_comment) goto again; \
			yylval = nn(ZN,0,ZN,ZN); \
			yylval->sym = x; \
			last_token = y; \
			return y; \
			}

static int  getinline(void);
static void uninline(void);

static int PushBack;
static int PushedBack;
static char pushedback[4096];

static void
push_back(char *s)
{
	if (PushedBack + strlen(s) > 4094)
	{	fatal("select statement too large", 0);
	}
	strcat(pushedback, s);
	PushedBack += strlen(s);
}

static int
Getchar(void)
{	int c;

	if (PushedBack > 0 && PushBack < PushedBack)
	{	c = pushedback[PushBack++];
		if (PushBack == PushedBack)
		{	pushedback[0] = '\0';
			PushBack = PushedBack = 0;
		}
		return c;	/* expanded select statement */
	}
	if (Inlining<0)
	{	do {	c = getc(yyin);
		} while (c == 0);	// ignore null chars
		// eventually there will always be an EOF
	} else
	{	c = getinline();
	}
#if 0
	if (0)
	{	printf("<%c:%d>[%d] ", c, c, Inlining);
	} else
	{	printf("%c", c);
	}
#endif
	return c;
}

static void
Ungetch(int c)
{
	if (PushedBack > 0 && PushBack > 0)
	{	PushBack--;
		return;
	}

	if (Inlining<0)
	{	ungetc(c,yyin);
	} else
	{	uninline();
	}
	if (0)
	{	printf("\n<bs{%d}bs>\n", c);
	}
}

static int
notdollar(int c)
{	return (c != '$' && c != '\n');
}

static int
notquote(int c)
{	return (c != '\"' && c != '\n');
}

int
isalnum_(int c)
{	return (isalnum(c) || c == '_');
}

static int
isalpha_(int c)
{	return isalpha(c);	/* could be macro */
}
       
static int
isdigit_(int c)
{	return isdigit(c);	/* could be macro */
}

static void
getword(int first, int (*tst)(int))
{	int i=0, c;

	yytext[i++]= (char) first;
	while (tst(c = Getchar()))
	{	if (c == EOF)
		{	break;
		}
		yytext[i++] = (char) c;
		if (c == '\\')
		{	c = Getchar();
			yytext[i++] = (char) c;	/* no tst */
	}	}
	yytext[i] = '\0';

	Ungetch(c);
}

static int
follow(int tok, int ifyes, int ifno)
{	int c;

	if ((c = Getchar()) == tok)
	{	return ifyes;
	}
	Ungetch(c);

	return ifno;
}

static IType *seqnames;

static void
def_inline(Symbol *s, int ln, char *ptr, char *prc, Lextok *nms)
{	IType *tmp;
	int  cnt = 0;
	char *nw = (char *) emalloc(strlen(ptr)+1);
	strcpy(nw, ptr);

	for (tmp = seqnames; tmp; cnt++, tmp = tmp->nxt)
		if (!strcmp(s->name, tmp->nm->name))
		{	non_fatal("procedure name %s redefined",
				tmp->nm->name);
			tmp->cn = (Lextok *) nw;
			tmp->params = nms;
			tmp->dln = ln;
			tmp->dfn = Fname;
			return;
		}
	tmp = (IType *) emalloc(sizeof(IType));
	tmp->nm = s;
	tmp->cn = (Lextok *) nw;
	tmp->params = nms;
	if (strlen(prc) > 0)
	{	tmp->prec = (char *) emalloc(strlen(prc)+1);
		strcpy(tmp->prec, prc);
	}
	tmp->dln = ln;
	tmp->dfn = Fname;
	tmp->uiid = cnt+1;	/* so that 0 means: not an inline */
	tmp->nxt = seqnames;
	seqnames = tmp;
}

void
gencodetable(FILE *fd)
{	IType *tmp;
	char *q;
	int cnt;

	if (separate == 2) return;

	fprintf(fd, "struct {\n");
	fprintf(fd, "	char *c; char *t;\n");
	fprintf(fd, "} code_lookup[] = {\n");

	if (has_code)
	for (tmp = seqnames; tmp; tmp = tmp->nxt)
		if (tmp->nm->type == CODE_FRAG
		||  tmp->nm->type == CODE_DECL)
		{	fprintf(fd, "\t{ \"%s\", ",
				tmp->nm->name);
			q = (char *) tmp->cn;

			while (*q == '\n' || *q == '\r' || *q == '\\')
				q++;

			fprintf(fd, "\"");
			cnt = 0;
			while (*q && cnt < 1024) /* pangen1.h allows 2048 */
			{	switch (*q) {
				case '"':
					fprintf(fd, "\\\"");
					break;
				case '%':
					fprintf(fd, "%%");
					break;
				case '\n':
					fprintf(fd, "\\n");
					break;
				default:
					putc(*q, fd);
					break;
				}
				q++; cnt++;
			}
			if (*q) fprintf(fd, "...");
			fprintf(fd, "\"");
			fprintf(fd, " },\n");
		}

	fprintf(fd, "	{ (char *) 0, \"\" }\n");
	fprintf(fd, "};\n");
}

static int
iseqname(char *t)
{	IType *tmp;

	for (tmp = seqnames; tmp; tmp = tmp->nxt)
	{	if (!strcmp(t, tmp->nm->name))
			return 1;
	}
	return 0;
}

Lextok *
return_statement(Lextok *n)
{
	if (Inline_stub[Inlining]->rval)
	{	Lextok *g = nn(ZN, NAME, ZN, ZN);
		Lextok *h = Inline_stub[Inlining]->rval;
		g->sym = lookup("rv_");
		return nn(h, ASGN, h, n);
	} else
	{	fatal("return statement outside inline", (char *) 0);
	}
	return ZN;
}

static int
getinline(void)
{	int c;

	if (ReDiRect)
	{	c = *ReDiRect++;
		if (c == '\0')
		{	ReDiRect = (char *) 0;
			c = *Inliner[Inlining]++;
		}
	} else
	{
		c = *Inliner[Inlining]++;
	}

	if (c == '\0')
	{
		lineno = Inline_stub[Inlining]->cln;
		Fname  = Inline_stub[Inlining]->cfn;
		Inlining--;

#if 0
		if (verbose&32)
		printf("spin: %s:%d, done inlining %s\n",
			Fname, lineno, Inline_stub[Inlining+1]->nm->name);
#endif
		return Getchar();
	}
	return c;
}

static void
uninline(void)
{
	if (ReDiRect)
		ReDiRect--;
	else
		Inliner[Inlining]--;
}

int
is_inline(void)
{
	if (Inlining < 0)
		return 0;	/* i.e., not an inline */
	if (Inline_stub[Inlining] == NULL)
		fatal("unexpected, inline_stub not set", 0);
	return Inline_stub[Inlining]->uiid;
}

IType *
find_inline(char *s)
{	IType *tmp;

	for (tmp = seqnames; tmp; tmp = tmp->nxt)
		if (!strcmp(s, tmp->nm->name))
			break;
	if (!tmp)
		fatal("cannot happen, missing inline def %s", s);

	return tmp;
}

void
c_state(Symbol *s, Symbol *t, Symbol *ival)	/* name, scope, ival */
{	C_Added *r;

	r = (C_Added *) emalloc(sizeof(C_Added));
	r->s = s;	/* pointer to a data object */
	r->t = t;	/* size of object, or "global", or "local proctype_name"  */
	r->ival = ival;
	r->lno = lineno;
	r->fnm = Fname;
	r->nxt = c_added;

	if(strncmp(r->s->name, "\"unsigned unsigned", 18) == 0)
	{	int i;
		for (i = 10; i < 18; i++)
		{	r->s->name[i] = ' ';
		}
	/*	printf("corrected <%s>\n", r->s->name);	*/
	}
	c_added = r;
}

void
c_track(Symbol *s, Symbol *t, Symbol *stackonly)	/* name, size */
{	C_Added *r;

	r = (C_Added *) emalloc(sizeof(C_Added));
	r->s = s;
	r->t = t;
	r->ival = stackonly;	/* abuse of name */
	r->nxt = c_tracked;
	r->fnm = Fname;
	r->lno = lineno;
	c_tracked = r;

	if (stackonly != ZS)
	{	if (strcmp(stackonly->name, "\"Matched\"") == 0)
			r->ival = ZS;	/* the default */
		else if (strcmp(stackonly->name, "\"UnMatched\"") != 0
		     &&  strcmp(stackonly->name, "\"unMatched\"") != 0
		     &&  strcmp(stackonly->name, "\"StackOnly\"") != 0)
			non_fatal("expecting '[Un]Matched', saw %s", stackonly->name);
		else
			has_stack = 1;	/* unmatched stack */
	}
}

char *
skip_white(char *p)
{
	if (p != NULL)
	{	while (*p == ' ' || *p == '\t')
			p++;
	} else
	{	fatal("bad format - 1", (char *) 0);
	}
	return p;
}

char *
skip_nonwhite(char *p)
{
	if (p != NULL)
	{	while (*p != ' ' && *p != '\t')
			p++;
	} else
	{	fatal("bad format - 2", (char *) 0);
	}
	return p;
}

static char *
jump_etc(C_Added *r)
{	char *op = r->s->name;
	char *p = op;
	char *q = (char *) 0;
	int oln = lineno;
	Symbol *ofnm = Fname;

	/* try to get the type separated from the name */
	lineno = r->lno;
	Fname  = r->fnm;

	p = skip_white(p);	/* initial white space */

	if (strncmp(p, "enum", strlen("enum")) == 0) /* special case: a two-part typename */
	{	p += strlen("enum")+1;
		p = skip_white(p);
	}
	if (strncmp(p, "unsigned", strlen("unsigned")) == 0) /* possibly a two-part typename */
	{	p += strlen("unsigned")+1;
		q = p = skip_white(p);
	}
	p = skip_nonwhite(p);	/* type name */
	p = skip_white(p);	/* white space */
	while (*p == '*') p++;	/* decorations */
	p = skip_white(p);	/* white space */

	if (*p == '\0')
	{	if (q)
		{	p = q;	/* unsigned with implied 'int' */
		} else
		{	fatal("c_state format (%s)", op);
	}	}

	if (strchr(p, '[')
	&& (!r->ival
	||  !r->ival->name
	||  !strchr(r->ival->name, '{')))	/* was !strchr(p, '{')) */
	{	non_fatal("array initialization error, c_state (%s)", p);
		p = (char *) 0;
	}

	lineno = oln;
	Fname = ofnm;

	return p;
}

void
c_add_globinit(FILE *fd)
{	C_Added *r;
	char *p, *q;

	fprintf(fd, "void\nglobinit(void)\n{\n");
	for (r = c_added; r; r = r->nxt)
	{	if (r->ival == ZS)
			continue;

		if (strncmp(r->t->name, " Global ", strlen(" Global ")) == 0)
		{	for (q = r->ival->name; *q; q++)
			{	if (*q == '\"')
					*q = ' ';
				if (*q == '\\')
					*q++ = ' '; /* skip over the next */
			}
			p = jump_etc(r);	/* e.g., "int **q" */
			if (p)
			fprintf(fd, "	now.%s = %s;\n", p, r->ival->name);

		} else
		if (strncmp(r->t->name, " Hidden ", strlen(" Hidden ")) == 0)
		{	for (q = r->ival->name; *q; q++)
			{	if (*q == '\"')
					*q = ' ';
				if (*q == '\\')
					*q++ = ' '; /* skip over the next */
			}
			p = jump_etc(r);	/* e.g., "int **q" */
			if (p)
			fprintf(fd, "	%s = %s;\n", p, r->ival->name);	/* no now. prefix */

	}	}
	fprintf(fd, "}\n");
}

void
c_add_locinit(FILE *fd, int tpnr, char *pnm)
{	C_Added *r;
	char *p, *q, *s;
	int frst = 1;

	fprintf(fd, "void\nlocinit%d(int h)\n{\n", tpnr);
	for (r = c_added; r; r = r->nxt)
		if (r->ival != ZS
		&&  strncmp(r->t->name, " Local", strlen(" Local")) == 0)
		{	for (q = r->ival->name; *q; q++)
				if (*q == '\"')
					*q = ' ';
			p = jump_etc(r);	/* e.g., "int **q" */

			q = r->t->name + strlen(" Local");
			while (*q == ' ' || *q == '\t')
				q++;			/* process name */

			s = (char *) emalloc(strlen(q)+1);
			strcpy(s, q);

			q = &s[strlen(s)-1];
			while (*q == ' ' || *q == '\t')
				*q-- = '\0';

			if (strcmp(pnm, s) != 0)
				continue;

			if (frst)
			{	fprintf(fd, "\tuchar *_this = pptr(h);\n");
				frst = 0;
			}

			if (p)
			{	fprintf(fd, "\t\t((P%d *)_this)->%s = %s;\n",
					tpnr, p, r->ival->name);
			}
		}
	fprintf(fd, "}\n");
}

/* tracking:
	1. for non-global and non-local c_state decls: add up all the sizes in c_added
	2. add a global char array of that size into now
	3. generate a routine that memcpy's the required values into that array
	4. generate a call to that routine
 */

void
c_preview(void)
{	C_Added *r;

	hastrack = 0;
	if (c_tracked)
		hastrack = 1;
	else
	for (r = c_added; r; r = r->nxt)
		if (strncmp(r->t->name, " Global ", strlen(" Global ")) != 0
		&&  strncmp(r->t->name, " Hidden ", strlen(" Hidden ")) != 0
		&&  strncmp(r->t->name, " Local",  strlen(" Local"))  != 0)
		{	hastrack = 1;	/* c_state variant now obsolete */
			break;
		}
}

int
c_add_sv(FILE *fd)	/* 1+2 -- called in pangen1.c */
{	C_Added *r;
	int cnt = 0;

	if (!c_added && !c_tracked) return 0;

	for (r = c_added; r; r = r->nxt)	/* pickup global decls */
		if (strncmp(r->t->name, " Global ", strlen(" Global ")) == 0)
			fprintf(fd, "	%s;\n", r->s->name);

	for (r = c_added; r; r = r->nxt)
		if (strncmp(r->t->name, " Global ", strlen(" Global ")) != 0
		&&  strncmp(r->t->name, " Hidden ", strlen(" Hidden ")) != 0
		&&  strncmp(r->t->name, " Local",  strlen(" Local"))  != 0)
		{	cnt++;	/* obsolete use */
		}

	for (r = c_tracked; r; r = r->nxt)
		cnt++;		/* preferred use */

	if (cnt == 0) return 0;

	cnt = 0;
	fprintf(fd, "	uchar c_state[");
	for (r = c_added; r; r = r->nxt)
		if (strncmp(r->t->name, " Global ", strlen(" Global ")) != 0
		&&  strncmp(r->t->name, " Hidden ", strlen(" Hidden ")) != 0
		&&  strncmp(r->t->name, " Local",  strlen(" Local"))  != 0)
		{	fprintf(fd, "%ssizeof(%s)",
				(cnt==0)?"":"+", r->t->name);
			cnt++;
		}

	for (r = c_tracked; r; r = r->nxt)
	{	if (r->ival != ZS) continue;

		fprintf(fd, "%s%s",
			(cnt==0)?"":"+", r->t->name);
		cnt++;
	}

	if (cnt == 0) fprintf(fd, "4");	/* now redundant */
	fprintf(fd, "];\n");
	return 1;
}

void
c_stack_size(FILE *fd)
{	C_Added *r;
	int cnt = 0;

	for (r = c_tracked; r; r = r->nxt)
		if (r->ival != ZS)
		{	fprintf(fd, "%s%s",
				(cnt==0)?"":"+", r->t->name);
			cnt++;
		}
	if (cnt == 0)
	{	fprintf(fd, "WS");
	}
}

void
c_add_stack(FILE *fd)
{	C_Added *r;
	int cnt = 0;

	if ((!c_added && !c_tracked) || !has_stack)
	{	return;
	}

	for (r = c_tracked; r; r = r->nxt)
		if (r->ival != ZS)
		{	cnt++;
		}

	if (cnt > 0)
	{	fprintf(fd, "	uchar c_stack[StackSize];\n");
	}
}

void
c_add_hidden(FILE *fd)
{	C_Added *r;

	for (r = c_added; r; r = r->nxt)	/* pickup hidden decls */
		if (strncmp(r->t->name, "\"Hidden\"", strlen("\"Hidden\"")) == 0)
		{	r->s->name[strlen(r->s->name)-1] = ' ';
			fprintf(fd, "%s;	/* Hidden */\n", &r->s->name[1]);
			r->s->name[strlen(r->s->name)-1] = '"';
		}
	/* called before c_add_def - quotes are still there */
}

void
c_add_loc(FILE *fd, char *s)	/* state vector entries for proctype s */
{	C_Added *r;
	static char buf[1024];
	char *p;

	if (!c_added) return;

	strcpy(buf, s);
	strcat(buf, " ");
	for (r = c_added; r; r = r->nxt)	/* pickup local decls */
	{	if (strncmp(r->t->name, " Local", strlen(" Local")) == 0)
		{	p = r->t->name + strlen(" Local");
fprintf(fd, "/* XXX p=<%s>, s=<%s>, buf=<%s> r->s->name=<%s>XXX */\n", p, s, buf, r->s->name);
			while (*p == ' ' || *p == '\t')
			{	p++;
			}
			if (strcmp(p, buf) == 0
			||  (strncmp(p, "init", 4) == 0 && strncmp(buf, ":init:", 6) == 0))
			{	fprintf(fd, "	%s;\n", r->s->name);
	}	}	}
}
void
c_add_def(FILE *fd)	/* 3 - called in plunk_c_fcts() */
{	C_Added *r;

	fprintf(fd, "#if defined(C_States) && (HAS_TRACK==1)\n");
	for (r = c_added; r; r = r->nxt)
	{	r->s->name[strlen(r->s->name)-1] = ' ';
		r->s->name[0] = ' '; /* remove the "s */

		r->t->name[strlen(r->t->name)-1] = ' ';
		r->t->name[0] = ' ';

		if (strncmp(r->t->name, " Global ", strlen(" Global ")) == 0
		||  strncmp(r->t->name, " Hidden ", strlen(" Hidden ")) == 0
		||  strncmp(r->t->name, " Local",  strlen(" Local"))  == 0)
			continue;

		if (strchr(r->s->name, '&'))
			fatal("dereferencing state object: %s", r->s->name);

		fprintf(fd, "extern %s %s;\n", r->t->name, r->s->name);
	}
	for (r = c_tracked; r; r = r->nxt)
	{	r->s->name[strlen(r->s->name)-1] = ' ';
		r->s->name[0] = ' '; /* remove " */

		r->t->name[strlen(r->t->name)-1] = ' ';
		r->t->name[0] = ' ';
	}

	if (separate == 2)
	{	fprintf(fd, "#endif\n");
		return;
	}

	if (has_stack)
	{	fprintf(fd, "int cpu_printf(const char *, ...);\n");
		fprintf(fd, "void\nc_stack(uchar *p_t_r)\n{\n");
		fprintf(fd, "#ifdef VERBOSE\n");
		fprintf(fd, "	cpu_printf(\"c_stack %%u\\n\", p_t_r);\n");
		fprintf(fd, "#endif\n");
		for (r = c_tracked; r; r = r->nxt)
		{	if (r->ival == ZS) continue;
	
			fprintf(fd, "\tif(%s)\n", r->s->name);
			fprintf(fd, "\t\tmemcpy(p_t_r, %s, %s);\n",
				r->s->name, r->t->name);
			fprintf(fd, "\telse\n");
			fprintf(fd, "\t\tmemset(p_t_r, 0, %s);\n",
				r->t->name);
			fprintf(fd, "\tp_t_r += %s;\n", r->t->name);
		}
		fprintf(fd, "}\n\n");
	}

	fprintf(fd, "void\nc_update(uchar *p_t_r)\n{\n");
	fprintf(fd, "#ifdef VERBOSE\n");
	fprintf(fd, "	printf(\"c_update %%p\\n\", p_t_r);\n");
	fprintf(fd, "#endif\n");
	for (r = c_added; r; r = r->nxt)
	{	if (strncmp(r->t->name, " Global ", strlen(" Global ")) == 0
		||  strncmp(r->t->name, " Hidden ", strlen(" Hidden ")) == 0
		||  strncmp(r->t->name, " Local",  strlen(" Local"))  == 0)
			continue;

		fprintf(fd, "\tmemcpy(p_t_r, &%s, sizeof(%s));\n",
			r->s->name, r->t->name);
		fprintf(fd, "\tp_t_r += sizeof(%s);\n", r->t->name);
	}

	for (r = c_tracked; r; r = r->nxt)
	{	if (r->ival) continue;

		fprintf(fd, "\tif(%s)\n", r->s->name);
		fprintf(fd, "\t\tmemcpy(p_t_r, %s, %s);\n",
			r->s->name, r->t->name);
		fprintf(fd, "\telse\n");
		fprintf(fd, "\t\tmemset(p_t_r, 0, %s);\n",
			r->t->name);
		fprintf(fd, "\tp_t_r += %s;\n", r->t->name);
	}

	fprintf(fd, "}\n");

	if (has_stack)
	{	fprintf(fd, "void\nc_unstack(uchar *p_t_r)\n{\n");
		fprintf(fd, "#ifdef VERBOSE\n");
		fprintf(fd, "	cpu_printf(\"c_unstack %%u\\n\", p_t_r);\n");
		fprintf(fd, "#endif\n");
		for (r = c_tracked; r; r = r->nxt)
		{	if (r->ival == ZS) continue;

			fprintf(fd, "\tif(%s)\n", r->s->name);
			fprintf(fd, "\t\tmemcpy(%s, p_t_r, %s);\n",
				r->s->name, r->t->name);
			fprintf(fd, "\tp_t_r += %s;\n", r->t->name);
		}
		fprintf(fd, "}\n");
	}

	fprintf(fd, "void\nc_revert(uchar *p_t_r)\n{\n");
	fprintf(fd, "#ifdef VERBOSE\n");
	fprintf(fd, "	printf(\"c_revert %%p\\n\", p_t_r);\n");
	fprintf(fd, "#endif\n");
	for (r = c_added; r; r = r->nxt)
	{	if (strncmp(r->t->name, " Global ", strlen(" Global ")) == 0
		||  strncmp(r->t->name, " Hidden ", strlen(" Hidden ")) == 0
		||  strncmp(r->t->name, " Local",  strlen(" Local"))  == 0)
			continue;

		fprintf(fd, "\tmemcpy(&%s, p_t_r, sizeof(%s));\n",
			r->s->name, r->t->name);
		fprintf(fd, "\tp_t_r += sizeof(%s);\n", r->t->name);
	}
	for (r = c_tracked; r; r = r->nxt)
	{	if (r->ival != ZS) continue;

		fprintf(fd, "\tif(%s)\n", r->s->name);
		fprintf(fd, "\t\tmemcpy(%s, p_t_r, %s);\n",
			r->s->name, r->t->name);
		fprintf(fd, "\tp_t_r += %s;\n", r->t->name);
	}

	fprintf(fd, "}\n");
	fprintf(fd, "#endif\n");
}

void
plunk_reverse(FILE *fd, IType *p, int matchthis)
{	char *y, *z;

	if (!p) return;
	plunk_reverse(fd, p->nxt, matchthis);

	if (!p->nm->context
	&&   p->nm->type == matchthis
	&&   p->is_expr == 0)
	{	fprintf(fd, "\n/* start of %s */\n", p->nm->name);
		z = (char *) p->cn;
		while (*z == '\n' || *z == '\r' || *z == '\\')
		{	z++;
		}
		/* e.g.: \#include "..." */

		y = z;
		while ((y = strstr(y, "\\#")) != NULL)
		{	*y = '\n'; y++;
		}

		fprintf(fd, "%s\n", z);
		fprintf(fd, "\n/* end of %s */\n", p->nm->name);
	}
}

void
plunk_c_decls(FILE *fd)
{
	plunk_reverse(fd, seqnames, CODE_DECL);
}

void
plunk_c_fcts(FILE *fd)
{
	if (separate == 2 && hastrack)
	{	c_add_def(fd);
		return;
	}

	c_add_hidden(fd);
	plunk_reverse(fd, seqnames, CODE_FRAG);

	if (c_added || c_tracked)	/* enables calls to c_revert and c_update */
		fprintf(fd, "#define C_States	1\n");
	else
		fprintf(fd, "#undef C_States\n");

	if (hastrack)
		c_add_def(fd);

	c_add_globinit(fd);
	do_locinits(fd);
}

static void
check_inline(IType *tmp)
{	char buf[128];
	ProcList *p;

	if (!X_lst) return;

	for (p = ready; p; p = p->nxt)
	{	if (strcmp(p->n->name, X_lst->n->name) == 0)
			continue;
		sprintf(buf, "P%s->", p->n->name);
		if (strstr((char *)tmp->cn, buf))
		{	printf("spin: in proctype %s, ref to object in proctype %s\n",
				X_lst->n->name, p->n->name);
			fatal("invalid variable ref in '%s'", tmp->nm->name);
	}	}
}

extern short terse;
extern short nocast;

void
plunk_expr(FILE *fd, char *s)
{	IType *tmp;
	char *q;

	tmp = find_inline(s);
	check_inline(tmp);

	if (terse && nocast)
	{	for (q = (char *) tmp->cn; q && *q != '\0'; q++)
		{	fflush(fd);
			if (*q == '"')
			{	fprintf(fd, "\\");
			}
			fprintf(fd, "%c", *q);
		}
	} else
	{	fprintf(fd, "%s", (char *) tmp->cn);
	}
}

void
preruse(FILE *fd, Lextok *n)	/* check a condition for c_expr with preconditions */
{	IType *tmp;

	if (!n) return;
	if (n->ntyp == C_EXPR)
	{	tmp = find_inline(n->sym->name);
		if (tmp->prec)
		{	fprintf(fd, "if (!(%s)) { if (!readtrail) { depth++; ", tmp->prec);
			fprintf(fd, "trpt++; trpt->pr = II; trpt->o_t = t; trpt->st = tt; ");
			fprintf(fd, "uerror(\"c_expr line %d precondition false: %s\"); continue;",
				tmp->dln, tmp->prec);
			fprintf(fd, " } else { printf(\"pan: precondition false: %s\\n\"); ",
				tmp->prec);
			fprintf(fd, "_m = 3; goto P999; } } \n\t\t");
		}
	} else
	{	preruse(fd, n->rgt);
		preruse(fd, n->lft);
	}
}

int
glob_inline(char *s)
{	IType *tmp;
	char *bdy;

	tmp = find_inline(s);
	bdy = (char *) tmp->cn;
	return (strstr(bdy, "now.")		/* global ref or   */
	||      strchr(bdy, '(') > bdy);	/* possible C-function call */
}

char *
put_inline(FILE *fd, char *s)
{	IType *tmp;

	tmp = find_inline(s);
	check_inline(tmp);
	return (char *) tmp->cn;
}

void
mark_last(void)
{
	if (seqnames)
	{	seqnames->is_expr = 1;
	}
}

void
plunk_inline(FILE *fd, char *s, int how, int gencode)	/* c_code with precondition */
{	IType *tmp;

	tmp = find_inline(s);
	check_inline(tmp);

	fprintf(fd, "{ ");
	if (how && tmp->prec)
	{	fprintf(fd, "if (!(%s)) { if (!readtrail) {",
			tmp->prec);
		fprintf(fd, " uerror(\"c_code line %d precondition false: %s\"); continue; ",
			tmp->dln,
			tmp->prec);
		fprintf(fd, "} else { ");
		fprintf(fd, "printf(\"pan: precondition false: %s\\n\"); _m = 3; goto P999; } } ",
			tmp->prec);
	}

	if (!gencode)	/* not in d_step */
	{	fprintf(fd, "\n\t\tsv_save();");
	}

	fprintf(fd, "%s", (char *) tmp->cn);
	fprintf(fd, " }\n");
}

int
side_scan(char *t, char *pat)
{	char *r = strstr(t, pat);
	return (r
		&& *(r-1) != '"'
		&& *(r-1) != '\'');
}

void
no_side_effects(char *s)
{	IType *tmp;
	char *t;
	char *z;

	/* could still defeat this check via hidden
	 * side effects in function calls,
	 * but this will catch at least some cases
	 */

	tmp = find_inline(s);
	t = (char *) tmp->cn;
	while (t && *t == ' ')
	{	t++;
	}

	z = strchr(t, '(');
	if (z
	&&  z > t
	&&  isalnum((int) *(z-1))
	&&  strncmp(t, "spin_mutex_free(", strlen("spin_mutex_free(")) != 0)
	{	goto bad;	/* fct call */
	}

	if (side_scan(t, ";")
	||  side_scan(t, "++")
	||  side_scan(t, "--"))
	{
bad:		lineno = tmp->dln;
		Fname = tmp->dfn;
		non_fatal("c_expr %s has side-effects", s);
		return;
	}
	while ((t = strchr(t, '=')) != NULL)
	{	if (*(t-1) == '!'
		||  *(t-1) == '>'
		||  *(t-1) == '<'
		||  *(t-1) == '"'
		||  *(t-1) == '\'')
		{	t += 2;
			continue;
		}
		t++;
		if (*t != '=')
			goto bad;
		t++;
	}
}

void
pickup_inline(Symbol *t, Lextok *apars, Lextok *rval)
{	IType *tmp; Lextok *p, *q; int j;

	tmp = find_inline(t->name);

	if (++Inlining >= MAXINL)
		fatal("inlines nested too deeply", 0);
	tmp->cln = lineno;	/* remember calling point */
	tmp->cfn = Fname;	/* and filename */
	tmp->rval = rval;

	for (p = apars, q = tmp->params, j = 0; p && q; p = p->rgt, q = q->rgt)
		j++; /* count them */
	if (p || q)
		fatal("wrong nr of params on call of '%s'", t->name);

	tmp->anms  = (char **) emalloc(j * sizeof(char *));
	for (p = apars, j = 0; p; p = p->rgt, j++)
	{	tmp->anms[j] = (char *) emalloc(strlen(IArg_cont[j])+1);
		strcpy(tmp->anms[j], IArg_cont[j]);
	}

	lineno = tmp->dln;	/* linenr of def */
	Fname = tmp->dfn;	/* filename of same */
	Inliner[Inlining] = (char *)tmp->cn;
	Inline_stub[Inlining] = tmp;
#if 0
	if (verbose&32)
	printf("spin: %s:%d, inlining '%s' (from %s:%d)\n",
		tmp->cfn->name, tmp->cln, t->name, tmp->dfn->name, tmp->dln);
#endif
	for (j = 0; j < Inlining; j++)
	{	if (Inline_stub[j] == Inline_stub[Inlining])
		{	fatal("cyclic inline attempt on: %s", t->name);
	}	}
	last_token = SEMI;	/* avoid insertion of extra semi */
}

extern int pp_mode;

static void
do_directive(int first)
{	int c = first;	/* handles lines starting with pound */

	getword(c, isalpha_);

	if (strcmp(yytext, "#ident") == 0)
		goto done;

	if ((c = Getchar()) != ' ')
		fatal("malformed preprocessor directive - # .", 0);

	if (!isdigit_(c = Getchar()))
		fatal("malformed preprocessor directive - # .lineno", 0);

	getword(c, isdigit_);
	lineno = atoi(yytext);	/* pickup the line number */

	if ((c = Getchar()) == '\n')
		return;	/* no filename */

	if (c != ' ')
		fatal("malformed preprocessor directive - .fname", 0);

	if ((c = Getchar()) != '\"')
	{	printf("got %c, expected \" -- lineno %d\n", c, lineno);
		fatal("malformed preprocessor directive - .fname (%s)", yytext);
	}

	getword(Getchar(), notquote);	/* was getword(c, notquote); */
	if (Getchar() != '\"')
		fatal("malformed preprocessor directive - fname.", 0);

	/* strcat(yytext, "\""); */
	Fname = lookup(yytext);
done:
	while (Getchar() != '\n')
		;
}

void
precondition(char *q)
{	int c, nest = 1;

	for (;;)
	{	c = Getchar();
		*q++ = c;
		switch (c) {
		case '\n':
			lineno++;
			break;
		case '[':
			nest++;
			break;
		case ']':
			if (--nest <= 0)
			{	*--q = '\0';
				return;
			}
			break;
		}
	}
	fatal("cannot happen", (char *) 0); /* unreachable */
}


Symbol *
prep_inline(Symbol *s, Lextok *nms)
{	int c, nest = 1, dln, firstchar, cnr;
	char *p;
	Lextok *t;
	static char Buf1[SOMETHINGBIG], Buf2[RATHERSMALL];
	static int c_code = 1;

	for (t = nms; t; t = t->rgt)
		if (t->lft)
		{	if (t->lft->ntyp != NAME)
			fatal("bad param to inline %s", s?s->name:"--");
			t->lft->sym->hidden |= 32;
		}

	if (!s)	/* C_Code fragment */
	{	s = (Symbol *) emalloc(sizeof(Symbol));
		s->name = (char *) emalloc(strlen("c_code")+26);
		sprintf(s->name, "c_code%d", c_code++);
		s->context = context;
		s->type = CODE_FRAG;
	} else
	{	s->type = PREDEF;
	}

	p = &Buf1[0];
	Buf2[0] = '\0';
	for (;;)
	{	c = Getchar();
		switch (c) {
		case '[':
			if (s->type != CODE_FRAG)
				goto bad;
			precondition(&Buf2[0]);	/* e.g., c_code [p] { r = p-r; } */
			continue;
		case '{':
			break;
		case '\n':
			lineno++;
			/* fall through */
		case ' ': case '\t': case '\f': case '\r':
			continue;
		default :
			 printf("spin: saw char '%c'\n", c);
bad:			 fatal("bad inline: %s", s->name);
		}
		break;
	}
	dln = lineno;
	if (s->type == CODE_FRAG)
	{	if (verbose&32)
		{	sprintf(Buf1, "\t/* line %d %s */\n\t\t",
				lineno, Fname->name);
		} else
		{	strcpy(Buf1, "");
		}
	} else
	{	sprintf(Buf1, "\n#line %d \"%s\"\n{", lineno, Fname->name);
	}
	p += strlen(Buf1);
	firstchar = 1;

	cnr = 1; /* not zero */
more:
	c = Getchar();
	*p++ = (char) c;
	if (p - Buf1 >= SOMETHINGBIG)
		fatal("inline text too long", 0);
	switch (c) {
	case '\n':
		lineno++;
		cnr = 0;
		break;
	case '{':
		cnr++;
		nest++;
		break;
	case '}':
		cnr++;
		if (--nest <= 0)
		{	*p = '\0';
			if (s->type == CODE_FRAG)
			{	*--p = '\0';	/* remove trailing '}' */
			}	
			def_inline(s, dln, &Buf1[0], &Buf2[0], nms);
			if (firstchar)
			{	printf("%3d: %s, warning: empty inline definition (%s)\n",
					dln, Fname->name, s->name);
			}
			return s;	/* normal return */
		}
		break;
	case '#':
		if (cnr == 0)
		{	p--;
			do_directive(c); /* reads to newline */
		} else
		{	firstchar = 0;
			cnr++;
		}
		break;
	case '\t':
	case ' ':
	case '\f':
		cnr++;
		break;
	case '"':
		do {
			c = Getchar();
			*p++ = (char) c;
			if (c == '\\')
			{	*p++ = (char) Getchar();
			}
			if (p - Buf1 >= SOMETHINGBIG)
			{	fatal("inline text too long", 0);
			}
		} while (c != '"');	/* end of string */
		/* *p = '\0'; */
		break;
	case '\'':
		c = Getchar();
		*p++ = (char) c;
		if (c == '\\')
		{	*p++ = (char) Getchar();
		}
		c = Getchar();
		*p++ = (char) c;
		assert(c == '\'');
		break;
	default:
		firstchar = 0;
		cnr++;
		break;
	}
	goto more;
}

static void
set_cur_scope(void)
{	int i;
	char tmpbuf[256];

	strcpy(CurScope, "_");

	if (context)
	for (i = 0; i < scope_level; i++)
	{	sprintf(tmpbuf, "%d_", scope_seq[i]);
		strcat(CurScope, tmpbuf);
	}
}

static int
pre_proc(void)
{	char b[512];
	int c, i = 0;

	b[i++] = '#';
	while ((c = Getchar()) != '\n' && c != EOF)
	{	b[i++] = (char) c;
	}
	b[i] = '\0';
	yylval = nn(ZN, 0, ZN, ZN);
	yylval->sym = lookup(b);
	return PREPROC;
}

static int specials[] = {
	'}', ')', ']',
	OD, FI, ELSE, BREAK,
	C_CODE, C_EXPR, C_DECL,
	NAME, CONST, INCR, DECR, 0
};

int
follows_token(int c)
{	int i;

	for (i = 0; specials[i]; i++)
	{	if (c == specials[i])
		{	return 1;
	}	}
	return 0;
}
#define DEFER_LTL
#ifdef DEFER_LTL
/* defer ltl formula to the end of the spec
 * no matter where they appear in the original
 */

static int deferred = 0;
static FILE *defer_fd;

int
get_deferred(void)
{
	if (!defer_fd)
	{	return 0;	/* nothing was deferred */
	}
	fclose(defer_fd);

	defer_fd = fopen(TMP_FILE2, "r");
	if (!defer_fd)
	{	non_fatal("cannot retrieve deferred ltl formula", (char *) 0);
		return 0;
	}
	fclose(yyin);
	yyin = defer_fd;
	return 1;
}

void
zap_deferred(void)
{
	(void) unlink(TMP_FILE2);
}

int
put_deferred(void)
{	int c, cnt;
	if (!defer_fd)
	{	defer_fd = fopen(TMP_FILE2, "w+");
		if (!defer_fd)
		{	non_fatal("cannot defer ltl expansion", (char *) 0);
			return 0;
	}	}
	fprintf(defer_fd, "ltl ");
	cnt = 0;
	while ((c = getc(yyin)) != EOF)
	{	if (c == '{')
		{	cnt++;
		}
		if (c == '}')
		{	cnt--;
			if (cnt == 0)
			{	break;
		}	}
		fprintf(defer_fd, "%c", c);
	}
	fprintf(defer_fd, "}\n");
	fflush(defer_fd);
	return 1;
}
#endif

#define EXPAND_SELECT
#ifdef EXPAND_SELECT
static char tmp_hold[256];
static int  tmp_has;

void
new_select(void)
{	tmp_hold[0] = '\0';
	tmp_has = 0;
}

static int
scan_to(int stop, int (*tst)(int), char *buf, int bufsz)
{	int c, i = 0;

	do {	c = Getchar();
		if (tmp_has < sizeof(tmp_hold))
		{	tmp_hold[tmp_has++] = c;
		}
		if (c == '\n')
		{	lineno++;
		} else if (buf && i < bufsz-1)
		{	buf[i++] = c;
		} else if (buf && i >= bufsz-1)
		{	buf[bufsz-1] = '\0';
			fatal("name too long", buf);
		}
		if (tst && !tst(c) && c != ' ' && c != '\t')
		{	break;
		}
	} while (c != stop && c != EOF);

	if (buf)
	{	if (i <= 0)
		{	fatal("input error", (char *) 0);
		}
		buf[i-1] = '\0';
	}

	if (c != stop)
	{	if (0)
		{	printf("saw: '%c', expected '%c'\n", c, stop);
		}
		if (tmp_has < sizeof(tmp_hold))
		{	tmp_hold[tmp_has] = '\0';
			push_back(tmp_hold);
			if (0)
			{	printf("pushed back: <'%s'>\n", tmp_hold);
			}
			return 0; /* internal expansion fails */
		} else
		{	fatal("expecting select ( name : constant .. constant )", 0);
	}	}
	return 1;		/* success */
}
#endif

int
lex(void)
{	int c;
again:
	c = Getchar();
/* more: */
	yytext[0] = (char) c;
	yytext[1] = '\0';
	switch (c) {
	case EOF:
#ifdef DEFER_LTL
		if (!deferred)
		{	deferred = 1;
			if (get_deferred())
			{	goto again;
			}
		} else
		{	zap_deferred();
		}
#endif
		return c;
	case '\n':		/* newline */
		lineno++;
		/* make most semi-colons optional */
		if (implied_semis
	/*	&&  context	*/
		&&  in_seq
		&&  par_cnt == 0
		&&  follows_token(last_token))
		{	if (last_token == '}')
			{	do {	c = Getchar();
					if (c == '\n')
					{	lineno++;
					}
				} while (c == ' ' || c == '\t' ||
					 c == '\f' || c == '\n' ||
					 c == '\r');
				Ungetch(c);
				if (0) printf("%d: saw %d\n", lineno, c);
				if (c == 'u') /* first letter of UNLESS */
				{	goto again;
			}	}
			if (0)
			{	printf("insert ; line %d, last_token %d in_seq %d\n",
				 lineno-1, last_token, in_seq);
			}
			ValToken(1, SEMI);
		}
		/* else fall thru */
	case '\r':		/* carriage return */
		goto again;

	case  ' ': case '\t': case '\f':	/* white space */
		goto again;

	case '#':		/* preprocessor directive */
		if (in_comment) goto again;
		if (pp_mode)
		{	last_token = PREPROC;
			return pre_proc();
		}
		do_directive(c);
		goto again;

	case '\"':
		getword(c, notquote);
		if (Getchar() != '\"')
			fatal("string not terminated", yytext);
		strcat(yytext, "\"");
		SymToken(lookup(yytext), STRING)

	case '$':
		getword('\"', notdollar);
		if (Getchar() != '$')
			fatal("ltl definition not terminated", yytext);
		strcat(yytext, "\""); 
		SymToken(lookup(yytext), STRING)

	case '\'':	/* new 3.0.9 */
		c = Getchar();
		if (c == '\\')
		{	c = Getchar();
			if (c == 'n') c = '\n';
			else if (c == 'r') c = '\r';
			else if (c == 't') c = '\t';
			else if (c == 'f') c = '\f';
		}
		if (Getchar() != '\'' && !in_comment)
			fatal("character quote missing: %s", yytext);
		ValToken(c, CONST)

	default:
		break;
	}

	if (isdigit_(c))
	{	long int nr;
		getword(c, isdigit_);
		errno = 0;
		nr = strtol(yytext, NULL, 10);
		if (errno != 0)
		{	fprintf(stderr, "spin: value out of range: '%s' read as '%d'\n",
				yytext, (int) nr);
		}
		ValToken((int)nr, CONST)
	}

	if (isalpha_(c) || c == '_')
	{	getword(c, isalnum_);
		if (!in_comment)
		{	c = check_name(yytext);

/* replace timeout with (timeout) */
			if (c == TIMEOUT
			&&  Inlining < 0
			&&  last_token != '(')
			{	push_back("timeout)");
				last_token = '(';
				return '(';
			}
/* end */

#ifdef EXPAND_SELECT
			if (c == SELECT && Inlining < 0)
			{	char name[64], from[32], upto[32];
				int i, a, b;
				new_select();
				if (!scan_to('(', 0, 0, 0)
				||  !scan_to(':', isalnum, name, sizeof(name))
				||  !scan_to('.', isdigit, from, sizeof(from))
				||  !scan_to('.', 0, 0, 0)
				||  !scan_to(')', isdigit, upto, sizeof(upto)))
				{	goto not_expanded;
				}
				a = atoi(from);
				b = atoi(upto);
				if (0)
				{	printf("Select %s from %d to %d\n",
						name, a, b);
				}
				if (a > b)
				{	non_fatal("bad range in select statement", 0);
					goto again;
				}
				if (b - a <= 32)
				{	push_back("if ");
					for (i = a; i <= b; i++)
					{	char buf[256];
						push_back(":: ");
						sprintf(buf, "%s = %d ",
							name, i);
						push_back(buf);
					}
					push_back("fi ");
				} else
				{	char buf[256];
					sprintf(buf, "%s = %d; do ",
						name, a);
					push_back(buf);
					sprintf(buf, ":: (%s < %d) -> %s++ ",
						name, b, name);
					push_back(buf);
					push_back(":: break od; ");
				}
				goto again;
			}
not_expanded:
#endif

#ifdef DEFER_LTL
			if (c == LTL && !deferred)
			{	if (put_deferred())
				{	goto again;
			}	}
#endif
			if (c)
			{	last_token = c;
				return c;
			}
			/* else fall through */
		}
		goto again;
	}

	if (ltl_mode)
	{	switch (c) {
		case '-': c = follow('>', IMPLIES,    '-'); break;
		case '[': c = follow(']', ALWAYS,     '['); break;
		case '<': c = follow('>', EVENTUALLY, '<');
			  if (c == '<')
			  {	c = Getchar();
				if (c == '-')
				{	c = follow('>', EQUIV, '-');
					if (c == '-')
					{	Ungetch(c);
						c = '<';
					}
				} else
				{	Ungetch(c);
					c = '<';
			  }	}
		default:  break;
	}	}

	switch (c) {
	case '/': c = follow('*', 0, '/');
		  if (!c) { in_comment = 1; goto again; }
		  break;
	case '*': c = follow('/', 0, '*');
		  if (!c) { in_comment = 0; goto again; }
		  break;
	case ':': c = follow(':', SEP, ':'); break;
	case '-': c = follow('>', ARROW, follow('-', DECR, '-')); break;
	case '+': c = follow('+', INCR, '+'); break;
	case '<': c = follow('<', LSHIFT, follow('=', LE, LT)); break;
	case '>': c = follow('>', RSHIFT, follow('=', GE, GT)); break;
	case '=': c = follow('=', EQ, ASGN); break;
	case '!': c = follow('=', NE, follow('!', O_SND, SND)); break;
	case '?': c = follow('?', R_RCV, RCV); break;
	case '&': c = follow('&', AND, '&'); break;
	case '|': c = follow('|', OR, '|'); break;
	case ';': c = SEMI; break;
	case '.': c = follow('.', DOTDOT, '.'); break;
	case '{': scope_seq[scope_level++]++; set_cur_scope(); break;
	case '}': scope_level--; set_cur_scope(); break;
	default : break;
	}
	ValToken(0, c)
}

static struct {
	char *s;	int tok;
} LTL_syms[] = {
	/* [], <>, ->, and <-> are intercepted in lex() */
	{ "U",		UNTIL   },
	{ "V",		RELEASE },
	{ "W",		WEAK_UNTIL },
	{ "X",		NEXT    },
	{ "always",	ALWAYS  },
	{ "eventually",	EVENTUALLY },
	{ "until",	UNTIL   },
	{ "stronguntil",UNTIL   },
	{ "weakuntil",	WEAK_UNTIL   },
	{ "release",	RELEASE },
	{ "next",	NEXT    },
	{ "implies",	IMPLIES },
	{ "equivalent",	EQUIV   },
	{ 0, 		0       },
};

static struct {
	char *s;	int tok;	int val;	char *sym;
} Names[] = {
	{"active",	ACTIVE,		0,		0},
	{"assert",	ASSERT,		0,		0},
	{"atomic",	ATOMIC,		0,		0},
	{"bit",		TYPE,		BIT,		0},
	{"bool",	TYPE,		BIT,		0},
	{"break",	BREAK,		0,		0},
	{"byte",	TYPE,		BYTE,		0},
	{"c_code",	C_CODE,		0,		0},
	{"c_decl",	C_DECL,		0,		0},
	{"c_expr",	C_EXPR,		0,		0},
	{"c_state",	C_STATE,	0,		0},
	{"c_track",	C_TRACK,	0,		0},
	{"D_proctype",	D_PROCTYPE,	0,		0},
	{"do",		DO,		0,		0},
	{"chan",	TYPE,		CHAN,		0},
	{"else", 	ELSE,		0,		0},
	{"empty",	EMPTY,		0,		0},
	{"enabled",	ENABLED,	0,		0},
	{"eval",	EVAL,		0,		0},
	{"false",	CONST,		0,		0},
	{"fi",		FI,		0,		0},
	{"for",		FOR,		0,		0},
	{"full",	FULL,		0,		0},
	{"get_priority", GET_P,		0,		0},
	{"goto",	GOTO,		0,		0},
	{"hidden",	HIDDEN,		0,		":hide:"},
	{"if",		IF,		0,		0},
	{"in",		IN,		0,		0},
	{"init",	INIT,		0,		":init:"},
	{"inline",	INLINE,		0,		0},
	{"int",		TYPE,		INT,		0},
	{"len",		LEN,		0,		0},
	{"local",	ISLOCAL,	0,		":local:"},
	{"ltl",		LTL,		0,		":ltl:"},
	{"mtype",	TYPE,		MTYPE,		0},
	{"nempty",	NEMPTY,		0,		0},
	{"never",	CLAIM,		0,		":never:"},
	{"nfull",	NFULL,		0,		0},
	{"notrace",	TRACE,		0,		":notrace:"},
	{"np_",		NONPROGRESS,	0,		0},
	{"od",		OD,		0,		0},
	{"of",		OF,		0,		0},
	{"pc_value",	PC_VAL,		0,		0},
	{"pid",		TYPE,		BYTE,		0},
	{"printf",	PRINT,		0,		0},
	{"printm",	PRINTM,		0,		0},
	{"priority",	PRIORITY,	0,		0},
	{"proctype",	PROCTYPE,	0,		0},
	{"provided",	PROVIDED,	0,		0},
	{"return",	RETURN,		0,		0},
	{"run",		RUN,		0,		0},
	{"d_step",	D_STEP,		0,		0},
	{"select",	SELECT,		0,		0},
	{"set_priority", SET_P,		0,		0},
	{"short",	TYPE,		SHORT,		0},
	{"skip",	CONST,		1,		0},
	{"timeout",	TIMEOUT,	0,		0},
	{"trace",	TRACE,		0,		":trace:"},
	{"true",	CONST,		1,		0},
	{"show",	SHOW,		0,		":show:"},
	{"typedef",	TYPEDEF,	0,		0},
	{"unless",	UNLESS,		0,		0},
	{"unsigned",	TYPE,		UNSIGNED,	0},
	{"xr",		XU,		XR,		0},
	{"xs",		XU,		XS,		0},
	{0, 		0,		0,		0},
};

static int
check_name(char *s)
{	int i;

	yylval = nn(ZN, 0, ZN, ZN);

	if (ltl_mode)
	{	for (i = 0; LTL_syms[i].s; i++)
		{	if (strcmp(s, LTL_syms[i].s) == 0)
			{	return LTL_syms[i].tok;
	}	}	}

	for (i = 0; Names[i].s; i++)
	{	if (strcmp(s, Names[i].s) == 0)
		{	yylval->val = Names[i].val;
			if (Names[i].sym)
				yylval->sym = lookup(Names[i].sym);
			if (Names[i].tok == IN && !in_for)
			{	continue;
			}
			return Names[i].tok;
	}	}

	if ((yylval->val = ismtype(s)) != 0)
	{	yylval->ismtyp = 1;
		yylval->sym = (Symbol *) emalloc(sizeof(Symbol));
		yylval->sym->name = (char *) emalloc(strlen(s)+1);
		strcpy(yylval->sym->name, s);
		return CONST;
	}

	if (strcmp(s, "_last") == 0)
		has_last++;

	if (strcmp(s, "_priority") == 0)
		has_priority++;

	if (Inlining >= 0 && !ReDiRect)
	{	Lextok *tt, *t = Inline_stub[Inlining]->params;

		for (i = 0; t; t = t->rgt, i++)				/* formal pars */
		 if (!strcmp(s, t->lft->sym->name)			/* varname matches formal */
		 &&   strcmp(s, Inline_stub[Inlining]->anms[i]) != 0)	/* actual pars */
		 {
#if 0
			if (verbose&32)
			printf("\tline %d, replace %s in call of '%s' with %s\n",
				lineno, s,
				Inline_stub[Inlining]->nm->name,
				Inline_stub[Inlining]->anms[i]);
#endif
			for (tt = Inline_stub[Inlining]->params; tt; tt = tt->rgt)
				if (!strcmp(Inline_stub[Inlining]->anms[i],
					tt->lft->sym->name))
				{	/* would be cyclic if not caught */
					printf("spin: %s:%d replacement value: %s\n",
						oFname->name?oFname->name:"--", lineno, tt->lft->sym->name);
					fatal("formal par of %s contains replacement value",
						Inline_stub[Inlining]->nm->name);
					yylval->ntyp = tt->lft->ntyp;
					yylval->sym = lookup(tt->lft->sym->name);
					return NAME;
				}

			/* check for occurrence of param as field of struct */
			{ char *ptr = Inline_stub[Inlining]->anms[i];
				char *optr = ptr;
				while ((ptr = strstr(ptr, s)) != NULL)
				{	if ((ptr > optr && *(ptr-1) == '.')
					||  *(ptr+strlen(s)) == '.')
					{	fatal("formal par of %s used in structure name",
						Inline_stub[Inlining]->nm->name);
					}
					ptr++;
			}	}
			ReDiRect = Inline_stub[Inlining]->anms[i];
			return 0;
	}	 }

	yylval->sym = lookup(s);	/* symbol table */
	if (isutype(s))
		return UNAME;
	if (isproctype(s))
		return PNAME;
	if (iseqname(s))
		return INAME;

	return NAME;
}

int
yylex(void)
{	static int last = 0;
	static int hold = 0;
	int c;
	/*
	 * repair two common syntax mistakes with
	 * semi-colons before or after a '}'
	 */
	if (hold)
	{	c = hold;
		hold = 0;
		last_token = c;
	} else
	{	c = lex();
		if (last == ELSE
		&&  c != SEMI
		&&  c != ARROW
		&&  c != FI)
		{	hold = c;
			last = 0;
			last_token = SEMI;
			return SEMI;
		}
		if (last == '}'
		&&  c != PROCTYPE
		&&  c != INIT
		&&  c != CLAIM
		&&  c != SEP
		&&  c != FI
		&&  c != OD
		&&  c != '}'
		&&  c != UNLESS
		&&  c != SEMI
		&&  c != ARROW
		&&  c != EOF)
		{	hold = c;
			last = 0;
			last_token = SEMI;
			return SEMI;	/* insert ';' */
		}
		if (c == SEMI || c == ARROW)
		{	/* if context, we're not in a typedef
			 * because they're global.
			 * if owner, we're at the end of a ref
			 * to a struct field -- prevent that the
			 * lookahead is interpreted as a field of
			 * the same struct...
			 */
			if (context) owner = ZS;
			hold = lex();	/* look ahead */
			if (hold == '}'
			||  hold == ARROW
			||  hold == SEMI)
			{	c = hold; /* omit ';' */
				hold = 0;
			}
		}
	}
	last = c;

	if (IArgs)
	{	static int IArg_nst = 0;

		if (strcmp(yytext, ",") == 0)
		{	IArg_cont[++IArgno][0] = '\0';
		} else if (strcmp(yytext, "(") == 0)
		{	if (IArg_nst++ == 0)
			{	IArgno = 0;
				IArg_cont[0][0] = '\0';
			} else
			{	assert(strlen(IArg_cont[IArgno])+strlen(yytext) < sizeof(IArg_cont));
				strcat(IArg_cont[IArgno], yytext);
			}
		} else if (strcmp(yytext, ")") == 0)
		{	if (--IArg_nst > 0)
			{	assert(strlen(IArg_cont[IArgno])+strlen(yytext) < sizeof(IArg_cont));
				strcat(IArg_cont[IArgno], yytext);
			}
		} else if (c == CONST && yytext[0] == '\'')
		{	sprintf(yytext, "'%c'", yylval->val);
			assert(strlen(IArg_cont[IArgno])+strlen(yytext) < sizeof(IArg_cont));
			strcat(IArg_cont[IArgno], yytext);
		} else if (c == CONST)
		{	sprintf(yytext, "%d", yylval->val);
			assert(strlen(IArg_cont[IArgno])+strlen(yytext) < sizeof(IArg_cont));
			strcat(IArg_cont[IArgno], yytext);
		} else
		{
			switch (c) {
			case ARROW:	strcpy(yytext, "->"); break; /* NEW */
			case SEP:	strcpy(yytext, "::"); break;
			case SEMI:	strcpy(yytext, ";"); break;
			case DECR:	strcpy(yytext, "--"); break;
			case INCR: 	strcpy(yytext, "++"); break;
			case LSHIFT:	strcpy(yytext, "<<"); break;
			case RSHIFT:	strcpy(yytext, ">>"); break;
			case LE:	strcpy(yytext, "<="); break;
			case LT:	strcpy(yytext, "<"); break;
			case GE:	strcpy(yytext, ">="); break;
			case GT:	strcpy(yytext, ">"); break;
			case EQ:	strcpy(yytext, "=="); break;
			case ASGN:	strcpy(yytext, "="); break;
			case NE:	strcpy(yytext, "!="); break;
			case R_RCV:	strcpy(yytext, "??"); break;
			case RCV:	strcpy(yytext, "?"); break;
			case O_SND:	strcpy(yytext, "!!"); break;
			case SND:	strcpy(yytext, "!"); break;
			case AND: 	strcpy(yytext, "&&"); break;
			case OR:	strcpy(yytext, "||"); break;
			}
			assert(strlen(IArg_cont[IArgno])+strlen(yytext) < sizeof(IArg_cont));
			strcat(IArg_cont[IArgno], yytext);
		}
	}
	return c;
}
