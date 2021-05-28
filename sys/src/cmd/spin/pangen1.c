/***** spin: pangen1.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include "spin.h"
#include "y.tab.h"
#include "pangen1.h"
#include "pangen3.h"
#include "pangen6.h"
#include <assert.h>

extern FILE	*fd_tc, *fd_th, *fd_tt;
extern Label	*labtab;
extern Ordered	*all_names;
extern ProcList	*ready;
extern Queue	*qtab;
extern Symbol	*Fname;
extern int	lineno, verbose, Pid_nr, separate, old_scope_rules, nclaims;
extern int	nrRdy, nrqs, mstp, Mpars, claimnr, eventmapnr;
extern short	has_sorted, has_random, has_provided, has_priority;
extern Queue	*ltab[];

int	Npars=0, u_sync=0, u_async=0, hastrack = 1;
short	has_io = 0;
short	has_state=0;	/* code contains c_state */

extern void	c_add_stack(FILE *);
extern void	c_stack_size(FILE *);

static Symbol	*LstSet=ZS;
static int	acceptors=0, progressors=0, nBits=0;
static int	Types[] = { UNSIGNED, BIT, BYTE, CHAN, MTYPE, SHORT, INT, STRUCT };

static int	doglobal(char *, int);
static void	dohidden(void);
static void	do_init(FILE *, Symbol *);
static void	end_labs(Symbol *, int);
static void	put_ptype(char *, int, int, int, enum btypes);
static void	tc_predef_np(void);
static void	put_pinit(ProcList *);
static void	multi_init(void);
       void	walk_struct(FILE *, int, char *, Symbol *, char *, char *, char *);

static void
reverse_names(ProcList *p)
{
	if (!p) return;
	reverse_names(p->nxt);
	fprintf(fd_tc, "   \"%s\",\n", p->n->name);
}
static void
reverse_types(ProcList *p)
{
	if (!p) return;
	reverse_types(p->nxt);
	fprintf(fd_tc, "   %d,	/* %s */\n", p->b, p->n->name);
}

static int
blog(int n)	/* for small log2 without rounding problems */
{	int m=1, r=2;

	while (r < n) { m++; r *= 2; }
	return 1+m;
}

void
genheader(void)
{	ProcList *p; int i;

	if (separate == 2)
	{	putunames(fd_th);
		goto here;
	}
	/* 5.2.3: gcc 3 no longer seems to compute sizeof at compile time */
	fprintf(fd_th, "#define WS		%d /* word size in bytes */\n", (int) sizeof(void *));
	fprintf(fd_th, "#define SYNC	%d\n", u_sync);
	fprintf(fd_th, "#define ASYNC	%d\n\n", u_async);
	fprintf(fd_th, "#ifndef NCORE\n");
	fprintf(fd_th, "	#ifdef DUAL_CORE\n");
	fprintf(fd_th, "		#define NCORE	2\n");
	fprintf(fd_th, "	#elif QUAD_CORE\n");
	fprintf(fd_th, "		#define NCORE	4\n");
	fprintf(fd_th, "	#else\n");
	fprintf(fd_th, "		#define NCORE	1\n");
	fprintf(fd_th, "	#endif\n");
	fprintf(fd_th, "#endif\n\n");

	putunames(fd_th);

	fprintf(fd_tc, "\nshort Air[] = { ");
	for (p = ready, i=0; p; p = p->nxt, i++)
		fprintf(fd_tc, "%s (short) Air%d", (p!=ready)?",":"", i);
	fprintf(fd_tc, ", (short) Air%d", i);	/* np_ */
	if (nclaims > 1)
	{	fprintf(fd_tc, "\n#ifndef NOCLAIM\n");
		fprintf(fd_tc, "	, (short) Air%d", i+1);	/* Multi */
		fprintf(fd_tc, "\n#endif\n\t");
	}
	fprintf(fd_tc, " };\n");

	fprintf(fd_tc, "char *procname[] = {\n");
		reverse_names(ready);
	fprintf(fd_tc, "   \":np_:\",\n");
	fprintf(fd_tc, "	0\n");
	fprintf(fd_tc, "};\n\n");

	fprintf(fd_tc, "enum btypes { NONE=%d, N_CLAIM=%d,", NONE, N_CLAIM);
	fprintf(fd_tc, " I_PROC=%d, A_PROC=%d,", I_PROC, A_PROC);
	fprintf(fd_tc, " P_PROC=%d, E_TRACE=%d, N_TRACE=%d };\n\n",
		P_PROC, E_TRACE, N_TRACE);

	fprintf(fd_tc, "int Btypes[] = {\n");
		reverse_types(ready);
	fprintf(fd_tc, "   0	/* :np_: */\n");
	fprintf(fd_tc, "};\n\n");

here:
	for (p = ready; p; p = p->nxt)
		put_ptype(p->n->name, p->tn, mstp, nrRdy+1, p->b);
		/* +1 for np_ */
	put_ptype("np_", nrRdy, mstp, nrRdy+1, 0);

	if (nclaims > 1)
	{	/* this is the structure that goes into the state-vector
		 * instead of the actual never claims
		 * this assumes that the claims do not have any local variables
		 * this claim records the types and states of all subclaims in an array
		 * NB: not sure if we need the first 3 fields in this structure
		 *     it's here for now to avoid breaking some possible dependence
		 * in the calculations above, we were already taking into account
		 * that there is one never-claim, which will now be this one
		 */

		i = blog(mstp);
		fprintf(fd_th, "\n");

		fprintf(fd_th, "#ifndef NOCLAIM\n");
		fprintf(fd_th, " #ifndef NP\n");
		fprintf(fd_th, "	#undef VERI\n");
		fprintf(fd_th, "	#define VERI	%d\n", nrRdy+1);
		fprintf(fd_th, " #endif\n");
		fprintf(fd_th, "	#define Pclaim	P%d\n\n", nrRdy+1);
		fprintf(fd_th, "typedef struct P%d {\n", nrRdy+1);
		fprintf(fd_th, "	unsigned _pid : 8; /* always zero */\n");
		fprintf(fd_th, "	unsigned _t   : %d; /* active-claim type  */\n",
			blog(nrRdy+1));
		fprintf(fd_th, "	unsigned _p   : %d; /* active-claim state */\n",
			i);
		fprintf(fd_th, "	unsigned _n   : %d; /* active-claim index */\n",
			blog(nclaims));
		if (i <= 255)	/* in stdint.h = UCHAR_MAX from limits.h */
		{	fprintf(fd_th, "	uchar c_cur[NCLAIMS]; /* claim-states */\n");
		} else if (i <= 65535)	/* really USHRT_MAX from limits.h */
		{	fprintf(fd_th, "	ushort c_cur[NCLAIMS]; /* claim-states */\n");
		} else	/* the most unlikely case */
		{	fprintf(fd_th, "	uint c_cur[NCLAIMS]; /* claim-states */\n");
		}
		fprintf(fd_th, "} P%d;\n", nrRdy+1);

		fprintf(fd_tc, "#ifndef NOCLAIM\n");
		fprintf(fd_tc, "uchar spin_c_typ[NCLAIMS]; /* claim-types */\n");
		fprintf(fd_tc, "#endif\n");

		fprintf(fd_th, "	#define Air%d	(0)\n\n", nrRdy+1);
		fprintf(fd_th, "#endif\n");
		/*
		 * find special states as:
		 *	stopstate [ claimnr ][ curstate ] == 1
		 *	accpstate [ claimnr ][ curstate ]
		 *	progstate [ claimnr ][ curstate ]
		 *	reached   [ claimnr ][ curstate ]
		 *	visstate  [ claimnr ][ curstate ]
		 *	loopstate [ claimnr ][ curstate ]
		 *	mapstate  [ claimnr ][ curstate ]
		 */
	} else
	{	fprintf(fd_th, "#define Pclaim	P0\n");
		fprintf(fd_th, "#ifndef NCLAIMS\n");
		fprintf(fd_th, "	#define NCLAIMS 1\n");
		fprintf(fd_th, "#endif\n");
		fprintf(fd_tc, "uchar spin_c_typ[NCLAIMS]; /* claim-types */\n");
	}

	ntimes(fd_th, 0, 1, Head0);

	if (separate != 2)
	{
		ntimes(fd_th, 0, 1, Header);
		fprintf(fd_th, "#define StackSize	(");
			c_stack_size(fd_th);
		fprintf(fd_th, ")\n");

		c_add_stack(fd_th);
		ntimes(fd_th, 0, 1, Header0);
	} else
	{	fprintf(fd_th, "extern char *emalloc(unsigned long);\n");
	}
	ntimes(fd_th, 0, 1, Head1);

	LstSet = ZS;
	(void) doglobal("", PUTV);

	hastrack = c_add_sv(fd_th);

	fprintf(fd_th, "#ifdef TRIX\n");
	fprintf(fd_th, "	/* room for 512 proc+chan ptrs, + safety margin */\n");
	fprintf(fd_th, "	char *_ids_[MAXPROC+MAXQ+4];\n");
	fprintf(fd_th, "#else\n");
	fprintf(fd_th, "	uchar sv[VECTORSZ];\n");
	fprintf(fd_th, "#endif\n");

	fprintf(fd_th, "} State");
#ifdef SOLARIS
	fprintf(fd_th,"\n#ifdef GCC\n");
	fprintf(fd_th, "\t__attribute__ ((aligned(8)))");
	fprintf(fd_th, "\n#endif\n\t");
#endif
	fprintf(fd_th, ";\n\n");

	fprintf(fd_th, "#ifdef TRIX\n");
	fprintf(fd_th, "typedef struct TRIX_v6 {\n");
	fprintf(fd_th, "	uchar *body; /* aligned */\n");
	fprintf(fd_th, "#ifndef BFS\n");
	fprintf(fd_th, "	short modified;\n");
	fprintf(fd_th, "#endif\n");
	fprintf(fd_th, "	short psize;\n");
	fprintf(fd_th, "	short parent_pid;\n");
	fprintf(fd_th, "	struct TRIX_v6 *nxt;\n");
	fprintf(fd_th, "} TRIX_v6;\n");
	fprintf(fd_th, "#endif\n\n");

	fprintf(fd_th, "#define HAS_TRACK	%d\n", hastrack);
	if (0 && hastrack)	/* not really a problem */
	{	fprintf(fd_th, "#ifdef BFS_PAR\n");
		fprintf(fd_th, "	#error cannot use BFS_PAR on models with c_track stmnts\n");
		fprintf(fd_th, "#endif\n");
	}
	if (separate != 2)
		dohidden();
}

void
genaddproc(void)
{	ProcList *p;
	int i = 0;

	if (separate == 2) goto shortcut;

	ntimes(fd_tc, nrRdy+1, nrRdy+2, R2); /* +1 for np_ -- was th */

	fprintf(fd_tc, "#ifdef TRIX\n");
	fprintf(fd_tc, "int what_p_size(int);\n");
	fprintf(fd_tc, "int what_q_size(int);\n\n");

	/* the number of processes just changed by 1 (up or down) */
	/* this means that the channel indices move up or down by one slot */
	/* not all new channels may have a valid index yet, but we move */
	/* all of them anyway, as if they existed */
	ntimes(fd_tc, 0, 1, R7a);
	fprintf(fd_tc, "#endif\n\n");

	ntimes(fd_tc, 0, 1, R7b);

	fprintf(fd_tc, "int\naddproc(int calling_pid, int priority, int n");
	for (/* i = 0 */; i < Npars; i++)
		fprintf(fd_tc, ", int par%d", i);

	ntimes(fd_tc, 0, 1, Addp0);
	ntimes(fd_tc, 1, nrRdy+1, R5); /* +1 for np_ */

	if (nclaims > 1)
	{	fprintf(fd_tc, "#ifndef NOCLAIM\n");
		ntimes(fd_tc, nrRdy+1, nrRdy+2, R5);
		fprintf(fd_tc, "#endif\n");
	}

	ntimes(fd_tc, 0, 1, Addp1);

	if (has_provided)
	{	fprintf(fd_tt, "\nint\nprovided(int II, unsigned char ot, ");
		fprintf(fd_tt, "int tt, Trans *t)\n");
		fprintf(fd_tt, "{\n\tswitch(ot) {\n");
	}
shortcut:
	if (nclaims > 1)
	{	multi_init();
	}
	tc_predef_np();
	for (p = ready; p; p = p->nxt)
	{	Pid_nr = p->tn;
		put_pinit(p);
	}
	if (separate == 2) return;

	Pid_nr = 0;
	if (has_provided)
	{	fprintf(fd_tt, "\tdefault: return 1; /* e.g., a claim */\n");
		fprintf(fd_tt, "\t}\n\treturn 0;\n}\n");
	}

	ntimes(fd_tc, i, i+1, R6);
	if (separate == 0)
		ntimes(fd_tc, 1, nrRdy+1, R5); /* +1 for np_ */
	else
		ntimes(fd_tc, 1, nrRdy, R5);
	ntimes(fd_tc, 0, 1, R8a);
}

void
do_locinits(FILE *fd)
{	ProcList *p;

	/* the locinit functions may refer to pptr or qptr */
	fprintf(fd, "#if VECTORSZ>32000\n");
	fprintf(fd, "	extern int \n");
	fprintf(fd, "#else\n");
	fprintf(fd, "	extern short \n");
	fprintf(fd, "#endif\n");
	fprintf(fd, "	*proc_offset, *q_offset;\n");

	for (p = ready; p; p = p->nxt)
	{	c_add_locinit(fd, p->tn, p->n->name);
	}
}

void
genother(void)
{	ProcList *p;

	switch (separate) {
	case 2:
		if (nclaims > 0)
		{	for (p = ready; p; p = p->nxt)
			{	if (p->b == N_CLAIM)
				{	ntimes(fd_tc, p->tn, p->tn+1, R0); /* claims only */
					fprintf(fd_tc, "#ifdef HAS_CODE\n");
					ntimes(fd_tc, p->tn, p->tn+1, R00);
					fprintf(fd_tc, "#endif\n");
		}	}	}
		break;
	case 1:
		ntimes(fd_tc,     0,    1, Code0);
		for (p = ready; p; p = p->nxt)
		{	if (p->b != N_CLAIM)
			{	ntimes(fd_tc, p->tn, p->tn+1, R0); /* all except claims */
				fprintf(fd_tc, "#ifdef HAS_CODE\n");
				ntimes(fd_tc, p->tn, p->tn+1, R00);
				fprintf(fd_tc, "#endif\n");
		}	}
		break;
	case 0:
		ntimes(fd_tc,     0,    1, Code0);
		ntimes(fd_tc,     0, nrRdy+1, R0); /* +1 for np_ */
		fprintf(fd_tc, "#ifdef HAS_CODE\n");
		ntimes(fd_tc,     0, nrRdy+1, R00); /* +1 for np_ */
		fprintf(fd_tc, "#endif\n");
		break;
	}
	/* new place, make sure Maxbody is set to its final value here */
	fprintf(fd_tc, "\n");

	if (separate != 2)
	{	ntimes(fd_tc, 1, u_sync+u_async+1, R3); /* nrqs is still 0 */
		fprintf(fd_tc, "\tMaxbody = max(Maxbody, sizeof(State)-VECTORSZ);\n");
		fprintf(fd_tc, "\tif ((Maxbody %% WS) != 0)\n");
		fprintf(fd_tc, "\t	Maxbody += WS - (Maxbody %% WS);\n\n");
	}

	for (p = ready; p; p = p->nxt)
		end_labs(p->n, p->tn);

	switch (separate) {
	case 2:
		if (nclaims > 0)
		{	for (p = ready; p; p = p->nxt)
			{	if (p->b == N_CLAIM)
				{	ntimes(fd_tc, p->tn, p->tn+1, R0a); /* claims only */
		}	}	}
		return;
	case 1:
		for (p = ready; p; p = p->nxt)
		{	if (p->b != N_CLAIM)
			{	ntimes(fd_tc, p->tn, p->tn+1, R0a); /* all except claims */
		}	}
		fprintf(fd_tc, "	if (state_tables)\n");
		fprintf(fd_tc, "		ini_claim(%d, 0);\n", claimnr);	/* the default claim */
		if (acceptors == 0)
		{	acceptors = 1;	/* assume at least 1 acceptstate */
		}
		break;
	case 0:
		ntimes(fd_tc, 0, nrRdy, R0a);	/* all */
		break;
	}

	ntimes(fd_th, acceptors,   acceptors+1,   Code1);
	ntimes(fd_th, progressors, progressors+1, Code3);

	ntimes(fd_tc, 0,     1, Code2a);	/* dfs, bfs */
	ntimes(fd_tc, 0,     1, Code2e);	/* multicore */
	ntimes(fd_tc, 0,     1, Code2c);	/* multicore */
	ntimes(fd_tc, 0,     1, Code2d);

	fprintf(fd_tc, "void\ndo_reach(void)\n{\n");
	ntimes(fd_tc, 0,     nrRdy, R4);
	fprintf(fd_tc, "}\n\n");

	fprintf(fd_tc, "void\niniglobals(int calling_pid)\n{\n");
	if (doglobal("", INIV) > 0)
	{	fprintf(fd_tc, "#ifdef VAR_RANGES\n");
		(void) doglobal("logval(\"", LOGV);
		fprintf(fd_tc, "#endif\n");
	}
	fprintf(fd_tc, "}\n\n");
}

void
gensvmap(void)
{
	ntimes(fd_tc, 0, 1, SvMap);
}

static struct {
	char *s,	*t;		int n,	m,	p;
} ln[] = {
	{"end",  	"stopstate",	3,	0,	0},
	{"progress",	"progstate",	8,	0,	1},
	{"accept",	"accpstate",	6,	1,	0},
	{0,		0,		0,	0,	0},
};

static void
end_labs(Symbol *s, int i)
{	int oln = lineno;
	Symbol *ofn = Fname;
	Label *l;
	int j; char foo[128];

	if ((pid_is_claim(i) && separate == 1)
	|| (!pid_is_claim(i) && separate == 2))
		return;

	for (l = labtab; l; l = l->nxt)
	for (j = 0; ln[j].n; j++)
	{	if (strncmp(l->s->name, ln[j].s, ln[j].n) == 0
		&&  strcmp(l->c->name, s->name) == 0)
		{	fprintf(fd_tc, "\t%s[%d][%d] = 1;\n",
				ln[j].t, i, l->e->seqno);
			acceptors   += ln[j].m;
			progressors += ln[j].p;
			if (l->e->status & D_ATOM)
			{	sprintf(foo, "%s label inside d_step",
					ln[j].s);
				goto complain;
			}
			if (j > 0 && (l->e->status & ATOM))
			{	sprintf(foo, "%s label inside atomic",
					ln[j].s);
		complain:	lineno = l->e->n->ln;
				Fname  = l->e->n->fn;
				printf("spin: %3d:%s, warning, %s - is invisible\n",
					lineno, Fname?Fname->name:"-", foo);
	}	}	}	
	/* visible states -- through remote refs: */
	for (l = labtab; l; l = l->nxt)
		if (l->visible
		&&  strcmp(l->s->context->name, s->name) == 0)
		fprintf(fd_tc, "\tvisstate[%d][%d] = 1;\n",
				i, l->e->seqno);

	lineno = oln;
	Fname  = ofn;
}

void
ntimes(FILE *fd, int n, int m, const char *c[])
{
	int i, j;
	for (j = 0; c[j]; j++)
	for (i = n; i < m; i++)
	{	fprintf(fd, c[j], i, i, i, i, i, i);
		fprintf(fd, "\n");
	}
}

void
prehint(Symbol *s)
{	Lextok *n;

	printf("spin: warning, ");
	if (!s) return;

	n = (s->context != ZS)?s->context->ini:s->ini;
	if (n)
	printf("line %s:%d, ", n->fn->name, n->ln);
}

void
checktype(Symbol *sp, char *s)
{	char buf[128]; int i;

	if (!s
	|| (sp->type != BYTE
	&&  sp->type != SHORT
	&&  sp->type != INT))
		return;

	if (sp->hidden&16)	/* formal parameter */
	{	ProcList *p; Lextok *f, *t;
		int posnr = 0;
		for (p = ready; p; p = p->nxt)
			if (p->n->name
			&&  strcmp(s, p->n->name) == 0)
				break;
		if (p)
		for (f = p->p; f; f = f->rgt) /* list of types */
		for (t = f->lft; t; t = t->rgt, posnr++)
			if (t->sym
			&&  strcmp(t->sym->name, sp->name) == 0)
			{	checkrun(sp, posnr);
				return;
			}

	} else if (!(sp->hidden&4))
	{	if (!(verbose&32)) return;
		sputtype(buf, sp->type);
		i = (int) strlen(buf);
		while (i > 0 && buf[--i] == ' ') buf[i] = '\0';
		prehint(sp);
		if (sp->context)
			printf("proctype %s:", s);
		else
			printf("global");
		printf(" '%s %s' could be declared 'bit %s'\n",
			buf, sp->name, sp->name);
	} else if (sp->type != BYTE && !(sp->hidden&8))
	{	if (!(verbose&32)) return;
		sputtype(buf, sp->type);
		i = (int) strlen(buf);
		while (buf[--i] == ' ') buf[i] = '\0';
		prehint(sp);
		if (sp->context)
			printf("proctype %s:", s);
		else
			printf("global");
		printf(" '%s %s' could be declared 'byte %s'\n",
			buf, sp->name, sp->name);
	}
}

static int
dolocal(FILE *ofd, char *pre, int dowhat, int p, char *s, enum btypes b)
{	int h, j, k=0; extern int nr_errs;
	Ordered *walk;
	Symbol *sp;
	char buf[128], buf2[128], buf3[128];

	if (dowhat == INIV)
	{	/* initialize in order of declaration */
		for (walk = all_names; walk; walk = walk->next)
		{	sp = walk->entry;
			if (sp->context
			&& !sp->owner
			&&  strcmp(s, sp->context->name) == 0)
			{	checktype(sp, s); /* fall through */
				if (!(sp->hidden&16))
				{	sprintf(buf, "((P%d *)pptr(h))->", p);
					do_var(ofd, dowhat, buf, sp, "", " = ", ";\n");
				}
				k++;
		}	}
	} else
	{	for (j = 0; j < 8; j++)
		for (h = 0; h <= 1; h++)
		for (walk = all_names; walk; walk = walk->next)
		{	sp = walk->entry;
			if (sp->context
			&& !sp->owner
			&&  sp->type == Types[j]
			&&  ((h == 0 && (sp->nel == 1 && sp->isarray == 0))
			||   (h == 1 && (sp->nel  > 1 || sp->isarray == 1)))
			&&  strcmp(s, sp->context->name) == 0)
			{	switch (dowhat) {
				case LOGV:
					if (sp->type == CHAN
					&&  verbose == 0)
						break;
					sprintf(buf, "%s%s:", pre, s);
					{ sprintf(buf2, "\", ((P%d *)pptr(h))->", p);
					  sprintf(buf3, ");\n");
					}
					do_var(ofd, dowhat, "", sp, buf, buf2, buf3);
					break;
				case PUTV:
					sprintf(buf, "((P%d *)pptr(h))->", p);
					do_var(ofd, dowhat, buf, sp, "", " = ", ";\n");
					k++;
					break;
				}
				if (b == N_CLAIM)
				{	printf("error: %s defines local %s\n",
						s, sp->name);
					nr_errs++;
	}	}	}	}

	return k;
}

void
c_chandump(FILE *fd)
{	Queue *q;
	char buf[256];
	int i;

	if (!qtab)
	{	fprintf(fd, "void\nc_chandump(int unused)\n");
		fprintf(fd, "{\tunused++; /* avoid complaints */\n}\n");
		return;
	}

	fprintf(fd, "void\nc_chandump(int from)\n");
	fprintf(fd, "{	uchar *z; int slot;\n");

	fprintf(fd, "	from--;\n");
	fprintf(fd, "	if (from >= (int) now._nr_qs || from < 0)\n");
	fprintf(fd, "	{	printf(\"pan: bad qid %%d\\n\", from+1);\n");
	fprintf(fd, "		return;\n");
	fprintf(fd, "	}\n");
	fprintf(fd, "	z = qptr(from);\n");
	fprintf(fd, "	switch (((Q0 *)z)->_t) {\n");

	for (q = qtab; q; q = q->nxt)
	{	fprintf(fd, "	case %d:\n\t\t", q->qid);
		sprintf(buf, "((Q%d *)z)->", q->qid);

		fprintf(fd, "for (slot = 0; slot < %sQlen; slot++)\n\t\t", buf);
		fprintf(fd, "{	printf(\" [\");\n\t\t");
		for (i = 0; i < q->nflds; i++)
		{	if (q->fld_width[i] == MTYPE)
			{	fprintf(fd, "\tprintm(%scontents[slot].fld%d",
					buf, i);
				if (q->mtp[i])
				{	fprintf(fd, ", \"%s\"", q->mtp[i]);
				} else
				{	fprintf(fd, ", 0");
				}
			} else
			{	fprintf(fd, "\tprintf(\"%%d,\", %scontents[slot].fld%d",
					buf, i);
			}
			fprintf(fd, ");\n\t\t");
		}
		fprintf(fd, "	printf(\"],\");\n\t\t");
		fprintf(fd, "}\n\t\t");
		fprintf(fd, "break;\n");
	}
	fprintf(fd, "	}\n");
	fprintf(fd, "	printf(\"\\n\");\n}\n");
}

void
c_var(FILE *fd, char *pref, Symbol *sp)
{	char *ptr, buf[256];
	int i;

	if (!sp)
	{	fatal("cannot happen - c_var", 0);
	}

	ptr = sp->name;
	if (!old_scope_rules)
	{	while (*ptr == '_' || isdigit((int)*ptr))
		{	ptr++;
	}	}

	switch (sp->type) {
	case STRUCT:
		/* c_struct(fd, pref, sp); */
		fprintf(fd, "\t\tprintf(\"\t(struct %s)\\n\");\n",
			sp->name);
		sprintf(buf, "%s%s.", pref, sp->name);
		c_struct(fd, buf, sp);
		break;
	case MTYPE:
	case BIT:   case BYTE:
	case SHORT: case INT:
	case UNSIGNED:
		sputtype(buf, sp->type);
		if (sp->nel == 1 && sp->isarray == 0)
		{
			if (sp->type == MTYPE && ismtype(sp->name))
			{	fprintf(fd, "\tprintf(\"\t%s %s:\t%d\\n\");\n",
					buf, ptr, ismtype(sp->name));
			} else
			{	fprintf(fd, "\tprintf(\"\t%s %s:\t%%d\\n\", %s%s);\n",
					buf, ptr, pref, sp->name);
			}
		} else
		{	fprintf(fd, "\t{\tint l_in;\n");
			fprintf(fd, "\t\tfor (l_in = 0; l_in < %d; l_in++)\n", sp->nel);
			fprintf(fd, "\t\t{\n");
			fprintf(fd, "\t\t\tprintf(\"\t%s %s[%%d]:\t%%d\\n\", l_in, %s%s[l_in]);\n",
						buf, ptr, pref, sp->name);
			fprintf(fd, "\t\t}\n");
			fprintf(fd, "\t}\n");
		}
		break;
	case CHAN:
		if (sp->nel == 1 && sp->isarray == 0)
		{  fprintf(fd, "\tprintf(\"\tchan %s (=%%d):\tlen %%d:\\t\", ", ptr);
		   fprintf(fd, "%s%s, q_len(%s%s));\n",
			pref, sp->name, pref, sp->name);
		   fprintf(fd, "\tc_chandump(%s%s);\n", pref, sp->name);
		} else
		for (i = 0; i < sp->nel; i++)
		{  fprintf(fd, "\tprintf(\"\tchan %s[%d] (=%%d):\tlen %%d:\\t\", ",
			ptr, i);
		   fprintf(fd, "%s%s[%d], q_len(%s%s[%d]));\n",
			pref, sp->name, i, pref, sp->name, i);
		   fprintf(fd, "\tc_chandump(%s%s[%d]);\n",
			pref, sp->name, i);
		}
		break;
	}
}

int
c_splurge_any(ProcList *p)
{	Ordered *walk;
	Symbol *sp;

	if (p->b != N_CLAIM && p->b != E_TRACE && p->b != N_TRACE)
	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if (!sp->context
		||  sp->type == 0
		||  strcmp(sp->context->name, p->n->name) != 0
		||  sp->owner || (sp->hidden&1)
		|| (sp->type == MTYPE && ismtype(sp->name)))
			continue;

		return 1;
	}
	return 0;
}

void
c_splurge(FILE *fd, ProcList *p)
{	Ordered *walk;
	Symbol *sp;
	char pref[64];

	if (p->b != N_CLAIM && p->b != E_TRACE && p->b != N_TRACE)
	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if (!sp->context
		||  sp->type == 0
		||  strcmp(sp->context->name, p->n->name) != 0
		||  sp->owner || (sp->hidden&1)
		|| (sp->type == MTYPE && ismtype(sp->name)))
			continue;

		sprintf(pref, "((P%d *)pptr(pid))->", p->tn);
		c_var(fd, pref, sp);
	}
}

void
c_wrapper(FILE *fd)	/* allow pan.c to print out global sv entries */
{	Ordered  *walk;
	ProcList *p;
	Symbol   *sp;
	Mtypes_t *lst;
	Lextok   *n;
	int	  j;
	extern Mtypes_t *Mtypes;

	fprintf(fd, "void\nc_globals(void)\n{\t/* int i; */\n");
	fprintf(fd, "	printf(\"global vars:\\n\");\n");
	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if (sp->context || sp->owner || (sp->hidden&1))
			continue;
		c_var(fd, "now.", sp);
	}
	fprintf(fd, "}\n");

	fprintf(fd, "void\nc_locals(int pid, int tp)\n{\t/* int i; */\n");
	fprintf(fd, "	switch(tp) {\n");
	for (p = ready; p; p = p->nxt)
	{	fprintf(fd, "	case %d:\n", p->tn);
		if (c_splurge_any(p))
		{	fprintf(fd, "	\tprintf(\"local vars proc %%d (%s):\\n\", pid);\n",
				p->n->name);
			c_splurge(fd, p);
		} else
		{	fprintf(fd, "	\t/* none */\n");
		}
		fprintf(fd, "	\tbreak;\n");
	}
	fprintf(fd, "	}\n}\n");

	fprintf(fd, "void\nprintm(int x, char *s)\n{\n");
	fprintf(fd, "	if (!s) { s = \"_unnamed_\"; }\n");
	for (lst = Mtypes; lst; lst = lst->nxt)
	{	fprintf(fd, "	if (strcmp(s, \"%s\") == 0)\n", lst->nm);
		fprintf(fd, "	switch (x) {\n");
	        for (n = lst->mt, j = 1; n && j; n = n->rgt, j++)
	                fprintf(fd, "\tcase %d: Printf(\"%s\"); return;\n",
				j, n->lft->sym->name);
		fprintf(fd, "	default: Printf(\"%%d\", x); return;\n");
		fprintf(fd, "	}\n");
	}
	fprintf(fd, "}\n");
}

static int
doglobal(char *pre, int dowhat)
{	Ordered *walk;
	Symbol *sp;
	int j, cnt = 0;

	for (j = 0; j < 8; j++)
	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if (!sp->context
		&&  !sp->owner
		&&  sp->type == Types[j])
		{	if (Types[j] != MTYPE || !ismtype(sp->name))
			switch (dowhat) {
			case LOGV:
				if (sp->type == CHAN
				&&  verbose == 0)
					break;
				if (sp->hidden&1)
					break;
				do_var(fd_tc, dowhat, "", sp,
					pre, "\", now.", ");\n");
				break;
			case INIV:
				checktype(sp, (char *) 0);
				cnt++; /* fall through */
			case PUTV:
				do_var(fd_tc, dowhat,
					(sp->hidden&1)?"":"now.", sp,
					"", " = ", ";\n");
				break;
	}	}	}
	return cnt;
}

static void
dohidden(void)
{	Ordered *walk;
	Symbol *sp;
	int j;

	for (j = 0; j < 8; j++)
	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if ((sp->hidden&1)
		&&  sp->type == Types[j])
		{	if (sp->context || sp->owner)
			  fatal("cannot hide non-globals (%s)", sp->name);
			if (sp->type == CHAN)
			  fatal("cannot hide channels (%s)", sp->name);
			fprintf(fd_th, "/* hidden variable: */");
			typ2c(sp);
	}	}
}

void
do_var(FILE *ofd, int dowhat, char *s, Symbol *sp,
	char *pre, char *sep, char *ter)
{	int i;
	char *ptr = sp?sp->name:"";

	if (!sp)
	{	fatal("cannot happen - do_var", 0);
	}

	switch(dowhat) {
	case PUTV:
		if (sp->hidden&1) break;

		typ2c(sp);
		break;

	case LOGV:
		if (!old_scope_rules)
		{	while (*ptr == '_' || isdigit((int)*ptr))
			{	ptr++;
		}	}
		/* fall thru */
	case INIV:
		if (sp->type == STRUCT)
		{	/* struct may contain a chan */
			walk_struct(ofd, dowhat, s, sp, pre, sep, ter);
			break;
		}
		if (!sp->ini && dowhat != LOGV)	/* it defaults to 0 */
			break;
		if (sp->nel == 1 && sp->isarray == 0)
		{	if (dowhat == LOGV)
			{	fprintf(ofd, "\t\t%s%s%s%s",
					pre, s, ptr, sep);
				fprintf(ofd, "%s%s", s, sp->name);
			} else
			{	fprintf(ofd, "\t\t%s%s%s%s",
					pre, s, sp->name, sep);
				do_init(ofd, sp);
			}
			fprintf(ofd, "%s", ter);
		} else
		{	if (sp->ini && sp->ini->ntyp == CHAN)
			{	for (i = 0; i < sp->nel; i++)
				{	fprintf(ofd, "\t\t%s%s%s[%d]%s",
						pre, s, sp->name, i, sep);
					if (dowhat == LOGV)
						fprintf(ofd, "%s%s[%d]",
							s, sp->name, i);
					else
						do_init(ofd, sp);
					fprintf(ofd, "%s", ter);
				}
			} else if (sp->ini)
			{	if (dowhat != LOGV && sp->isarray && sp->ini->ntyp == ',')
				{	Lextok *z, *y;
					z = sp->ini;
					for (i = 0; i < sp->nel; i++)
					{	if (z && z->ntyp == ',')
						{	y = z->lft;
							z = z->rgt;
						} else
						{	y = z;
						}
						fprintf(ofd, "\t\t%s%s%s[%d]%s",
							pre, s, sp->name, i, sep);
						putstmnt(ofd, y, 0);
						fprintf(ofd, "%s", ter);
					}
				} else
				{	fprintf(ofd, "\t{\tint l_in;\n");
					fprintf(ofd, "\t\tfor (l_in = 0; l_in < %d; l_in++)\n",
						sp->nel);
					fprintf(ofd, "\t\t{\n");
					fprintf(ofd, "\t\t\t%s%s%s[l_in]%s",
						pre, s, sp->name, sep);
					if (dowhat == LOGV)
					{	fprintf(ofd, "%s%s[l_in]", s, sp->name);
					} else
					{	putstmnt(ofd, sp->ini, 0);
					}
					fprintf(ofd, "%s", ter);
					fprintf(ofd, "\t\t}\n");
					fprintf(ofd, "\t}\n");
		}	}	}
		break;
	}
}

static void
do_init(FILE *ofd, Symbol *sp)
{	int i; 

	if (sp->ini
	&&  sp->type == CHAN
	&& ((i = qmake(sp)) > 0))
	{	if (sp->ini->ntyp == CHAN)
		{	fprintf(ofd, "addqueue(calling_pid, %d, %d)",
				i, ltab[i-1]->nslots == 0);
		} else
		{	fprintf(ofd, "%d", i);
		}
	} else
	{	putstmnt(ofd, sp->ini, 0);
	}
}

static void
put_ptype(char *s, int i, int m0, int m1, enum btypes b)
{	int k;

	if (b == I_PROC)
	{	fprintf(fd_th, "#define Pinit	((P%d *)_this)\n", i);
	} else if (b == P_PROC || b == A_PROC)
	{	fprintf(fd_th, "#define P%s	((P%d *)_this)\n", s, i);
	}

	fprintf(fd_th, "typedef struct P%d { /* %s */\n", i, s);
	fprintf(fd_th, "	unsigned _pid : 8;  /* 0..255 */\n");
	fprintf(fd_th, "	unsigned _t   : %d; /* proctype */\n", blog(m1));
	fprintf(fd_th, "	unsigned _p   : %d; /* state    */\n", blog(m0));
	fprintf(fd_th, "#ifdef HAS_PRIORITY\n");
	fprintf(fd_th, "	unsigned _priority : 8; /* 0..255 */\n");
	fprintf(fd_th, "#endif\n");
	LstSet = ZS;
	nBits = 8 + blog(m1) + blog(m0);
	k = dolocal(fd_tc, "", PUTV, i, s, b);	/* includes pars */
	c_add_loc(fd_th, s);

	fprintf(fd_th, "} P%d;\n", i);
	if ((!LstSet && k > 0) || has_state)
		fprintf(fd_th, "#define Air%d	0\n\n", i);
	else if (LstSet || k == 0)			/* 5.0, added condition */
	{	fprintf(fd_th, "#define Air%d	(sizeof(P%d) - ", i, i);
		if (k == 0)
		{	fprintf(fd_th, "%d", (nBits+7)/8);
			goto done;
		}
		if ((LstSet->type != BIT && LstSet->type != UNSIGNED)
		||   LstSet->nel != 1)
		{	fprintf(fd_th, "Offsetof(P%d, %s) - %d*sizeof(",
				i, LstSet->name, LstSet->nel);
		}
		switch(LstSet->type) {
		case UNSIGNED:
			fprintf(fd_th, "%d", (nBits+7)/8);
			break;
		case BIT:
			if (LstSet->nel == 1)
			{	fprintf(fd_th, "%d", (nBits+7)/8);
				break;
			}	/* else fall through */
		case MTYPE: case BYTE: case CHAN:
			fprintf(fd_th, "uchar)"); break;
		case SHORT:
			fprintf(fd_th, "short)"); break;
		case INT:
			fprintf(fd_th, "int)"); break;
		default:
			fatal("cannot happen Air %s",
				LstSet->name);
		}
done:		fprintf(fd_th, ")\n\n");
	}
}

static void
tc_predef_np(void)
{
	fprintf(fd_th, "#define _NP_	%d\n", nrRdy);	/* 1+ highest proctype nr */

	fprintf(fd_th, "#define _nstates%d	3 /* np_ */\n", nrRdy);
	fprintf(fd_th, "#define _endstate%d	2 /* np_ */\n\n", nrRdy);
	fprintf(fd_th, "#define _start%d	0 /* np_ */\n", nrRdy);

	fprintf(fd_tc, "\tcase %d:	/* np_ */\n", nrRdy);
	if (separate == 1)
	{	fprintf(fd_tc, "\t\tini_claim(%d, h);\n", nrRdy);
	} else
	{	fprintf(fd_tc, "\t\t((P%d *)pptr(h))->_t = %d;\n", nrRdy, nrRdy);
		fprintf(fd_tc, "\t\t((P%d *)pptr(h))->_p = 0;\n", nrRdy);

		fprintf(fd_tc, "#ifdef HAS_PRIORITY\n");
		fprintf(fd_tc, "\t\t((P%d *)pptr(h))->_priority = priority;\n", nrRdy);
		fprintf(fd_tc, "#endif\n");

		fprintf(fd_tc, "\t\treached%d[0] = 1;\n", nrRdy);
		fprintf(fd_tc, "\t\taccpstate[%d][1] = 1;\n", nrRdy);
	}
	fprintf(fd_tc, "\t\tbreak;\n");
}

static void
multi_init(void)
{	ProcList *p;
	Element	*e;
	int i = nrRdy+1;
	int ini, j;
	int nrc = nclaims;

	fprintf(fd_tc, "#ifndef NOCLAIM\n");
	fprintf(fd_tc, "\tcase %d:	/* claim select */\n", i);
	for (p = ready, j = 0; p; p = p->nxt, j++)
	{	if (p->b == N_CLAIM)
		{	e = p->s->frst;
			ini = huntele(e, e->status, -1)->seqno;

			fprintf(fd_tc, "\t\tspin_c_typ[%d] = %d; /* %s */\n",
				j, p->tn, p->n->name);
			fprintf(fd_tc, "\t\t((P%d *)pptr(h))->c_cur[%d] = %d;\n",
				i, j, ini);
			fprintf(fd_tc, "\t\treached%d[%d]=1;\n", p->tn, ini);

			/* the default initial claim is first one in model */
			if (--nrc == 0)	
			{ fprintf(fd_tc, "\t\t((P%d *)pptr(h))->_t = %d;\n", i, p->tn);
			  fprintf(fd_tc, "\t\t((P%d *)pptr(h))->_p = %d;\n", i, ini);
			  fprintf(fd_tc, "\t\t((P%d *)pptr(h))->_n = %d; /* %s */\n",
				i, j, p->n->name);
			  fprintf(fd_tc, "\t\tsrc_claim = src_ln%d;\n", p->tn);
			  fprintf(fd_tc, "#ifndef BFS\n");
			  fprintf(fd_tc, "\t\tif (whichclaim == -1 && claimname == NULL)\n");
			  fprintf(fd_tc, "\t\t\tprintf(\"pan: ltl formula %s\\n\");\n",
				p->n->name);
			  fprintf(fd_tc, "#endif\n");
			}
	}	}
	fprintf(fd_tc, "\t\tif (whichclaim != -1)\n");
	fprintf(fd_tc, "\t\t{	select_claim(whichclaim);\n");
	fprintf(fd_tc, "\t\t}\n");
	fprintf(fd_tc, "\t\tbreak;\n\n");
	fprintf(fd_tc, "#endif\n");
}

static void
put_pinit(ProcList *P)
{	Lextok	*fp, *fpt, *t;
	Element	*e = P->s->frst;
	Symbol	*s = P->n;
	Lextok	*p = P->p;
	int	 i = P->tn;
	int	ini, j, k;

	if (pid_is_claim(i)
	&&  separate == 1)
	{	fprintf(fd_tc, "\tcase %d:	/* %s */\n", i, s->name);
		fprintf(fd_tc, "\t\tini_claim(%d, h);\n", i);
		fprintf(fd_tc, "\t\tbreak;\n");
		return;
	}
	if (!pid_is_claim(i)
	&&  separate == 2)
		return;

	ini = huntele(e, e->status, -1)->seqno;
	fprintf(fd_th, "#define _start%d	%d\n", i, ini);
	if (i == eventmapnr)
	fprintf(fd_th, "#define start_event	%d\n", ini);

	fprintf(fd_tc, "\tcase %d:	/* %s */\n", i, s->name);

	fprintf(fd_tc, "\t\t((P%d *)pptr(h))->_t = %d;\n", i, i);
	fprintf(fd_tc, "\t\t((P%d *)pptr(h))->_p = %d;\n", i, ini);
	fprintf(fd_tc, "#ifdef HAS_PRIORITY\n");

	fprintf(fd_tc, "\t\t((P%d *)pptr(h))->_priority = priority; /* was: %d */\n",
		i, (P->priority<1)? 1 : P->priority);

	fprintf(fd_tc, "#endif\n");
	fprintf(fd_tc, "\t\treached%d[%d]=1;\n", i, ini);
	if (P->b == N_CLAIM)
	{	fprintf(fd_tc, "\t\tsrc_claim = src_ln%d;\n", i);
	}

	if (has_provided)
	{	fprintf(fd_tt, "\tcase %d: /* %s */\n\t\t", i, s->name);
		if (P->prov)
		{	fprintf(fd_tt, "if (");
			putstmnt(fd_tt, P->prov, 0);
			fprintf(fd_tt, ")\n\t\t\t");
		}
		fprintf(fd_tt, "return 1;\n");
		if (P->prov)
			fprintf(fd_tt, "\t\tbreak;\n");
	}

	fprintf(fd_tc, "\t\t/* params: */\n");
	for (fp  = p, j=0; fp; fp = fp->rgt)
	for (fpt = fp->lft; fpt; fpt = fpt->rgt, j++)
	{	t = (fpt->ntyp == ',') ? fpt->lft : fpt;
		if (t->sym->nel > 1 || t->sym->isarray)
		{	lineno = t->ln;
			Fname  = t->fn;
			fatal("array in parameter list, %s",
			t->sym->name);
		}
		fprintf(fd_tc, "\t\t((P%d *)pptr(h))->", i);
		if (t->sym->type == STRUCT)
		{	if (full_name(fd_tc, t, t->sym, 1))
			{	lineno = t->ln;
				Fname  = t->fn;
				fatal("hidden array in parameter %s",
				t->sym->name);
			}
		} else
			fprintf(fd_tc, "%s", t->sym->name);
		fprintf(fd_tc, " = par%d;\n", j);
	}
	fprintf(fd_tc, "\t\t/* locals: */\n");
	k = dolocal(fd_tc, "", INIV, i, s->name, P->b);
	if (k > 0)
	{	fprintf(fd_tc, "#ifdef VAR_RANGES\n");
		(void) dolocal(fd_tc, "logval(\"", LOGV, i, s->name, P->b);
		fprintf(fd_tc, "#endif\n");
	}

	fprintf(fd_tc, "#ifdef HAS_CODE\n");
	fprintf(fd_tc, "\t\tlocinit%d(h);\n", i);
	fprintf(fd_tc, "#endif\n");

	dumpclaims(fd_tc, i, s->name);
	fprintf(fd_tc, "\t	break;\n");
}

Element *
huntstart(Element *f)
{	Element *e = f;
	Element *elast = (Element *) 0;
	int cnt = 0;

	while (elast != e && cnt++ < 200)	/* new 4.0.8 */
	{	elast = e;
		if (e->n)
		{	if (e->n->ntyp == '.' && e->nxt)
				e = e->nxt;
			else if (e->n->ntyp == UNLESS)
				e = e->sub->this->frst;
	}	}

	if (cnt >= 200 || !e)
	{	lineno = (f && f->n)?f->n->ln:lineno;
		fatal("confusing control. structure", (char *) 0);
	}
	return e;
}

Element *
huntele(Element *f, unsigned int o, int stopat)
{	Element *g, *e = f;
	int cnt=0; /* a precaution against loops */

	if (e)
	for ( ; cnt < 500 && e->n; cnt++)
	{
		if (e->seqno == stopat)
			break;

		switch (e->n->ntyp) {
		case GOTO:
			g = get_lab(e->n,1);
			if (e == g)
			{	lineno = (f && f->n)?f->n->ln:lineno;
				fatal("infinite goto loop", (char *) 0);
			}
			cross_dsteps(e->n, g->n);
			break;
		case '.':
		case BREAK:
			if (!e->nxt)
				return e;
			g = e->nxt;
			break;
		case UNLESS:
			g = huntele(e->sub->this->frst, o, stopat);
			if (!g)
			{	fatal("unexpected error 1", (char *) 0);
			}
			break;
		case D_STEP:
		case ATOMIC:
		case NON_ATOMIC:
		default:
			return e;
		}
		if ((o & ATOM) && !(g->status & ATOM))
			return e;
		e = g;
	}
	if (cnt >= 500 || !e)
	{	lineno = (f && f->n)?f->n->ln:lineno;
		fatal("confusing control structure", (char *) 0);
	}
	return e;
}

void
typ2c(Symbol *sp)
{	int wsbits = sizeof(long)*8; /* wordsize in bits */
	switch (sp->type) {
	case UNSIGNED:
		if (sp->hidden&1)
			fprintf(fd_th, "\tuchar %s;", sp->name);
		else
			fprintf(fd_th, "\tunsigned %s : %d",
				sp->name, sp->nbits);
		LstSet = sp;
		if (nBits%wsbits > 0
		&&  wsbits - nBits%wsbits < sp->nbits)
		{	/* must padd to a word-boundary */
			nBits += wsbits - nBits%wsbits;
		}
		nBits += sp->nbits;
		break;
	case BIT:
		if (sp->nel == 1 && sp->isarray == 0 && !(sp->hidden&1))
		{	fprintf(fd_th, "\tunsigned %s : 1", sp->name);
			LstSet = sp; 
			nBits++;
			break;
		} /* else fall through */
		if (!(sp->hidden&1) && (verbose&32))
		printf("spin: warning: bit-array %s[%d] mapped to byte-array\n",
			sp->name, sp->nel);
		nBits += 8*sp->nel; /* mapped onto array of uchars */
	case MTYPE:
	case BYTE:
	case CHAN:	/* good for up to 255 channels */
		fprintf(fd_th, "\tuchar %s", sp->name);
		LstSet = sp;
		break;
	case SHORT:
		fprintf(fd_th, "\tshort %s", sp->name);
		LstSet = sp;
		break;
	case INT:
		fprintf(fd_th, "\tint %s", sp->name);
		LstSet = sp;
		break;
	case STRUCT:
		if (!sp->Snm)
			fatal("undeclared structure element %s", sp->name);
		fprintf(fd_th, "\tstruct %s %s",
			sp->Snm->name,
			sp->name);
		LstSet = ZS;
		break;
	case CODE_FRAG:
	case PREDEF:
		return;
	default:
		fatal("variable %s undeclared", sp->name);
	}

	if (sp->nel > 1 || sp->isarray)
		fprintf(fd_th, "[%d]", sp->nel);
	fprintf(fd_th, ";\n");
}

static void
ncases(FILE *fd, int p, int n, int m, const char *c[])
{	int i, j;

	for (j = 0; c[j]; j++)
	for (i = n; i < m; i++)
	{	fprintf(fd, c[j], i, p, i);
		fprintf(fd, "\n");
	}
}

void
qlen_type(int qmax)
{
	fprintf(fd_th, "\t");
	if (qmax < 256)
		fprintf(fd_th, "uchar");
	else if (qmax < 65535)
		fprintf(fd_th, "ushort");
	else
		fprintf(fd_th, "uint");
	fprintf(fd_th, " Qlen;	/* q_size */\n");
}

void
genaddqueue(void)
{	char buf0[256];
	int j, qmax = 0;
	Queue *q;

	ntimes(fd_tc, 0, 1, Addq0);

	if (has_io && !nrqs)
		fprintf(fd_th, "#define NQS	1 /* nrqs=%d, but has_io */\n", nrqs);
	else
		fprintf(fd_th, "#define NQS	%d\n", nrqs);

	for (q = qtab; q; q = q->nxt)
		if (q->nslots > qmax)
			qmax = q->nslots;

	for (q = qtab; q; q = q->nxt)
	{	j = q->qid;
		fprintf(fd_tc, "\tcase %d: j = sizeof(Q%d);", j, j);
		fprintf(fd_tc, " q_flds[%d] = %d;", j, q->nflds);
		fprintf(fd_tc, " q_max[%d] = %d;", j, max(1,q->nslots));
		fprintf(fd_tc, " break;\n");

		fprintf(fd_th, "typedef struct Q%d {\n", j);
		qlen_type(qmax);	/* 4.2.2 */
		fprintf(fd_th, "	uchar _t;	/* q_type */\n");
		fprintf(fd_th, "	struct {\n");

		for (j = 0; j < q->nflds; j++)
		{	switch (q->fld_width[j]) {
			case BIT:
				if (q->nflds != 1)
				{	fprintf(fd_th, "\t\tunsigned");
					fprintf(fd_th, " fld%d : 1;\n", j);
					break;
				} /* else fall through: smaller struct */
			case MTYPE:
			case CHAN:
			case BYTE:
				fprintf(fd_th, "\t\tuchar fld%d;\n", j);
				break;
			case SHORT:
				fprintf(fd_th, "\t\tshort fld%d;\n", j);
				break;
			case INT:
				fprintf(fd_th, "\t\tint fld%d;\n", j);
				break;
			default:
				fatal("bad channel spec", "");
			}
		}
		fprintf(fd_th, "	} contents[%d];\n", max(1, q->nslots));
		fprintf(fd_th, "} Q%d;\n", q->qid);
	}

	fprintf(fd_th, "typedef struct Q0 {\t/* generic q */\n");
	qlen_type(qmax);	/* 4.2.2 */
	fprintf(fd_th, "	uchar _t;\n");
	fprintf(fd_th, "} Q0;\n");

	ntimes(fd_tc, 0, 1, Addq1);

	fprintf(fd_tc, "#ifdef TRIX\n");
	fprintf(fd_tc, "int\nwhat_p_size(int t)\n{\tint j;\n");
	fprintf(fd_tc, "	switch (t) {\n");
	ntimes(fd_tc, 0, nrRdy+1, R5); /* +1 for np_ */
	fprintf(fd_tc, "	default: Uerror(\"bad proctype\");\n");
	fprintf(fd_tc, "	}\n	return j;\n}\n\n");

	fprintf(fd_tc, "int\nwhat_q_size(int t)\n{\tint j;\n");
	fprintf(fd_tc, "	switch (t) {\n");
	for (j = 0; j < nrqs+1; j++)
	{	fprintf(fd_tc, "	case %d: j = sizeof(Q%d); break;\n", j, j);
	}
	fprintf(fd_tc, "	default: Uerror(\"bad qtype\");\n");
	fprintf(fd_tc, "	}\n	return j;\n}\n");
	fprintf(fd_tc, "#endif\n\n");

	if (has_random)
	{	fprintf(fd_th, "int Q_has(int");
		for (j = 0; j < Mpars; j++)
			fprintf(fd_th, ", int, int");
		fprintf(fd_th, ");\n");

		fprintf(fd_tc, "int\nQ_has(int into");
		for (j = 0; j < Mpars; j++)
			fprintf(fd_tc, ", int want%d, int fld%d", j, j);
		fprintf(fd_tc, ")\n");
		fprintf(fd_tc, "{	int i;\n\n");
		fprintf(fd_tc, "	if (!into--)\n");
		fprintf(fd_tc, "	uerror(\"ref to unknown chan ");
		fprintf(fd_tc, "(recv-poll)\");\n\n");
		fprintf(fd_tc, "	if (into >= now._nr_qs || into < 0)\n");
		fprintf(fd_tc, "		Uerror(\"qrecv bad queue#\");\n\n");
		fprintf(fd_tc, "	for (i = 0; i < ((Q0 *)qptr(into))->Qlen;");
		fprintf(fd_tc, " i++)\n");
		fprintf(fd_tc, "	{\n");
		for (j = 0; j < Mpars; j++)
		{	fprintf(fd_tc, "		if (want%d && ", j);
			fprintf(fd_tc, "qrecv(into+1, i, %d, 0) != fld%d)\n",
				j, j);
			fprintf(fd_tc, "			continue;\n");
		}
		fprintf(fd_tc, "		return i+1;\n");
		fprintf(fd_tc, "	}\n");
		fprintf(fd_tc, "	return 0;\n");
		fprintf(fd_tc, "}\n");
	}

	fprintf(fd_tc, "#if NQS>0\n");
	fprintf(fd_tc, "void\nqsend(int into, int sorted");
	for (j = 0; j < Mpars; j++)
		fprintf(fd_tc, ", int fld%d", j);
	fprintf(fd_tc, ", int args_given)\n");
	ntimes(fd_tc, 0, 1, Addq11);

	for (q = qtab; q; q = q->nxt)
	{	sprintf(buf0, "((Q%d *)z)->", q->qid);
		fprintf(fd_tc, "\tcase %d:%s\n", q->qid,
			(q->nslots)?"":" /* =rv= */");
		if (q->nslots == 0)	/* reset handshake point */
			fprintf(fd_tc, "\t\t(trpt+2)->o_m = 0;\n");

		if (has_sorted)
		{	fprintf(fd_tc, "\t\tif (!sorted) goto append%d;\n", q->qid);
			fprintf(fd_tc, "\t\tfor (j = 0; j < %sQlen; j++)\n", buf0);
			fprintf(fd_tc, "\t\t{\t/* find insertion point */\n");
			sprintf(buf0, "((Q%d *)z)->contents[j].fld", q->qid);
			for (j = 0; j < q->nflds; j++)
			{	fprintf(fd_tc, "\t\t\tif (fld%d > %s%d) continue;\n",
						j, buf0, j);
				fprintf(fd_tc, "\t\t\tif (fld%d < %s%d) ", j, buf0, j);
				fprintf(fd_tc, "goto found%d;\n\n", q->qid);
			}
			fprintf(fd_tc, "\t\t}\n");
			fprintf(fd_tc, "\tfound%d:\n", q->qid);
			sprintf(buf0, "((Q%d *)z)->", q->qid);
			fprintf(fd_tc, "\t\tfor (k = %sQlen - 1; k >= j; k--)\n", buf0);
			fprintf(fd_tc, "\t\t{\t/* shift up */\n");
			for (j = 0; j < q->nflds; j++)
			{	fprintf(fd_tc, "\t\t\t%scontents[k+1].fld%d = ",
					buf0, j);
				fprintf(fd_tc, "%scontents[k].fld%d;\n",
					buf0, j);
			}
			fprintf(fd_tc, "\t\t}\n");
			fprintf(fd_tc, "\tappend%d:\t/* insert in slot j */\n", q->qid);
		}

		fprintf(fd_tc, "#ifdef HAS_SORTED\n");
		fprintf(fd_tc, "\t\t(trpt+1)->ipt = j;\n");	/* ipt was bup.oval */
		fprintf(fd_tc, "#endif\n");
		fprintf(fd_tc, "\t\t%sQlen = %sQlen + 1;\n", buf0, buf0);
		sprintf(buf0, "((Q%d *)z)->contents[j].fld", q->qid);
		for (j = 0; j < q->nflds; j++)
		{	fprintf(fd_tc, "\t\t%s%d = fld%d;", buf0, j, j);
			if (q->fld_width[j] == MTYPE)
			{	fprintf(fd_tc, "\t/* mtype %s */",
					q->mtp[j]?q->mtp[j]:"_unnamed_");
			}
			fprintf(fd_tc, "\n");
		}
		fprintf(fd_tc, "\t\tif (args_given != %d)\n", q->nflds);
		fprintf(fd_tc, "\t\t{	if (args_given > %d)\n", q->nflds);
		fprintf(fd_tc, "\t\t		uerror(\"too many parameters in send stmnt\");\n");
		fprintf(fd_tc, "\t\t	else\n");
		fprintf(fd_tc, "\t\t		uerror(\"too few parameters in send stmnt\");\n");
		fprintf(fd_tc, "\t\t}\n");
		fprintf(fd_tc, "\t\tbreak;\n");
	}
	ntimes(fd_tc, 0, 1, Addq2);

	for (q = qtab; q; q = q->nxt)
	fprintf(fd_tc, "\tcase %d: return %d;\n", q->qid, (!q->nslots));

	ntimes(fd_tc, 0, 1, Addq3);

	for (q = qtab; q; q = q->nxt)
	fprintf(fd_tc, "\tcase %d: return (q_sz(from) == %d);\n",
			q->qid, max(1, q->nslots));

	ntimes(fd_tc, 0, 1, Addq4);
	for (q = qtab; q; q = q->nxt)
	{	sprintf(buf0, "((Q%d *)z)->", q->qid);
		fprintf(fd_tc, "	case %d:%s\n\t\t",
			q->qid, (q->nslots)?"":" /* =rv= */");
		if (q->nflds == 1)
		{	fprintf(fd_tc, "if (fld == 0) r = %s", buf0);
			fprintf(fd_tc, "contents[slot].fld0;\n");
		} else
		{	fprintf(fd_tc, "switch (fld) {\n");
			ncases(fd_tc, q->qid, 0, q->nflds, R12);
			fprintf(fd_tc, "\t\tdefault: Uerror");
			fprintf(fd_tc, "(\"too many fields in recv\");\n");
			fprintf(fd_tc, "\t\t}\n");
		}
		fprintf(fd_tc, "\t\tif (done)\n");
		if (q->nslots == 0)
		{	fprintf(fd_tc, "\t\t{	j = %sQlen - 1;\n",  buf0);
			fprintf(fd_tc, "\t\t	%sQlen = 0;\n", buf0);
			sprintf(buf0, "\t\t\t((Q%d *)z)->contents", q->qid);
		} else
	 	{	fprintf(fd_tc, "\t\t{	j = %sQlen;\n",  buf0);
			fprintf(fd_tc, "\t\t	%sQlen = --j;\n", buf0);
			fprintf(fd_tc, "\t\t	for (k=slot; k<j; k++)\n");
			fprintf(fd_tc, "\t\t	{\n");
			sprintf(buf0, "\t\t\t((Q%d *)z)->contents", q->qid);
			for (j = 0; j < q->nflds; j++)
			{	fprintf(fd_tc, "\t%s[k].fld%d = \n", buf0, j);
				fprintf(fd_tc, "\t\t%s[k+1].fld%d;\n", buf0, j);
			}
			fprintf(fd_tc, "\t\t	}\n");
	 	}

		for (j = 0; j < q->nflds; j++)
			fprintf(fd_tc, "%s[j].fld%d = 0;\n", buf0, j);
		fprintf(fd_tc, "\t\t\tif (fld+1 != %d)\n\t\t\t", q->nflds);
		fprintf(fd_tc, "\tuerror(\"missing pars in receive\");\n");
		/* incompletely received msgs cannot be unrecv'ed */
		fprintf(fd_tc, "\t\t}\n");
		fprintf(fd_tc, "\t\tbreak;\n");
	}
	ntimes(fd_tc, 0, 1, Addq5);
	for (q = qtab; q; q = q->nxt)
	fprintf(fd_tc, "	case %d: j = sizeof(Q%d); break;\n",
		q->qid, q->qid);
	ntimes(fd_tc, 0, 1, R8b);
	ntimes(fd_th, 0, 1, Proto);	/* function prototypes */

	fprintf(fd_th, "void qsend(int, int");
	for (j = 0; j < Mpars; j++)
		fprintf(fd_th, ", int");
	fprintf(fd_th, ", int);\n\n");

	fprintf(fd_th, "#define Addproc(x,y)	addproc(256, y, x");
	/* 256 is param outside the range of valid pids */
	for (j = 0; j < Npars; j++)
		fprintf(fd_th, ", 0");
	fprintf(fd_th, ")\n");
}
