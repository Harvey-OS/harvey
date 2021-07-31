/***** spin: main.c *****/

/* Copyright (c) 1991,1995 by AT&T Corporation.  All Rights Reserved.     */
/* This software is for educational purposes only.                        */
/* Permission is given to distribute this code provided that this intro-  */
/* ductory message is not removed and no monies are exchanged.            */
/* No guarantee is expressed or implied by the distribution of this code. */
/* Software written by Gerard J. Holzmann as part of the book:            */
/* `Design and Validation of Computer Protocols,' ISBN 0-13-539925-4,     */
/* Prentice Hall, Englewood Cliffs, NJ, 07632.                            */
/* Send bug-reports and/or questions to: gerard@research.att.com          */

#include "spin.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "y.tab.h"

extern int	DstepStart, lineno;
extern FILE	*yyin;

Symbol	*Fname, *oFname;

int	verbose = 0;
int	analyze = 0;
int	s_trail = 0;
int	m_loss  = 0;
int	dumptab	= 0;
int	nr_errs	= 0;
int	dataflow = 0;
int	has_remote = 0;
int	Interactive = 0;
int	Ntimeouts = 0;	/* counts those used in never claim */
int	Etimeouts = 0;	/* counts those used in program */
int	xspin = 0;	/* set when used with xspin interface */

void
main(int argc, char *argv[])
{	Symbol *s;
	int T = (int) time((long *)0);

	while (argc > 1 && argv[1][0] == '-')
	{	switch (argv[1][1]) {
		case 'a': analyze  =  1; break;
		case 'd': dumptab  =  1; break;
		case 'D': dataflow++; break;
		case 'g': verbose +=  1; break;
		case 'i': Interactive = 1; break;
		case 'l': verbose +=  2; break;
		case 'm': m_loss   =  1; break;
		case 'n': T = atoi(&argv[1][2]); break;
		case 'p': verbose +=  4; break;
		case 'r': verbose +=  8; break;
		case 's': verbose += 16; break;
		case 't': s_trail  =  1; break;
		case 'v': verbose += 32; break;
		case 'V': printf("%s\n", Version); exit(0);
		case 'X': xspin    =  1;
			  signal(SIGPIPE, exit); /* not posix compliant... */
			  break;
		default : printf("use: spin [-option] ... [-option] file\n");
			  printf("\t-a produce an analyzer\n");
			  printf("\t-d produce symbol-table information\n");
			  printf("\t-D write/write dataflow\n");
			  printf("\t-D -D read/write dataflow\n");
			  printf("\t-g print all global variables\n");
			  printf("\t-i interactive (random simulation)\n");
			  printf("\t-l print all local variables\n");
			  printf("\t-m lose msgs sent to full queues\n");
			  printf("\t-nN seed for random nr generator\n");
			  printf("\t-p print all statements\n");
			  printf("\t-r print receive events\n");
			  printf("\t-s print send events\n");
			  printf("\t-v verbose, more warnings\n");
			  printf("\t-t follow a simulation trail\n");
			  printf("\t-V print version number and exit\n");
			  exit(1);
		}
		argc--, argv++;
	}
	if (argc > 1)
	{	char outfile[32], cmd[128];
		extern char *tmpnam(char *);
		(void) tmpnam(outfile);

		/* on some systems: "/usr/ccs/lib/cpp" */
		sprintf(cmd, "/bin/cpp %s > %s", argv[1], outfile);
		if (system((const char *)cmd))
		{	(void) unlink((const char *)outfile);
			exit(1);
		} else if (!(yyin = fopen(outfile, "r")))
		{	printf("cannot open %s\n", outfile);
			exit(1);
		}
		(void) unlink((const char *)outfile);
		oFname = Fname = lookup(argv[1]);
		if (oFname->name[0] == '\"')
		{	int i = strlen(oFname->name);
			oFname->name[i-1] = '\0';
			oFname = lookup(&oFname->name[1]);
		}
	} else
		Fname = lookup("<stdin>");
	Srand(T);	/* defined in run.c */
	s = lookup("_p");	s->type = PREDEF;
	s = lookup("_pid");	s->type = PREDEF;
	s = lookup("_last");	s->type = PREDEF;
	yyparse();
	exit(nr_errs);
}

int
yywrap(void)	/* dummy routine */
{
	return 1;
}

void
non_fatal(char *s1, char *s2)
{	extern int yychar; extern char yytext[];

	printf("spin: line %3d %s: ", lineno, Fname->name);
	if (s2)
		printf(s1, s2);
	else
		printf(s1);
	if (yychar != -1 && yychar != 0)
	{	printf("	saw '");
		explain(yychar);
		printf("'");
	}
	if (yytext && strlen(yytext)>1)
		printf(" near '%s'", yytext);
	printf("\n"); fflush(stdout);
	nr_errs++;
}

void
fatal(char *s1, char *s2)
{
	non_fatal(s1, s2);
	exit(1);
}

char *
emalloc(int n)
{	char *tmp;

	if (!(tmp = (char *) malloc(n)))
		fatal("not enough memory", (char *)0);
	memset(tmp, 0, n);
	return tmp;
}

Lextok *
nn(Lextok *s, int t, Lextok *ll, Lextok *rl)
{	Lextok *n = (Lextok *) emalloc(sizeof(Lextok));
	extern char *claimproc;
	extern Symbol *context;

	n->ntyp = t;
	if (s && s->fn)
	{	n->ln = s->ln;
		n->fn = s->fn;
	} else if (rl && rl->fn)
	{	n->ln = rl->ln;
		n->fn = rl->fn;
	} else if (ll && ll->fn)
	{	n->ln = ll->ln;
		n->fn = ll->fn;
	} else
	{	n->ln = lineno;
		n->fn = Fname;
	}
	if (s) n->sym  = s->sym;
	n->lft  = ll;
	n->rgt  = rl;
	n->indstep = DstepStart;

	if (t == TIMEOUT) Etimeouts++;

	if (!context)
		return n;
	if (context->name == claimproc)
	{	switch (t) {
		case ASGN: case 'r': case 's':
			non_fatal("never claim has side-effect",(char *)0);
			break;
		case TIMEOUT:
			/* never claim polls timeout */
			Ntimeouts++; Etimeouts--;
			break;
		case LEN: case EMPTY: case FULL:
		case 'R': case NFULL: case NEMPTY:
			/* status bumped to non-exclusive */
			if (n->sym) n->sym->xu |= XX;
			break;
		default:
			break;
		}
	} else if (t == ENABLED)
		fatal("using enabled() outside never-claim",(char *)0);
		/* this affects how enabled is implemented in run.c */

	return n;
}

Lextok *
rem_lab(Symbol *a, Lextok *b, Symbol *c)
{	Lextok *tmp1, *tmp2, *tmp3;

	has_remote++;
	fix_dest(c, a);	/* in case target is jump */
	tmp1 = nn(ZN, '?',   b, ZN); tmp1->sym = a;
	tmp1 = nn(ZN, 'p', tmp1, ZN);
	tmp1->sym = lookup("_p");
	tmp2 = nn(ZN, NAME,  ZN, ZN); tmp2->sym = a;
	tmp3 = nn(ZN, 'q', tmp2, ZN); tmp3->sym = c;
	return nn(ZN, EQ, tmp1, tmp3);
}

char Operator[] = "operator: ";
char Keyword[]  = "keyword: ";
char Function[] = "function-name: ";

void
explain(int n)
{
	switch (n) {
	default:	if (n > 0 && n < 256)
				printf("%c' = '", n);
			printf("%d", n);
			break;
	case '\b':	printf("\\b"); break;
	case '\t':	printf("\\t"); break;
	case '\f':	printf("\\f"); break;
	case '\n':	printf("\\n"); break;
	case '\r':	printf("\\r"); break;
	case 'c':	printf("condition"); break;
	case 's':	printf("send"); break;
	case 'r':	printf("recv"); break;
	case '@':	printf("delproc"); break;
	case '?':	printf("(x->y:z)"); break;
	case ACTIVE:	printf("%sactive",	Keyword); break;
	case AND:	printf("%s&&",		Operator); break;
	case ASGN:	printf("%s=",		Operator); break;
	case ASSERT:	printf("%sassert",	Function); break;
	case ATOMIC:	printf("%satomic",	Keyword); break;
	case BREAK:	printf("%sbreak",	Keyword); break;
	case CLAIM:	printf("%snever",	Keyword); break;
	case CONST:	printf("a constant"); break;
	case DECR:	printf("%s--",		Operator); break;
	case D_STEP:	printf("%sd_step",	Keyword); break;
	case DO:	printf("%sdo",		Keyword); break;
	case DOT:	printf("."); break;
	case ELSE:	printf("%selse",	Keyword); break;
	case EMPTY:	printf("%sempty",	Function); break;
	case ENABLED:	printf("%senabled",	Function); break;
	case EQ:	printf("%s==",		Operator); break;
	case FI:	printf("%sfi",		Keyword); break;
	case FULL:	printf("%sfull",	Function); break;
	case GE:	printf("%s>=",		Operator); break;
	case GOTO:	printf("%sgoto",	Keyword); break;
	case GT:	printf("%s>",		Operator); break;
	case IF:	printf("%sif",		Keyword); break;
	case INCR:	printf("%s++",		Operator); break;
	case INIT:	printf("%sinit",	Keyword); break;
	case LABEL:	printf("a label-name"); break;
	case LE:	printf("%s<=",		Operator); break;
	case LEN:	printf("%slen",		Function); break;
	case LSHIFT:	printf("%s<<",		Operator); break;
	case LT:	printf("%s<",		Operator); break;
	case MTYPE:	printf("%smtype",	Keyword); break;
	case NAME:	printf("an identifier"); break;
	case NE:	printf("%s!=",		Operator); break;
	case NEG:	printf("%s! (not)",	Operator); break;
	case NEMPTY:	printf("%snempty",	Function); break;
	case NFULL:	printf("%snfull",	Function); break;
	case NON_ATOMIC: printf("sub-sequence"); break;
	case OD:	printf("%sod",		Keyword); break;
	case OF:	printf("%sof",		Keyword); break;
	case OR:	printf("%s||",		Operator); break;
	case O_SND:	printf("%s!!",		Operator); break;
	case PC_VAL:	printf("%spc_value",	Function); break;
	case PNAME:	printf("process name"); break;
	case PRINT:	printf("%sprintf",	Function); break;
	case PROCTYPE:	printf("%sproctype",	Keyword); break;
	case RCV:	printf("%s?",		Operator); break;
	case R_RCV:	printf("%s??",		Operator); break;
	case RSHIFT:	printf("%s>>",		Operator); break;
	case RUN:	printf("%srun",		Operator); break;
	case SEP:	printf("token: ::"); break;
	case SEMI:	printf(";"); break;
	case SND:	printf("%s!",		Operator); break;
	case STRING:	printf("a string"); break;
	case TIMEOUT:	printf("%stimeout",	Keyword); break;
	case TYPE:	printf("data typename"); break;
	case TYPEDEF:	printf("%stypedef",	Keyword); break;
	case XU:	printf("%sx[rs]",	Keyword); break;
	case UMIN:	printf("%s- (unary minus)", Operator); break;
	case UNAME:	printf("a typename"); break;
	case UNLESS:	printf("%sunless",	Keyword); break;
	}
}

static int IsAsgn = 0, OrIsAsgn = 0;
static Element *Same;

int
used_here(Symbol *s, Lextok *n)
{	extern Symbol *context;
	int res = 0;

	if (!n) return 0;
#ifdef DEBUG
	{	int oln; Symbol *ofn;
		printf("	used_here %s -- ", context->name);
		oln = lineno; ofn = Fname;
		comment(stdout, n, 0);
		lineno = oln; Fname = ofn;
		printf(" -- %d:%s\n", n->ln,n->fn->name);
	}
#endif
	if (n->sym == s) res = (IsAsgn || n->ntyp == ASGN)?2:1;
	if (n->ntyp == ASGN)
		res |= used_here(s, n->lft->lft);
	else
		res |= used_here(s, n->lft);
	if (n->ntyp == 'r')
		IsAsgn = 1;
	res |= used_here(s, n->rgt);
	if (n->ntyp == 'r')
		IsAsgn = 0;
	return res;
}

int
used_later(Symbol *s, Element *t)
{	extern Symbol *context;
	int res = 0;

	if (!t || !s)
		return 0;
	if (t->status&CHECK2)
	{
#ifdef DEBUG
		printf("\t%d used_later: done before\n", t->Seqno);
#endif
		return (t->Seqno == Same->Seqno) ? 4 : 0;
	}

	t->status |= CHECK2;

#ifdef DEBUG
	{	int oln; Symbol *ofn;
		printf("\t%d %u ->%d %u used_later %s -- ",
			t->seqno,
			t, (t->nxt)?t->nxt->seqno:-1,
			t->nxt, context->name);
		oln = lineno; ofn = Fname;
		comment(stdout, t->n, 0);
		lineno = oln; Fname = ofn;
		printf(" -- %d:%s\n", t->n->ln, t->n->fn->name);
	}
#endif
	if (t->n->ntyp == GOTO)
	{	Element *j = target(t);
#ifdef DEBUG
		printf("\t\tjump to %d\n", j?j->Seqno:-1);
#endif
		res |= used_later(s, j);
		goto done;
	}

	if (t->n->sl && ! t->sub)	/* d_step or (non-) atomic */
	{	SeqList *f;
		for (f = t->n->sl; f; f = f->nxt)
		{	f->this->last->nxt = t->nxt;
#ifdef DEBUG
	printf("\tPatch2 %d->%d (%d)\n",
	f->this->last->seqno, t->nxt?t->nxt->seqno:-1, t->n->ntyp);
#endif
			res |= used_later(s, f->this->frst);
		}
	} else if (t->sub)	/* IF or DO */
	{	SeqList *f;
		for (f = t->sub; f; f = f->nxt)
			res |= used_later(s, f->this->frst);
	} else
	{	res |= used_here(s, t->n);
	}
	if (!(res&3)) res |= used_later(s, t->nxt);
done:
	t->status &= ~CHECK2;
	return res;
}

void
varused(Lextok *t, Element *u, int isread)
{	int res = 0;

	if (!t || !t->sym)		return;
	if (dataflow == 1 && isread)	return;

	res = used_later(t->sym, u);

	if ((res&1)
	||  (isread && res&4))
		 return;	/* followed by at least one read */

	printf("%s:%3d: ", 
		Same->n->fn->name,
		Same->n->ln);
	if (t->sym->owner) printf("%s.", t->sym->owner->name);
	printf("%s -- (%s)",
		t->sym->name,
		(isread)?"read":"write");

	if (!res)	{ printf(" none"); printf("\n");exit(0); }
	if (res&2)	printf(" write");
	if (res&4)	printf(" same");
	printf("\n");

}

void
varprobe(Element *parent, Lextok *n, Element *q)
{
	if (!n) return;

	Same = parent;

	/* can't deal with globals, structs, or arrays */
	if (n->sym
	&&  n->sym->context
	&&  n->sym->nel == 1
	&&  n->sym->type != STRUCT
	&&  n->ntyp != PRINT)
		varused(n, q, (!OrIsAsgn && n->ntyp != ASGN));

	if (n->ntyp == ASGN)
		varprobe(parent, n->lft->lft, q);
	else
		varprobe(parent, n->lft, q);

	if (n->ntyp == 'r')
		OrIsAsgn = 1;

	varprobe(parent, n->rgt, q);

	if (n->ntyp == 'r')
		OrIsAsgn = 0;
}

#if 0
#define walkprog	varcheck
#else
#define Varcheck	varcheck
#endif

void
Varcheck(Element *e, Element *nx)
{	SeqList *f; extern Symbol *context;

	if (!dataflow || !e || e->status&CHECK1)
		return;
#ifdef DEBUG
	{	int oln; Symbol *ofn;
		printf("%s:%d -- ", context->name, e->Seqno);
		oln = lineno; ofn = Fname;
		comment(stdout, e->n, 0);
		lineno = oln; Fname = ofn;
		printf(" -- %d:%s\n", e->n->ln, e->n->fn->name);
	}
#endif
	e->status |= CHECK1;

	if (e->n->ntyp == GOTO)
	{	Element *ef = target(e);
		if (ef) varcheck(ef, ef->nxt);
		goto done;
	} else if (e->n->sl && !e->sub)	/* d_step or (non)-atomic */
	{	for (f = e->n->sl; f; f = f->nxt)
		{	f->this->last->nxt = nx;
#ifdef DEBUG
			printf("\tPatch1 %d->%d\n",
			f->this->last->seqno, nx?nx->seqno:-1);
			varcheck(f->this->frst, nx);
#endif
			f->this->last->nxt = 0;
		}
	} else if (e->sub && e->n->ntyp == IF)	/* if */
	{	for (f = e->sub; f; f = f->nxt)
			varcheck(f->this->frst, nx);
	} else if (e->sub && e->n->ntyp == DO)	/* do */
	{	for (f = e->sub; f; f = f->nxt)
			varcheck(f->this->frst, e);
	} else
	{	varprobe(e, e->n, e->nxt);
	}
	{	Element *ef = huntele(e->nxt, e->status);
		if (ef) varcheck(ef, ef->nxt);
	}
done:
	/* e->status &= ~CHECK1 */ ;
}

void
nested(int n)
{	int i;
	for (i = 0; i < n; i++)
		printf("\t");
}

void
walkprog(Element *e, Element *nx)
{	SeqList *f; extern Symbol *context;
	static int Nest=0; int oln;

	if (!dataflow) return;
	if (!e)
	{	nested(Nest);
		printf("nil\n");
		return;
	}

	nested(Nest);
	printf("%4d,%4d, %s:%d(%u) -- ",
		e->status, lineno,
		context->name, e->Seqno, e);
	oln = lineno;
	comment(stdout, e->n, 0);
	lineno = oln;
	printf(" -- %d:%s\n", e->n->ln, e->n->fn->name);

	if (e->status&CHECK1)
	{	nested(Nest);
		printf("seenbefore\n");
		return;
	}

	e->status |= CHECK1;

	if (e->n->ntyp == GOTO)
	{	Element *ef = target(e);
		if (ef) walkprog(ef, ef->nxt);
	} else if (e->n->sl && !e->sub)	/* ATOMIC, NON_ATOMIC, D_STEP */
	{	int cnt;

		for (f = e->n->sl, cnt=1; f; f = f->nxt, cnt++)
		{	Nest++;
			nested(Nest);
			printf("---a>%d:\n", cnt);
#ifdef DEBUG
			printf("\tPatch0 %d->%d\n",
			f->this->last->seqno, nx?nx->seqno:-1);
			f->this->last->nxt = nx;
			walkprog(f->this->frst, nx);
#endif
		/*	f->this->last->nxt = 0;		*/
			Nest--;
		}
	} else if (e->sub && e->n->ntyp == IF)
	{	int cnt;
		for (f = e->sub, cnt=1; f; f = f->nxt, cnt++)
		{	Nest++;
			nested(Nest);
			printf("---s>%d:\n", cnt);
			walkprog(f->this->frst, nx);
			Nest--;
		}
	} else if (e->sub && e->n->ntyp == DO)
	{	int cnt;
		for (f = e->sub, cnt=1; f; f = f->nxt, cnt++)
		{	Nest++;
			nested(Nest);
			printf("---s>%d:\n", cnt);
			walkprog(f->this->frst, e);
			Nest--;
		}
	}
	{	Element *ef = huntele(e->nxt, e->status);
		if (ef) walkprog(ef, ef->nxt);
	}
	e->status &= ~CHECK1;
}
