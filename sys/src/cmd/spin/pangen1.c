/***** spin: pangen1.c *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */
/* (c) 2007: small additions for V5.0 to support multi-core verifications */

#include "spin.h"
#include "y.tab.h"
#include "pangen1.h"
#include "pangen3.h"
#include "pangen6.h"
#include <assert.h>

extern FILE	*tc, *th, *tt;
extern Label	*labtab;
extern Ordered	*all_names;
extern ProcList	*rdy;
extern Queue	*qtab;
extern Symbol	*Fname;
extern int	lineno, verbose, Pid, separate, old_scope_rules, nclaims;
extern int	nrRdy, nqs, mst, Mpars, claimnr, eventmapnr;
extern short	has_sorted, has_random, has_provided;
extern Queue	*ltab[];

int	Npars=0, u_sync=0, u_async=0, hastrack = 1;
short	has_io = 0;
short	has_state=0;	/* code contains c_state */

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
	fprintf(th, "   \"%s\",\n", p->n->name);
}
static void
reverse_types(ProcList *p)
{
	if (!p) return;
	reverse_types(p->nxt);
	fprintf(th, "   %d,	/* %s */\n", p->b, p->n->name);
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
	{	putunames(th);
		goto here;
	}
	/* 5.2.3: gcc 3 no longer seems to compute sizeof at compile time */
	fprintf(th, "#define WS		%d /* word size in bytes */\n", (int) sizeof(void *));
	fprintf(th, "#define SYNC	%d\n", u_sync);
	fprintf(th, "#define ASYNC	%d\n\n", u_async);
	fprintf(th, "#ifndef NCORE\n");
	fprintf(th, "	#ifdef DUAL_CORE\n");
	fprintf(th, "		#define NCORE	2\n");
	fprintf(th, "	#elif QUAD_CORE\n");
	fprintf(th, "		#define NCORE	4\n");
	fprintf(th, "	#else\n");
	fprintf(th, "		#define NCORE	1\n");
	fprintf(th, "	#endif\n");
	fprintf(th, "#endif\n");

	putunames(th);

	fprintf(tc, "short Air[] = { ");
	for (p = rdy, i=0; p; p = p->nxt, i++)
		fprintf(tc, "%s (short) Air%d", (p!=rdy)?",":"", i);
	fprintf(tc, ", (short) Air%d", i);	/* np_ */
	if (nclaims > 1)
	{	fprintf(tc, "\n#ifndef NOCLAIM\n");
		fprintf(tc, "	, (short) Air%d", i+1);	/* Multi */
		fprintf(tc, "\n#endif\n\t");
	}
	fprintf(tc, " };\n");

	fprintf(th, "char *procname[] = {\n");
		reverse_names(rdy);
	fprintf(th, "   \":np_:\",\n");
	fprintf(th, "};\n\n");

	fprintf(th, "enum btypes { NONE=%d, N_CLAIM=%d,", NONE, N_CLAIM);
	fprintf(th, " I_PROC=%d, A_PROC=%d,", I_PROC, A_PROC);
	fprintf(th, " P_PROC=%d, E_TRACE=%d, N_TRACE=%d };\n",
		P_PROC, E_TRACE, N_TRACE);
	fprintf(th, "int Btypes[] = {\n");
		reverse_types(rdy);
	fprintf(th, "   0	/* :np_: */\n");
	fprintf(th, "};\n\n");

here:
	for (p = rdy; p; p = p->nxt)
		put_ptype(p->n->name, p->tn, mst, nrRdy+1, p->b);
		/* +1 for np_ */
	put_ptype("np_", nrRdy, mst, nrRdy+1, 0);

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

		i = blog(mst);
		fprintf(th, "\n");
		fprintf(th, "#ifndef NOCLAIM\n");
		fprintf(th, "	#undef VERI\n");
		fprintf(th, "	#define VERI	%d\n", nrRdy+1);
		fprintf(th, "	#define Pclaim	P%d\n\n", nrRdy+1);
		fprintf(th, "typedef struct P%d {\n", nrRdy+1);
		fprintf(th, "	unsigned _pid : 8; /* always zero */\n");
		fprintf(th, "	unsigned _t   : %d; /* active-claim type  */\n",
			blog(nrRdy+1));
		fprintf(th, "	unsigned _p   : %d; /* active-claim state */\n",
			i);
		fprintf(th, "	unsigned _n   : %d; /* active-claim index */\n",
			blog(nclaims));
		if (i <= 255)	/* in stdint.h = UCHAR_MAX from limits.h */
		{	fprintf(th, "	uchar c_cur[NCLAIMS]; /* claim-states */\n");
		} else if (i <= 65535)	/* really USHRT_MAX from limits.h */
		{	fprintf(th, "	ushort c_cur[NCLAIMS]; /* claim-states */\n");
		} else	/* the most unlikely case */
		{	fprintf(th, "	uint c_cur[NCLAIMS]; /* claim-states */\n");
		}
		fprintf(th, "} P%d;\n", nrRdy+1);
		fprintf(th, "uchar spin_c_typ[NCLAIMS]; /* claim-types */\n");
		fprintf(th, "	#define Air%d	(0)\n\n", nrRdy+1);
		fprintf(th, "#endif\n");
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
	{	fprintf(th, "\n#define Pclaim	P0\n");
		fprintf(th, "#ifndef NCLAIMS\n");
		fprintf(th, "	#define NCLAIMS 1\n");
		fprintf(th, "#endif\n");
		fprintf(th, "uchar spin_c_typ[NCLAIMS]; /* claim-types */\n");
	}

	ntimes(th, 0, 1, Head0);

	if (separate != 2)
	{	extern void c_add_stack(FILE *);
		extern void c_stack_size(FILE *);

		ntimes(th, 0, 1, Header);
		fprintf(th, "#define StackSize	(");
			c_stack_size(th);
		fprintf(th, ")\n");

		c_add_stack(th);
		ntimes(th, 0, 1, Header0);
	}
	ntimes(th, 0, 1, Head1);

	LstSet = ZS;
	(void) doglobal("", PUTV);

	hastrack = c_add_sv(th);

	fprintf(th, "#ifdef TRIX\n");
	fprintf(th, "	/* room for 512 proc+chan ptrs, + safety margin */\n");
	fprintf(th, "	char *_ids_[MAXPROC+MAXQ+4];\n");
	fprintf(th, "#else\n");
	fprintf(th, "	uchar sv[VECTORSZ];\n");
	fprintf(th, "#endif\n");

	fprintf(th, "} State");
#ifdef SOLARIS
	fprintf(th,"\n#ifdef GCC\n");
	fprintf(th, "\t__attribute__ ((aligned(8)))");
	fprintf(th, "\n#endif\n\t");
#endif
	fprintf(th, ";\n\n");

	fprintf(th, "#ifdef TRIX\n");
	fprintf(th, "typedef struct TRIX_v6 {\n");
	fprintf(th, "	uchar *body; /* aligned */\n");
	fprintf(th, "#ifndef BFS\n");
	fprintf(th, "	short modified;\n");
	fprintf(th, "#endif\n");
	fprintf(th, "	short psize;\n");
	fprintf(th, "	short parent_pid;\n");
	fprintf(th, "	struct TRIX_v6 *nxt;\n");
	fprintf(th, "} TRIX_v6;\n");
	fprintf(th, "TRIX_v6 *freebodies;\n");
	fprintf(th, "TRIX_v6 *processes[MAXPROC+1];\n");
	fprintf(th, "TRIX_v6 *channels[MAXQ+1]; \n");
	fprintf(th, "long _p_count[MAXPROC];\n");
	fprintf(th, "long _c_count[MAXPROC];\n");
	fprintf(th, "#endif\n\n");

	fprintf(th, "#define HAS_TRACK	%d\n", hastrack);

	if (separate != 2)
		dohidden();
}

void
genaddproc(void)
{	ProcList *p;
	int i = 0, j;

	if (separate == 2) goto shortcut;

	fprintf(tc, "\n#ifdef TRIX\n");
	fprintf(tc, "int what_p_size(int);\n");
	fprintf(tc, "int what_q_size(int);\n\n");
	/* the number of processes just changed by 1 (up or down) */
	/* this means that the channel indices move up or down by one slot */
	/* not all new channels may have a valid index yet, but we move */
	/* all of them anyway, as if they existed */
	fprintf(tc, "void\nre_mark_all(int whichway)\n");
	fprintf(tc, "{	int j;\n");
	fprintf(tc, "	#ifdef V_TRIX\n");
	fprintf(tc, "		printf(\"%%d: re_mark_all channels %%d\\n\", depth, whichway);\n");
	fprintf(tc, "	#endif\n");
	fprintf(tc, "	#ifndef BFS\n");
	fprintf(tc, "	for (j = 0; j < now._nr_qs; j++)\n");
	fprintf(tc, "		channels[j]->modified = 1; /* channel index moved */\n");
	fprintf(tc, "	#endif\n");
	fprintf(tc, "	#ifndef TRIX_ORIG\n");
	fprintf(tc, "	if (whichway > 0)\n");
	fprintf(tc, "	{	for (j = now._nr_pr + now._nr_qs - 1; j >= now._nr_pr; j--)\n");
	fprintf(tc, "			now._ids_[j] = now._ids_[j-1];\n");
	fprintf(tc, "	} else\n");
	fprintf(tc, "	{	for (j = now._nr_pr; j < now._nr_pr + now._nr_qs; j++)\n");
	fprintf(tc, "			now._ids_[j] = now._ids_[j+1];\n");
	fprintf(tc, "	}\n");
	fprintf(tc, "	#endif\n");
	fprintf(tc, "}\n");

	fprintf(tc, "#endif\n\n");

	fprintf(tc, "int\naddproc(int calling_pid, int n");
	for (/* i = 0 */; i < Npars; i++)
		fprintf(tc, ", int par%d", i);

	ntimes(tc, 0, 1, Addp0);
	ntimes(tc, 1, nrRdy+1, R5); /* +1 for np_ */

	if (nclaims > 1)
	{	fprintf(tc, "#ifndef NOCLAIM\n");
		ntimes(tc, nrRdy+1, nrRdy+2, R5);
		fprintf(tc, "#endif\n");
	}

	ntimes(tc, 0, 1, Addp1);

	if (has_provided)
	{	fprintf(tt, "\nint\nprovided(int II, unsigned char ot, ");
		fprintf(tt, "int tt, Trans *t)\n");
		fprintf(tt, "{\n\tswitch(ot) {\n");
	}
shortcut:
	if (nclaims > 1)
	{	multi_init();
	}
	tc_predef_np();
	for (p = rdy; p; p = p->nxt)
	{	Pid = p->tn;
		put_pinit(p);
	}
	if (separate == 2) return;

	Pid = 0;
	if (has_provided)
	{	fprintf(tt, "\tdefault: return 1; /* e.g., a claim */\n");
		fprintf(tt, "\t}\n\treturn 0;\n}\n");
	}

	ntimes(tc, i, i+1, R6);
	if (separate == 0)
		ntimes(tc, 1, nrRdy+1, R5); /* +1 for np_ */
	else
		ntimes(tc, 1, nrRdy, R5);
	ntimes(tc, 0, 1, R8a);
}

void
do_locinits(FILE *fd)
{	ProcList *p;

	for (p = rdy; p; p = p->nxt)
		c_add_locinit(fd, p->tn, p->n->name);
}

void
genother(void)
{	ProcList *p;

	switch (separate) {
	case 2:
		if (nclaims > 0)
		{	for (p = rdy; p; p = p->nxt)
			{	if (p->b == N_CLAIM)
				{	ntimes(tc, p->tn, p->tn+1, R0); /* claims only */
		}	}	}
		break;
	case 1:
		ntimes(tc,     0,    1, Code0);
		for (p = rdy; p; p = p->nxt)
		{	if (p->b != N_CLAIM)
			{	ntimes(tc, p->tn, p->tn+1, R0); /* all except claims */
		}	}
		break;
	case 0:
		ntimes(tc,     0,    1, Code0);
		ntimes(tc,     0, nrRdy+1, R0); /* +1 for np_ */
		break;
	}

	for (p = rdy; p; p = p->nxt)
		end_labs(p->n, p->tn);

	switch (separate) {
	case 2:
		if (nclaims > 0)
		{	for (p = rdy; p; p = p->nxt)
			{	if (p->b == N_CLAIM)
				{	ntimes(tc, p->tn, p->tn+1, R0a); /* claims only */
		}	}	}
		return;
	case 1:
		for (p = rdy; p; p = p->nxt)
		{	if (p->b != N_CLAIM)
			{	ntimes(tc, p->tn, p->tn+1, R0a); /* all except claims */
		}	}
		fprintf(tc, "	if (state_tables)\n");
		fprintf(tc, "		ini_claim(%d, 0);\n", claimnr);	/* the default claim */
		if (acceptors == 0)
		{	acceptors = 1;	/* assume at least 1 acceptstate */
		}
		break;
	case 0:
		ntimes(tc, 0, nrRdy, R0a);	/* all */
		break;
	}

	ntimes(th, acceptors,   acceptors+1,   Code1);
	ntimes(th, progressors, progressors+1, Code3);
	ntimes(th, nrRdy+1, nrRdy+2, R2); /* +1 for np_ */

	ntimes(tc, 0,     1, Code2a);	/* dfs, bfs */
	ntimes(tc, 0,     1, Code2c);	/* multicore */
	ntimes(tc, 0,     1, Code2d);

	fprintf(tc, "void\ndo_reach(void)\n{\n");
	ntimes(tc, 0,     nrRdy, R4);
	fprintf(tc, "}\n\n");

	fprintf(tc, "void\niniglobals(int calling_pid)\n{\n");
	ntimes(tc, 1, u_sync+u_async+1, R3); /* because nqs is still 0 */
	fprintf(tc, "\tMaxbody = max(Maxbody, sizeof(State)-VECTORSZ);\n");
	fprintf(tc, "\tif ((Maxbody %% WS) != 0)\n");
	fprintf(tc, "\t	Maxbody += WS - (Maxbody %% WS);\n\n");

	/* after the value of Maxbody has settled */
	if (doglobal("", INIV) > 0)
	{	fprintf(tc, "#ifdef VAR_RANGES\n");
		(void) doglobal("logval(\"", LOGV);
		fprintf(tc, "#endif\n");
	}
	fprintf(tc, "}\n\n");
}

void
gensvmap(void)
{
	ntimes(tc, 0, 1, SvMap);
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
		{	fprintf(tc, "\t%s[%d][%d] = 1;\n",
				ln[j].t, i, l->e->seqno);
			acceptors += ln[j].m;
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
		fprintf(tc, "\tvisstate[%d][%d] = 1;\n",
				i, l->e->seqno);

	lineno = oln;
	Fname  = ofn;
}

void
ntimes(FILE *fd, int n, int m, char *c[])
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
		for (p = rdy; p; p = p->nxt)
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
	{	fprintf(fd, "void\nc_chandump(int unused) ");
		fprintf(fd, "{ unused++; /* avoid complaints */ }\n");
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
			{	fprintf(fd, "\tprintm(%scontents[slot].fld%d);\n\t\t",
				buf, i);
			} else
			fprintf(fd, "\tprintf(\"%%d,\", %scontents[slot].fld%d);\n\t\t",
				buf, i);
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

	ptr = sp?sp->name:"";
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
	case BIT:   case BYTE:
	case SHORT: case INT:
	case UNSIGNED:
		sputtype(buf, sp->type);
		if (sp->nel == 1 && sp->isarray == 0)
		{	fprintf(fd, "\tprintf(\"\t%s %s:\t%%d\\n\", %s%s);\n",
				buf, ptr, pref, sp->name);
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
{	Ordered *walk;
	ProcList *p;
	Symbol *sp;
	Lextok *n;
	extern Lextok *Mtype;
	int j;

	fprintf(fd, "void\nc_globals(void)\n{\t/* int i; */\n");
	fprintf(fd, "	printf(\"global vars:\\n\");\n");
	for (walk = all_names; walk; walk = walk->next)
	{	sp = walk->entry;
		if (sp->context || sp->owner || (sp->hidden&1)
		|| (sp->type == MTYPE && ismtype(sp->name)))
			continue;

		c_var(fd, "now.", sp);
	}
	fprintf(fd, "}\n");

	fprintf(fd, "void\nc_locals(int pid, int tp)\n{\t/* int i; */\n");
	fprintf(fd, "	switch(tp) {\n");
	for (p = rdy; p; p = p->nxt)
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

	fprintf(fd, "void\nprintm(int x)\n{\n");
	fprintf(fd, "	switch (x) {\n");
        for (n = Mtype, j = 1; n && j; n = n->rgt, j++)
                fprintf(fd, "\tcase %d: Printf(\"%s\"); break;\n",
			j, n->lft->sym->name);
	fprintf(fd, "	default: Printf(\"%%d\", x);\n");
	fprintf(fd, "	}\n");
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
				do_var(tc, dowhat, "", sp,
					pre, "\", now.", ");\n");
				break;
			case INIV:
				checktype(sp, (char *) 0);
				cnt++; /* fall through */
			case PUTV:
				do_var(tc, dowhat,
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
			fprintf(th, "/* hidden variable: */");
			typ2c(sp);
	}	}
	fprintf(th, "int _; /* a predefined write-only variable */\n\n");
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
			} else
			{	fprintf(ofd, "\t{\tint l_in;\n");
				fprintf(ofd, "\t\tfor (l_in = 0; l_in < %d; l_in++)\n", sp->nel);
				fprintf(ofd, "\t\t{\n");
				fprintf(ofd, "\t\t\t%s%s%s[l_in]%s",
						pre, s, sp->name, sep);
				if (dowhat == LOGV)
					fprintf(ofd, "%s%s[l_in]", s, sp->name);
				else
					putstmnt(ofd, sp->ini, 0);
				fprintf(ofd, "%s", ter);
				fprintf(ofd, "\t\t}\n");
				fprintf(ofd, "\t}\n");
		}	}
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
	{	fprintf(th, "#define Pinit	((P%d *)this)\n", i);
	} else if (b == P_PROC || b == A_PROC)
	{	fprintf(th, "#define P%s	((P%d *)this)\n", s, i);
	}

	fprintf(th, "typedef struct P%d { /* %s */\n", i, s);
	fprintf(th, "	unsigned _pid : 8;  /* 0..255 */\n");
	fprintf(th, "	unsigned _t   : %d; /* proctype */\n", blog(m1));
	fprintf(th, "	unsigned _p   : %d; /* state    */\n", blog(m0));
	LstSet = ZS;
	nBits = 8 + blog(m1) + blog(m0);
	k = dolocal(tc, "", PUTV, i, s, b);	/* includes pars */

	c_add_loc(th, s);

	fprintf(th, "} P%d;\n", i);
	if ((!LstSet && k > 0) || has_state)
		fprintf(th, "#define Air%d	0\n", i);
	else if (LstSet || k == 0)			/* 5.0, added condition */
	{	fprintf(th, "#define Air%d	(sizeof(P%d) - ", i, i);
		if (k == 0)
		{	fprintf(th, "%d", (nBits+7)/8);
			goto done;
		}
		if ((LstSet->type != BIT && LstSet->type != UNSIGNED)
		||   LstSet->nel != 1)
		{	fprintf(th, "Offsetof(P%d, %s) - %d*sizeof(",
				i, LstSet->name, LstSet->nel);
		}
		switch(LstSet->type) {
		case UNSIGNED:
			fprintf(th, "%d", (nBits+7)/8);
			break;
		case BIT:
			if (LstSet->nel == 1)
			{	fprintf(th, "%d", (nBits+7)/8);
				break;
			}	/* else fall through */
		case MTYPE: case BYTE: case CHAN:
			fprintf(th, "uchar)"); break;
		case SHORT:
			fprintf(th, "short)"); break;
		case INT:
			fprintf(th, "int)"); break;
		default:
			fatal("cannot happen Air %s",
				LstSet->name);
		}
done:		fprintf(th, ")\n");
	}
}

static void
tc_predef_np(void)
{	int i = nrRdy;	/* 1+ highest proctype nr */

	fprintf(th, "#define _NP_	%d\n", i);
/*	if (separate == 2) fprintf(th, "extern ");	*/
	fprintf(th, "uchar reached%d[3];  /* np_ */\n", i);
	fprintf(th, "uchar *loopstate%d;  /* np_ */\n", i);

	fprintf(th, "#define nstates%d	3 /* np_ */\n", i);
	fprintf(th, "#define endstate%d	2 /* np_ */\n\n", i);
	fprintf(th, "#define start%d	0 /* np_ */\n", i);

	fprintf(tc, "\tcase %d:	/* np_ */\n", i);
	if (separate == 1)
	{	fprintf(tc, "\t\tini_claim(%d, h);\n", i);
	} else
	{	fprintf(tc, "\t\t((P%d *)pptr(h))->_t = %d;\n", i, i);
		fprintf(tc, "\t\t((P%d *)pptr(h))->_p = 0;\n", i);
		fprintf(tc, "\t\treached%d[0] = 1;\n", i);
		fprintf(tc, "\t\taccpstate[%d][1] = 1;\n", i);
	}
	fprintf(tc, "\t\tbreak;\n");
}

static void
multi_init(void)
{	ProcList *p;
	Element	*e;
	int i = nrRdy+1;
	int ini, j;
	int nrc = nclaims;

	fprintf(tc, "#ifndef NOCLAIM\n");
	fprintf(tc, "\tcase %d:	/* claim select */\n", i);
	for (p = rdy, j = 0; p; p = p->nxt, j++)
	{	if (p->b == N_CLAIM)
		{	e = p->s->frst;
			ini = huntele(e, e->status, -1)->seqno;

			fprintf(tc, "\t\tspin_c_typ[%d] = %d; /* %s */\n",
				j, p->tn, p->n->name);
			fprintf(tc, "\t\t((P%d *)pptr(h))->c_cur[%d] = %d;\n",
				i, j, ini);
			fprintf(tc, "\t\treached%d[%d]=1;\n", p->tn, ini);

			/* the default initial claim is first one in model */
			if (--nrc == 0)	
			{ fprintf(tc, "\t\t((P%d *)pptr(h))->_t = %d;\n", i, p->tn);
			  fprintf(tc, "\t\t((P%d *)pptr(h))->_p = %d;\n", i, ini);
			  fprintf(tc, "\t\t((P%d *)pptr(h))->_n = %d; /* %s */\n",
				i, j, p->n->name);
			  fprintf(tc, "\t\tsrc_claim = src_ln%d;\n", p->tn);
			  fprintf(tc, "#ifndef BFS\n");
			  fprintf(tc, "\t\tif (whichclaim == -1 && claimname == NULL)\n");
			  fprintf(tc, "\t\t\tprintf(\"0: Claim %s (%d), from state %d\\n\");\n",
				p->n->name, p->tn, ini);
			  fprintf(tc, "#endif\n");
			}
	}	}
	fprintf(tc, "\t\tif (whichclaim != -1)\n");
	fprintf(tc, "\t\t{	select_claim(whichclaim);\n");
	fprintf(tc, "\t\t}\n");
	fprintf(tc, "\t\tbreak;\n\n");
	fprintf(tc, "#endif\n");
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
	{	fprintf(tc, "\tcase %d:	/* %s */\n", i, s->name);
		fprintf(tc, "\t\tini_claim(%d, h);\n", i);
		fprintf(tc, "\t\tbreak;\n");
		return;
	}
	if (!pid_is_claim(i)
	&&  separate == 2)
		return;

	ini = huntele(e, e->status, -1)->seqno;
	fprintf(th, "#define start%d	%d\n", i, ini);
	if (i == eventmapnr)
	fprintf(th, "#define start_event	%d\n", ini);

	fprintf(tc, "\tcase %d:	/* %s */\n", i, s->name);

	fprintf(tc, "\t\t((P%d *)pptr(h))->_t = %d;\n", i, i);
	fprintf(tc, "\t\t((P%d *)pptr(h))->_p = %d;\n", i, ini);
	fprintf(tc, "\t\treached%d[%d]=1;\n", i, ini);
	if (P->b == N_CLAIM)
	{	fprintf(tc, "\t\tsrc_claim = src_ln%d;\n", i);
	}

	if (has_provided)
	{	fprintf(tt, "\tcase %d: /* %s */\n\t\t", i, s->name);
		if (P->prov)
		{	fprintf(tt, "if (");
			putstmnt(tt, P->prov, 0);
			fprintf(tt, ")\n\t\t\t");
		}
		fprintf(tt, "return 1;\n");
		if (P->prov)
			fprintf(tt, "\t\tbreak;\n");
	}

	fprintf(tc, "\t\t/* params: */\n");
	for (fp  = p, j=0; fp; fp = fp->rgt)
	for (fpt = fp->lft; fpt; fpt = fpt->rgt, j++)
	{	t = (fpt->ntyp == ',') ? fpt->lft : fpt;
		if (t->sym->nel > 1 || t->sym->isarray)
		{	lineno = t->ln;
			Fname  = t->fn;
			fatal("array in parameter list, %s",
			t->sym->name);
		}
		fprintf(tc, "\t\t((P%d *)pptr(h))->", i);
		if (t->sym->type == STRUCT)
		{	if (full_name(tc, t, t->sym, 1))
			{	lineno = t->ln;
				Fname  = t->fn;
				fatal("hidden array in parameter %s",
				t->sym->name);
			}
		} else
			fprintf(tc, "%s", t->sym->name);
		fprintf(tc, " = par%d;\n", j);
	}
	fprintf(tc, "\t\t/* locals: */\n");
	k = dolocal(tc, "", INIV, i, s->name, P->b);
	if (k > 0)
	{	fprintf(tc, "#ifdef VAR_RANGES\n");
		(void) dolocal(tc, "logval(\"", LOGV, i, s->name, P->b);
		fprintf(tc, "#endif\n");
	}

	fprintf(tc, "#ifdef HAS_CODE\n");
	fprintf(tc, "\t\tlocinit%d(h);\n", i);
	fprintf(tc, "#endif\n");

	dumpclaims(tc, i, s->name);
	fprintf(tc, "\t	break;\n");
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
		fatal("confusing control structure", (char *) 0);
	return e;
}

Element *
huntele(Element *f, int o, int stopat)
{	Element *g, *e = f;
	int cnt=0; /* a precaution against loops */

	if (e)
	for ( ; cnt < 200 && e->n; cnt++)
	{
		if (e->seqno == stopat)
			break;

		switch (e->n->ntyp) {
		case GOTO:
			g = get_lab(e->n,1);
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
	if (cnt >= 200 || !e)
		fatal("confusing control structure", (char *) 0);
	return e;
}

void
typ2c(Symbol *sp)
{	int wsbits = sizeof(long)*8; /* wordsize in bits */
	switch (sp->type) {
	case UNSIGNED:
		if (sp->hidden&1)
			fprintf(th, "\tuchar %s;", sp->name);
		else
			fprintf(th, "\tunsigned %s : %d",
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
		{	fprintf(th, "\tunsigned %s : 1", sp->name);
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
		fprintf(th, "\tuchar %s", sp->name);
		LstSet = sp;
		break;
	case SHORT:
		fprintf(th, "\tshort %s", sp->name);
		LstSet = sp;
		break;
	case INT:
		fprintf(th, "\tint %s", sp->name);
		LstSet = sp;
		break;
	case STRUCT:
		if (!sp->Snm)
			fatal("undeclared structure element %s", sp->name);
		fprintf(th, "\tstruct %s %s",
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
		fprintf(th, "[%d]", sp->nel);
	fprintf(th, ";\n");
}

static void
ncases(FILE *fd, int p, int n, int m, char *c[])
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
	fprintf(th, "\t");
	if (qmax < 256)
		fprintf(th, "uchar");
	else if (qmax < 65535)
		fprintf(th, "ushort");
	else
		fprintf(th, "uint");
	fprintf(th, " Qlen;	/* q_size */\n");
}

void
genaddqueue(void)
{	char buf0[256];
	int j, qmax = 0;
	Queue *q;

	ntimes(tc, 0, 1, Addq0);
	if (has_io && !nqs)
		fprintf(th, "#define NQS	1 /* nqs=%d, but has_io */\n", nqs);
	else
		fprintf(th, "#define NQS	%d\n", nqs);
	fprintf(th, "short q_flds[%d];\n", nqs+1);
	fprintf(th, "short q_max[%d];\n", nqs+1);

	for (q = qtab; q; q = q->nxt)
		if (q->nslots > qmax)
			qmax = q->nslots;

	for (q = qtab; q; q = q->nxt)
	{	j = q->qid;
		fprintf(tc, "\tcase %d: j = sizeof(Q%d);", j, j);
		fprintf(tc, " q_flds[%d] = %d;", j, q->nflds);
		fprintf(tc, " q_max[%d] = %d;", j, max(1,q->nslots));
		fprintf(tc, " break;\n");

		fprintf(th, "typedef struct Q%d {\n", j);
		qlen_type(qmax);	/* 4.2.2 */
		fprintf(th, "	uchar _t;	/* q_type */\n");
		fprintf(th, "	struct {\n");

		for (j = 0; j < q->nflds; j++)
		{	switch (q->fld_width[j]) {
			case BIT:
				if (q->nflds != 1)
				{	fprintf(th, "\t\tunsigned");
					fprintf(th, " fld%d : 1;\n", j);
					break;
				} /* else fall through: smaller struct */
			case MTYPE:
			case CHAN:
			case BYTE:
				fprintf(th, "\t\tuchar fld%d;\n", j);
				break;
			case SHORT:
				fprintf(th, "\t\tshort fld%d;\n", j);
				break;
			case INT:
				fprintf(th, "\t\tint fld%d;\n", j);
				break;
			default:
				fatal("bad channel spec", "");
			}
		}
		fprintf(th, "	} contents[%d];\n", max(1, q->nslots));
		fprintf(th, "} Q%d;\n", q->qid);
	}

	fprintf(th, "typedef struct Q0 {\t/* generic q */\n");
	qlen_type(qmax);	/* 4.2.2 */
	fprintf(th, "	uchar _t;\n");
	fprintf(th, "} Q0;\n");

	ntimes(tc, 0, 1, Addq1);

	fprintf(tc, "#ifdef TRIX\n");
	fprintf(tc, "int\nwhat_p_size(int t)\n{\tint j;\n");
	fprintf(tc, "	switch (t) {\n");
	ntimes(tc, 0, nrRdy+1, R5); /* +1 for np_ */
	fprintf(tc, "	default: Uerror(\"bad proctype\");\n");
	fprintf(tc, "	}\n	return j;\n}\n\n");

	fprintf(tc, "int\nwhat_q_size(int t)\n{\tint j;\n");
	fprintf(tc, "	switch (t) {\n");
	for (j = 0; j < nqs+1; j++)
	{	fprintf(tc, "	case %d: j = sizeof(Q%d); break;\n", j, j);
	}
	fprintf(tc, "	default: Uerror(\"bad qtype\");\n");
	fprintf(tc, "	}\n	return j;\n}\n");
	fprintf(tc, "#endif\n\n");

	if (has_random)
	{	fprintf(th, "int Q_has(int");
		for (j = 0; j < Mpars; j++)
			fprintf(th, ", int, int");
		fprintf(th, ");\n");

		fprintf(tc, "int\nQ_has(int into");
		for (j = 0; j < Mpars; j++)
			fprintf(tc, ", int want%d, int fld%d", j, j);
		fprintf(tc, ")\n");
		fprintf(tc, "{	int i;\n\n");
		fprintf(tc, "	if (!into--)\n");
		fprintf(tc, "	uerror(\"ref to unknown chan ");
		fprintf(tc, "(recv-poll)\");\n\n");
		fprintf(tc, "	if (into >= now._nr_qs || into < 0)\n");
		fprintf(tc, "		Uerror(\"qrecv bad queue#\");\n\n");
		fprintf(tc, "	for (i = 0; i < ((Q0 *)qptr(into))->Qlen;");
		fprintf(tc, " i++)\n");
		fprintf(tc, "	{\n");
		for (j = 0; j < Mpars; j++)
		{	fprintf(tc, "		if (want%d && ", j);
			fprintf(tc, "qrecv(into+1, i, %d, 0) != fld%d)\n",
				j, j);
			fprintf(tc, "			continue;\n");
		}
		fprintf(tc, "		return i+1;\n");
		fprintf(tc, "	}\n");
		fprintf(tc, "	return 0;\n");
		fprintf(tc, "}\n");
	}

	fprintf(tc, "#if NQS>0\n");
	fprintf(tc, "void\nqsend(int into, int sorted");
	for (j = 0; j < Mpars; j++)
		fprintf(tc, ", int fld%d", j);
	fprintf(tc, ", int args_given)\n");
	ntimes(tc, 0, 1, Addq11);

	for (q = qtab; q; q = q->nxt)
	{	sprintf(buf0, "((Q%d *)z)->", q->qid);
		fprintf(tc, "\tcase %d:%s\n", q->qid,
			(q->nslots)?"":" /* =rv= */");
		if (q->nslots == 0)	/* reset handshake point */
			fprintf(tc, "\t\t(trpt+2)->o_m = 0;\n");

		if (has_sorted)
		{	fprintf(tc, "\t\tif (!sorted) goto append%d;\n", q->qid);
			fprintf(tc, "\t\tfor (j = 0; j < %sQlen; j++)\n", buf0);
			fprintf(tc, "\t\t{\t/* find insertion point */\n");
			sprintf(buf0, "((Q%d *)z)->contents[j].fld", q->qid);
			for (j = 0; j < q->nflds; j++)
			{	fprintf(tc, "\t\t\tif (fld%d > %s%d) continue;\n",
						j, buf0, j);
				fprintf(tc, "\t\t\tif (fld%d < %s%d) ", j, buf0, j);
				fprintf(tc, "goto found%d;\n\n", q->qid);
			}
			fprintf(tc, "\t\t}\n");
			fprintf(tc, "\tfound%d:\n", q->qid);
			sprintf(buf0, "((Q%d *)z)->", q->qid);
			fprintf(tc, "\t\tfor (k = %sQlen - 1; k >= j; k--)\n", buf0);
			fprintf(tc, "\t\t{\t/* shift up */\n");
			for (j = 0; j < q->nflds; j++)
			{	fprintf(tc, "\t\t\t%scontents[k+1].fld%d = ",
					buf0, j);
				fprintf(tc, "%scontents[k].fld%d;\n",
					buf0, j);
			}
			fprintf(tc, "\t\t}\n");
			fprintf(tc, "\tappend%d:\t/* insert in slot j */\n", q->qid);
		}

		fprintf(tc, "#ifdef HAS_SORTED\n");
		fprintf(tc, "\t\t(trpt+1)->ipt = j;\n");	/* ipt was bup.oval */
		fprintf(tc, "#endif\n");
		fprintf(tc, "\t\t%sQlen = %sQlen + 1;\n", buf0, buf0);
		sprintf(buf0, "((Q%d *)z)->contents[j].fld", q->qid);
		for (j = 0; j < q->nflds; j++)
			fprintf(tc, "\t\t%s%d = fld%d;\n", buf0, j, j);
		fprintf(tc, "\t\tif (args_given != %d)\n", q->nflds);
		fprintf(tc, "\t\t{	if (args_given > %d)\n", q->nflds);
		fprintf(tc, "\t\t		uerror(\"too many parameters in send stmnt\");\n");
		fprintf(tc, "\t\t	else\n");
		fprintf(tc, "\t\t		uerror(\"too few parameters in send stmnt\");\n");
		fprintf(tc, "\t\t}\n");
		fprintf(tc, "\t\tbreak;\n");
	}
	ntimes(tc, 0, 1, Addq2);

	for (q = qtab; q; q = q->nxt)
	fprintf(tc, "\tcase %d: return %d;\n", q->qid, (!q->nslots));

	ntimes(tc, 0, 1, Addq3);

	for (q = qtab; q; q = q->nxt)
	fprintf(tc, "\tcase %d: return (q_sz(from) == %d);\n",
			q->qid, max(1, q->nslots));

	ntimes(tc, 0, 1, Addq4);
	for (q = qtab; q; q = q->nxt)
	{	sprintf(buf0, "((Q%d *)z)->", q->qid);
		fprintf(tc, "	case %d:%s\n\t\t",
			q->qid, (q->nslots)?"":" /* =rv= */");
		if (q->nflds == 1)
		{	fprintf(tc, "if (fld == 0) r = %s", buf0);
			fprintf(tc, "contents[slot].fld0;\n");
		} else
		{	fprintf(tc, "switch (fld) {\n");
			ncases(tc, q->qid, 0, q->nflds, R12);
			fprintf(tc, "\t\tdefault: Uerror");
			fprintf(tc, "(\"too many fields in recv\");\n");
			fprintf(tc, "\t\t}\n");
		}
		fprintf(tc, "\t\tif (done)\n");
		if (q->nslots == 0)
		{	fprintf(tc, "\t\t{	j = %sQlen - 1;\n",  buf0);
			fprintf(tc, "\t\t	%sQlen = 0;\n", buf0);
			sprintf(buf0, "\t\t\t((Q%d *)z)->contents", q->qid);
		} else
	 	{	fprintf(tc, "\t\t{	j = %sQlen;\n",  buf0);
			fprintf(tc, "\t\t	%sQlen = --j;\n", buf0);
			fprintf(tc, "\t\t	for (k=slot; k<j; k++)\n");
			fprintf(tc, "\t\t	{\n");
			sprintf(buf0, "\t\t\t((Q%d *)z)->contents", q->qid);
			for (j = 0; j < q->nflds; j++)
			{	fprintf(tc, "\t%s[k].fld%d = \n", buf0, j);
				fprintf(tc, "\t\t%s[k+1].fld%d;\n", buf0, j);
			}
			fprintf(tc, "\t\t	}\n");
	 	}

		for (j = 0; j < q->nflds; j++)
			fprintf(tc, "%s[j].fld%d = 0;\n", buf0, j);
		fprintf(tc, "\t\t\tif (fld+1 != %d)\n\t\t\t", q->nflds);
		fprintf(tc, "\tuerror(\"missing pars in receive\");\n");
		/* incompletely received msgs cannot be unrecv'ed */
		fprintf(tc, "\t\t}\n");
		fprintf(tc, "\t\tbreak;\n");
	}
	ntimes(tc, 0, 1, Addq5);
	for (q = qtab; q; q = q->nxt)
	fprintf(tc, "	case %d: j = sizeof(Q%d); break;\n",
		q->qid, q->qid);
	ntimes(tc, 0, 1, R8b);

	ntimes(th, 0, 1, Proto);	/* tag on function prototypes */
	fprintf(th, "void qsend(int, int");
	for (j = 0; j < Mpars; j++)
		fprintf(th, ", int");
	fprintf(th, ", int);\n");

	fprintf(th, "#define Addproc(x)	addproc(256, x");
	/* 256 is param outside the range of valid pids */
	for (j = 0; j < Npars; j++)
		fprintf(th, ", 0");
	fprintf(th, ")\n");
}
