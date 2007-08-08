/***** spin: pangen2.c *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

#include "spin.h"
#include "version.h"
#include "y.tab.h"
#include "pangen2.h"
#include "pangen4.h"
#include "pangen5.h"

#define DELTA	500	/* sets an upperbound on nr of chan names */

#define blurb(fd, e)	{ fprintf(fd, "\n"); if (!merger) fprintf(fd, "\t\t/* %s:%d */\n", \
				e->n->fn->name, e->n->ln); }
#define tr_map(m, e)	{ if (!merger) fprintf(tt, "\t\ttr_2_src(%d, %s, %d);\n", \
				m, e->n->fn->name, e->n->ln); }

extern ProcList	*rdy;
extern RunList	*run;
extern Symbol	*Fname, *oFname, *context;
extern char	*claimproc, *eventmap;
extern int	lineno, verbose, Npars, Mpars;
extern int	m_loss, has_remote, has_remvar, merger, rvopt, separate;
extern int	Ntimeouts, Etimeouts, deadvar;
extern int	u_sync, u_async, nrRdy;
extern int	GenCode, IsGuard, Level, TestOnly;
extern short	has_stack;
extern char	*NextLab[];

FILE	*tc, *th, *tt, *tm, *tb;

int	OkBreak = -1;
short	nocast=0;	/* to turn off casts in lvalues */
short	terse=0;	/* terse printing of varnames */
short	no_arrays=0;
short	has_last=0;	/* spec refers to _last */
short	has_badelse=0;	/* spec contains else combined with chan refs */
short	has_enabled=0;	/* spec contains enabled() */
short	has_pcvalue=0;	/* spec contains pc_value() */
short	has_np=0;	/* spec contains np_ */
short	has_sorted=0;	/* spec contains `!!' (sorted-send) operator */
short	has_random=0;	/* spec contains `??' (random-recv) operator */
short	has_xu=0;	/* spec contains xr or xs assertions */
short	has_unless=0;	/* spec contains unless statements */
short	has_provided=0;	/* spec contains PROVIDED clauses on procs */
short	has_code=0;	/* spec contains c_code, c_expr, c_state */
short	_isok=0;	/* checks usage of predefined variable _ */
short	evalindex=0;	/* evaluate index of var names */
short	withprocname=0;	/* prefix local varnames with procname */
int	mst=0;		/* max nr of state/process */
int	claimnr = -1;	/* claim process, if any */
int	eventmapnr = -1; /* event trace, if any */
int	Pid;		/* proc currently processed */
int	multi_oval;	/* set in merges, used also in pangen4.c */

#define MAXMERGE	256	/* max nr of bups per merge sequence */

static short	CnT[MAXMERGE];
static Lextok	XZ, YZ[MAXMERGE];
static int	didcase, YZmax, YZcnt;

static Lextok	*Nn[2];
static int	Det;	/* set if deterministic */
static int	T_sum, T_mus, t_cyc;
static int	TPE[2], EPT[2];
static int	uniq=1;
static int	multi_needed, multi_undo;
static short	AllGlobal=0;	/* set if process has provided clause */

int	has_global(Lextok *);
static int	getweight(Lextok *);
static int	scan_seq(Sequence *);
static void	genconditionals(void);
static void	mark_seq(Sequence *);
static void	patch_atomic(Sequence *);
static void	put_seq(Sequence *, int, int);
static void	putproc(ProcList *);
static void	Tpe(Lextok *);
extern void	spit_recvs(FILE *, FILE*);

static int
fproc(char *s)
{	ProcList *p;

	for (p = rdy; p; p = p->nxt)
		if (strcmp(p->n->name, s) == 0)
			return p->tn;

	fatal("proctype %s not found", s);
	return -1;
}

static void
reverse_procs(RunList *q)
{
	if (!q) return;
	reverse_procs(q->nxt);
	fprintf(tc, "	Addproc(%d);\n", q->tn);
}

static void
tm_predef_np(void)
{
	fprintf(th, "#define _T5	%d\n", uniq++);
	fprintf(th, "#define _T2	%d\n", uniq++);
	fprintf(tm, "\tcase  _T5:\t/* np_ */\n");

	if (separate == 2)
	fprintf(tm, "\t\tif (!((!(o_pm&4) && !(tau&128))))\n");
	else
	fprintf(tm, "\t\tif (!((!(trpt->o_pm&4) && !(trpt->tau&128))))\n");

	fprintf(tm, "\t\t\tcontinue;\n");
	fprintf(tm, "\t\t/* else fall through */\n");
	fprintf(tm, "\tcase  _T2:\t/* true */\n");
	fprintf(tm, "\t\t_m = 3; goto P999;\n");
}

static void
tt_predef_np(void)
{
	fprintf(tt, "\t/* np_ demon: */\n");
	fprintf(tt, "\ttrans[_NP_] = ");
	fprintf(tt, "(Trans **) emalloc(2*sizeof(Trans *));\n");
	fprintf(tt, "\tT = trans[_NP_][0] = ");
	fprintf(tt, "settr(9997,0,1,_T5,0,\"(np_)\", 1,2,0);\n");
	fprintf(tt, "\t    T->nxt	  = ");
	fprintf(tt, "settr(9998,0,0,_T2,0,\"(1)\",   0,2,0);\n");
	fprintf(tt, "\tT = trans[_NP_][1] = ");
	fprintf(tt, "settr(9999,0,1,_T5,0,\"(np_)\", 1,2,0);\n");
}

static struct {
	char *nm[3];
} Cfile[] = {
	{ { "pan.c", "pan_s.c", "pan_t.c" } },
	{ { "pan.h", "pan_s.h", "pan_t.h" } },
	{ { "pan.t", "pan_s.t", "pan_t.t" } },
	{ { "pan.m", "pan_s.m", "pan_t.m" } },
	{ { "pan.b", "pan_s.b", "pan_t.b" } }
};

void
gensrc(void)
{	ProcList *p;

	if (!(tc = fopen(Cfile[0].nm[separate], "w"))		/* main routines */
	||  !(th = fopen(Cfile[1].nm[separate], "w"))		/* header file   */
	||  !(tt = fopen(Cfile[2].nm[separate], "w"))		/* transition matrix */
	||  !(tm = fopen(Cfile[3].nm[separate], "w"))		/* forward  moves */
	||  !(tb = fopen(Cfile[4].nm[separate], "w")))	/* backward moves */
	{	printf("spin: cannot create pan.[chtmfb]\n");
		alldone(1);
	}

	fprintf(th, "#define Version	\"%s\"\n", Version);
	fprintf(th, "#define Source	\"%s\"\n\n", oFname->name);
	if (separate != 2)
	fprintf(th, "char *TrailFile = Source; /* default */\n");

	fprintf(th, "#if defined(BFS)\n");
	fprintf(th, "#ifndef SAFETY\n");
	fprintf(th, "#define SAFETY\n");
	fprintf(th, "#endif\n");
	fprintf(th, "#ifndef XUSAFE\n");
	fprintf(th, "#define XUSAFE\n");
	fprintf(th, "#endif\n");
	fprintf(th, "#endif\n");

	fprintf(th, "#ifndef uchar\n");
	fprintf(th, "#define uchar	unsigned char\n");
	fprintf(th, "#endif\n");
	fprintf(th, "#ifndef uint\n");
	fprintf(th, "#define uint	unsigned int\n");
	fprintf(th, "#endif\n");

	if (sizeof(void *) > 4)	/* 64 bit machine */
	{	fprintf(th, "#ifndef HASH32\n");
		fprintf(th, "#define HASH64\n");
		fprintf(th, "#endif\n");
	}
#if 0
	if (sizeof(long)==sizeof(int))
		fprintf(th, "#define long	int\n");
#endif
	if (separate == 1 && !claimproc)
	{	Symbol *n = (Symbol *) emalloc(sizeof(Symbol));
		Sequence *s = (Sequence *) emalloc(sizeof(Sequence));
		claimproc = n->name = "_:never_template:_";
		ready(n, ZN, s, 0, ZN);
	}
	if (separate == 2)
	{	if (has_remote)
		{	printf("spin: warning, make sure that the S1 model\n");
			printf("      includes the same remote references\n");
		}
		fprintf(th, "#ifndef NFAIR\n");
		fprintf(th, "#define NFAIR	2	/* must be >= 2 */\n");
		fprintf(th, "#endif\n");
		if (has_last)
		fprintf(th, "#define HAS_LAST	%d\n", has_last);
		goto doless;
	}

	fprintf(th, "#define DELTA	%d\n", DELTA);
	fprintf(th, "#ifdef MA\n");
	fprintf(th, "#if MA==1\n"); /* user typed -DMA without size */
	fprintf(th, "#undef MA\n#define MA	100\n");
	fprintf(th, "#endif\n#endif\n");
	fprintf(th, "#ifdef W_XPT\n");
	fprintf(th, "#if W_XPT==1\n"); /* user typed -DW_XPT without size */
	fprintf(th, "#undef W_XPT\n#define W_XPT 1000000\n");
	fprintf(th, "#endif\n#endif\n");
	fprintf(th, "#ifndef NFAIR\n");
	fprintf(th, "#define NFAIR	2	/* must be >= 2 */\n");
	fprintf(th, "#endif\n");
	if (Ntimeouts)
	fprintf(th, "#define NTIM	%d\n", Ntimeouts);
	if (Etimeouts)
	fprintf(th, "#define ETIM	%d\n", Etimeouts);
	if (has_remvar)
	fprintf(th, "#define REM_VARS	1\n");
	if (has_remote)
	fprintf(th, "#define REM_REFS	%d\n", has_remote); /* not yet used */
	if (has_last)
	fprintf(th, "#define HAS_LAST	%d\n", has_last);
	if (has_sorted)
	fprintf(th, "#define HAS_SORTED	%d\n", has_sorted);
	if (m_loss)
	fprintf(th, "#define M_LOSS\n");
	if (has_random)
	fprintf(th, "#define HAS_RANDOM	%d\n", has_random);
	fprintf(th, "#define HAS_CODE\n");	/* doesn't seem to cause measurable overhead */
	if (has_stack)
	fprintf(th, "#define HAS_STACK\n");
	if (has_enabled)
	fprintf(th, "#define HAS_ENABLED	1\n");
	if (has_unless)
	fprintf(th, "#define HAS_UNLESS	%d\n", has_unless);
	if (has_provided)
	fprintf(th, "#define HAS_PROVIDED	%d\n", has_provided);
	if (has_pcvalue)
	fprintf(th, "#define HAS_PCVALUE	%d\n", has_pcvalue);
	if (has_badelse)
	fprintf(th, "#define HAS_BADELSE	%d\n", has_badelse);
	if (has_enabled
	||  has_pcvalue
	||  has_badelse
	||  has_last)
	{	fprintf(th, "#ifndef NOREDUCE\n");
		fprintf(th, "#define NOREDUCE	1\n");
		fprintf(th, "#endif\n");
	}
	if (has_np)
	fprintf(th, "#define HAS_NP	%d\n", has_np);
	if (merger)
	fprintf(th, "#define MERGED	1\n");

doless:
	fprintf(th, "#ifdef NP	/* includes np_ demon */\n");
	if (!has_np)
	fprintf(th, "#define HAS_NP	2\n");
	fprintf(th, "#define VERI	%d\n",	nrRdy);
	fprintf(th, "#define endclaim	3 /* none */\n");
	fprintf(th, "#endif\n");
	if (claimproc)
	{	claimnr = fproc(claimproc);
		/* NP overrides claimproc */
		fprintf(th, "#if !defined(NOCLAIM) && !defined NP\n");
		fprintf(th, "#define VERI	%d\n",	claimnr);
		fprintf(th, "#define endclaim	endstate%d\n",	claimnr);
		fprintf(th, "#endif\n");
	}
	if (eventmap)
	{	eventmapnr = fproc(eventmap);
		fprintf(th, "#define EVENT_TRACE	%d\n",	eventmapnr);
		fprintf(th, "#define endevent	endstate%d\n",	eventmapnr);
		if (eventmap[2] == 'o')	/* ":notrace:" */
		fprintf(th, "#define NEGATED_TRACE	1\n");
	}

	fprintf(th, "typedef struct S_F_MAP {\n");
	fprintf(th, "	char *fnm; int from; int upto;\n");
	fprintf(th, "} S_F_MAP;\n");

	fprintf(tc, "/*** Generated by %s ***/\n", Version);
	fprintf(tc, "/*** From source: %s ***/\n\n", oFname->name);

	ntimes(tc, 0, 1, Pre0);

	plunk_c_decls(tc);	/* types can be refered to in State */

	switch (separate) {
	case 0:	fprintf(tc, "#include \"pan.h\"\n"); break;
	case 1:	fprintf(tc, "#include \"pan_s.h\"\n"); break;
	case 2:	fprintf(tc, "#include \"pan_t.h\"\n"); break;
	}

	fprintf(tc, "State	A_Root;	/* seed-state for cycles */\n");
	fprintf(tc, "State	now;	/* the full state-vector */\n");
	plunk_c_fcts(tc);	/* State can be used in fcts */

	if (separate != 2)
		ntimes(tc, 0, 1, Preamble);
	else
		fprintf(tc, "extern int verbose; extern long depth;\n");

	fprintf(tc, "#ifndef NOBOUNDCHECK\n");
	fprintf(tc, "#define Index(x, y)\tBoundcheck(x, y, II, tt, t)\n");
	fprintf(tc, "#else\n");
	fprintf(tc, "#define Index(x, y)\tx\n");
	fprintf(tc, "#endif\n");

	c_preview();	/* sets hastrack */

	for (p = rdy; p; p = p->nxt)
		mst = max(p->s->maxel, mst);

	if (separate != 2)
	{	fprintf(tt, "#ifdef PEG\n");
		fprintf(tt, "struct T_SRC {\n");
		fprintf(tt, "	char *fl; int ln;\n");
		fprintf(tt, "} T_SRC[NTRANS];\n\n");
		fprintf(tt, "void\ntr_2_src(int m, char *file, int ln)\n");
		fprintf(tt, "{	T_SRC[m].fl = file;\n");
		fprintf(tt, "	T_SRC[m].ln = ln;\n");
		fprintf(tt, "}\n\n");
		fprintf(tt, "void\nputpeg(int n, int m)\n");
		fprintf(tt, "{	printf(\"%%5d\ttrans %%4d \", m, n);\n");
		fprintf(tt, "	printf(\"file %%s line %%3d\\n\",\n");
		fprintf(tt, "		T_SRC[n].fl, T_SRC[n].ln);\n");
		fprintf(tt, "}\n");
		if (!merger)
		{	fprintf(tt, "#else\n");
			fprintf(tt, "#define tr_2_src(m,f,l)\n");
		}
		fprintf(tt, "#endif\n\n");
		fprintf(tt, "void\nsettable(void)\n{\tTrans *T;\n");
		fprintf(tt, "\tTrans *settr(int, int, int, int, int,");
		fprintf(tt, " char *, int, int, int);\n\n");
		fprintf(tt, "\ttrans = (Trans ***) ");
		fprintf(tt, "emalloc(%d*sizeof(Trans **));\n", nrRdy+1);
				/* +1 for np_ automaton */

		if (separate == 1)
		{
		fprintf(tm, "	if (II == 0)\n");
		fprintf(tm, "	{ _m = step_claim(trpt->o_pm, trpt->tau, tt, ot, t);\n");
		fprintf(tm, "	  if (_m) goto P999; else continue;\n");
		fprintf(tm, "	} else\n");
		}

		fprintf(tm, "#define rand	pan_rand\n");
		fprintf(tm, "#if defined(HAS_CODE) && defined(VERBOSE)\n");
		fprintf(tm, "	printf(\"Pr: %%d Tr: %%d\\n\", II, t->forw);\n");
		fprintf(tm, "#endif\n");
		fprintf(tm, "	switch (t->forw) {\n");
	} else
	{	fprintf(tt, "#ifndef PEG\n");
		fprintf(tt, "#define tr_2_src(m,f,l)\n");
		fprintf(tt, "#endif\n");
		fprintf(tt, "void\nset_claim(void)\n{\tTrans *T;\n");
		fprintf(tt, "\textern Trans ***trans;\n");
		fprintf(tt, "\textern Trans *settr(int, int, int, int, int,");
		fprintf(tt, " char *, int, int, int);\n\n");

		fprintf(tm, "#define rand	pan_rand\n");
		fprintf(tm, "#if defined(HAS_CODE) && defined(VERBOSE)\n");
		fprintf(tm, "	printf(\"Pr: %%d Tr: %%d\\n\", II, forw);\n");
		fprintf(tm, "#endif\n");
		fprintf(tm, "	switch (forw) {\n");
	}

	fprintf(tm, "	default: Uerror(\"bad forward move\");\n");
	fprintf(tm, "	case 0:	/* if without executable clauses */\n");
	fprintf(tm, "		continue;\n");
	fprintf(tm, "	case 1: /* generic 'goto' or 'skip' */\n");
	if (separate != 2)
		fprintf(tm, "		IfNotBlocked\n");
	fprintf(tm, "		_m = 3; goto P999;\n");
	fprintf(tm, "	case 2: /* generic 'else' */\n");
	if (separate == 2)
		fprintf(tm, "		if (o_pm&1) continue;\n");
	else
	{	fprintf(tm, "		IfNotBlocked\n");
		fprintf(tm, "		if (trpt->o_pm&1) continue;\n");
	}
	fprintf(tm, "		_m = 3; goto P999;\n");
	uniq = 3;

	if (separate == 1)
		fprintf(tb, "	if (II == 0) goto R999;\n");

	fprintf(tb, "	switch (t->back) {\n");
	fprintf(tb, "	default: Uerror(\"bad return move\");\n");
	fprintf(tb, "	case  0: goto R999; /* nothing to undo */\n");

	for (p = rdy; p; p = p->nxt)
		putproc(p);


	if (separate != 2)
	{	fprintf(th, "struct {\n");
		fprintf(th, "	int tp; short *src;\n");
		fprintf(th, "} src_all[] = {\n");
		for (p = rdy; p; p = p->nxt)
			fprintf(th, "	{ %d, &src_ln%d[0] },\n",
				p->tn, p->tn);
		fprintf(th, "	{ 0, (short *) 0 }\n");
		fprintf(th, "};\n");
		fprintf(th, "short *frm_st0;\n");	/* records src states for transitions in never claim */
	} else
	{	fprintf(th, "extern short *frm_st0;\n");
	}

	gencodetable(th);

	if (separate != 1)
	{	tm_predef_np();
		tt_predef_np();
	}
	fprintf(tt, "}\n\n");	/* end of settable() */

	fprintf(tm, "#undef rand\n");
	fprintf(tm, "	}\n\n");
	fprintf(tb, "	}\n\n");

	if (separate != 2)
	{	ntimes(tt, 0, 1, Tail);
		genheader();
		if (separate == 1)
		{	fprintf(th, "#define FORWARD_MOVES\t\"pan_s.m\"\n");
			fprintf(th, "#define REVERSE_MOVES\t\"pan_s.b\"\n");
			fprintf(th, "#define SEPARATE\n");
			fprintf(th, "#define TRANSITIONS\t\"pan_s.t\"\n");
			fprintf(th, "extern void ini_claim(int, int);\n");
		} else
		{	fprintf(th, "#define FORWARD_MOVES\t\"pan.m\"\n");
			fprintf(th, "#define REVERSE_MOVES\t\"pan.b\"\n");
			fprintf(th, "#define TRANSITIONS\t\"pan.t\"\n");
		}
		genaddproc();
		genother();
		genaddqueue();
		genunio();
		genconditionals();
		gensvmap();
		if (!run) fatal("no runable process", (char *)0);
		fprintf(tc, "void\n");
		fprintf(tc, "active_procs(void)\n{\n");
			reverse_procs(run);
		fprintf(tc, "}\n");
		ntimes(tc, 0, 1, Dfa);
		ntimes(tc, 0, 1, Xpt);

		fprintf(th, "#define NTRANS	%d\n", uniq);
		fprintf(th, "#ifdef PEG\n");
		fprintf(th, "long peg[NTRANS];\n");
		fprintf(th, "#endif\n");

		if (u_sync && !u_async)
			spit_recvs(th, tc);
	} else
	{	genheader();
		fprintf(th, "#define FORWARD_MOVES\t\"pan_t.m\"\n");
		fprintf(th, "#define REVERSE_MOVES\t\"pan_t.b\"\n");
		fprintf(th, "#define TRANSITIONS\t\"pan_t.t\"\n");
		fprintf(tc, "extern int Maxbody;\n");
		fprintf(tc, "#if VECTORSZ>32000\n");
		fprintf(tc, "extern int proc_offset[];\n");
		fprintf(tc, "#else\n");
		fprintf(tc, "extern short proc_offset[];\n");
		fprintf(tc, "#endif\n");
		fprintf(tc, "extern uchar proc_skip[];\n");
		fprintf(tc, "extern uchar *reached[];\n");
		fprintf(tc, "extern uchar *accpstate[];\n");
		fprintf(tc, "extern uchar *progstate[];\n");
		fprintf(tc, "extern uchar *stopstate[];\n");
		fprintf(tc, "extern uchar *visstate[];\n\n");
		fprintf(tc, "extern short *mapstate[];\n");

		fprintf(tc, "void\nini_claim(int n, int h)\n{");
		fprintf(tc, "\textern State now;\n");
		fprintf(tc, "\textern void set_claim(void);\n\n");
		fprintf(tc, "#ifdef PROV\n");
		fprintf(tc, "#include PROV\n");
		fprintf(tc, "#endif\n");
		fprintf(tc, "\tset_claim();\n");
		genother();
		fprintf(tc, "\n\tswitch (n) {\n");
		genaddproc();
		fprintf(tc, "\t}\n");
		fprintf(tc, "\n}\n");
		fprintf(tc, "int\nstep_claim(int o_pm, int tau, int tt, int ot, Trans *t)\n");
		fprintf(tc, "{	int forw = t->forw; int _m = 0; extern char *noptr; int II=0;\n");
		fprintf(tc, "	extern State now;\n");
		fprintf(tc, "#define continue	return 0\n");
		fprintf(tc, "#include \"pan_t.m\"\n");
		fprintf(tc, "P999:\n\treturn _m;\n}\n");
		fprintf(tc, "#undef continue\n");
		fprintf(tc, "int\nrev_claim(int backw)\n{ return 0; }\n");
		fprintf(tc, "#include TRANSITIONS\n");
	}
	if (separate != 1)
		ntimes(tc, 0, 1, Nvr1);

	if (separate != 2)
	{	c_wrapper(tc);
		c_chandump(tc);
	}
}

static int
find_id(Symbol *s)
{	ProcList *p;

	if (s)
	for (p = rdy; p; p = p->nxt)
		if (s == p->n)
			return p->tn;
	return 0;
}

static void
dolen(Symbol *s, char *pre, int pid, int ai, int qln)
{
	if (ai > 0)
		fprintf(tc, "\n\t\t\t ||    ");
	fprintf(tc, "%s(", pre);
	if (!(s->hidden&1))
	{	if (s->context)
			fprintf(tc, "((P%d *)this)->", pid);
		else
			fprintf(tc, "now.");
	}
	fprintf(tc, "%s", s->name);
	if (qln > 1) fprintf(tc, "[%d]", ai);
	fprintf(tc, ")");
}

struct AA {	char TT[9];	char CC[8]; };

static struct AA BB[4] = {
	{ "Q_FULL_F",	" q_full" },
	{ "Q_FULL_T",	"!q_full" },
	{ "Q_EMPT_F",	" !q_len" },
	{ "Q_EMPT_T",	"  q_len" }
	};

static struct AA DD[4] = {
	{ "Q_FULL_F",	" q_e_f" },	/* empty or full */
	{ "Q_FULL_T",	"!q_full" },
	{ "Q_EMPT_F",	" q_e_f" },
	{ "Q_EMPT_T",	" q_len" }
	};
	/* this reduces the number of cases where 's' and 'r'
	   are considered conditionally safe under the
	   partial order reduction rules;  as a price for
	   this simple implementation, it also affects the
	   cases where nfull and nempty can be considered
	   safe -- since these are labeled the same way as
	   's' and 'r' respectively
	   it only affects reduction, not functionality
	 */

void
bb_or_dd(int j, int which)
{
	if (which)
	{	if (has_unless)
			fprintf(tc, "%s", DD[j].CC);
		else
			fprintf(tc, "%s", BB[j].CC);
	} else
	{	if (has_unless)
			fprintf(tc, "%s", DD[j].TT);
		else
			fprintf(tc, "%s", BB[j].TT);
	}
}

void
Done_case(char *nm, Symbol *z)
{	int j, k;
	int nid = z->Nid;
	int qln = z->nel;

	fprintf(tc, "\t\tcase %d: if (", nid);
	for (j = 0; j < 4; j++)
	{	fprintf(tc, "\t(t->ty[i] == ");
		bb_or_dd(j, 0);
		fprintf(tc, " && (");
		for (k = 0; k < qln; k++)
		{	if (k > 0)
				fprintf(tc, "\n\t\t\t ||    ");
			bb_or_dd(j, 1);
			fprintf(tc, "(%s%s", nm, z->name);
			if (qln > 1)
				fprintf(tc, "[%d]", k);
			fprintf(tc, ")");
		}
		fprintf(tc, "))\n\t\t\t ");
		if (j < 3)
			fprintf(tc, "|| ");
		else
			fprintf(tc, "   ");
	}
	fprintf(tc, ") return 0; break;\n");
}

static void
Docase(Symbol *s, int pid, int nid)
{	int i, j;

	fprintf(tc, "\t\tcase %d: if (", nid);
	for (j = 0; j < 4; j++)
	{	fprintf(tc, "\t(t->ty[i] == ");
		bb_or_dd(j, 0);
		fprintf(tc, " && (");
		if (has_unless)
		{	for (i = 0; i < s->nel; i++)
				dolen(s, DD[j].CC, pid, i, s->nel);
		} else
		{	for (i = 0; i < s->nel; i++)
				dolen(s, BB[j].CC, pid, i, s->nel);
		}
		fprintf(tc, "))\n\t\t\t ");
		if (j < 3)
			fprintf(tc, "|| ");
		else
			fprintf(tc, "   ");
	}
	fprintf(tc, ") return 0; break;\n");
}

static void
genconditionals(void)
{	Symbol *s;
	int last=0, j;
	extern Ordered	*all_names;
	Ordered *walk;

	fprintf(th, "#define LOCAL	1\n");
	fprintf(th, "#define Q_FULL_F	2\n");
	fprintf(th, "#define Q_EMPT_F	3\n");
	fprintf(th, "#define Q_EMPT_T	4\n");
	fprintf(th, "#define Q_FULL_T	5\n");
	fprintf(th, "#define TIMEOUT_F	6\n");
	fprintf(th, "#define GLOBAL	7\n");
	fprintf(th, "#define BAD	8\n");
	fprintf(th, "#define ALPHA_F	9\n");

	fprintf(tc, "int\n");
	fprintf(tc, "q_cond(short II, Trans *t)\n");
	fprintf(tc, "{	int i = 0;\n");
	fprintf(tc, "	for (i = 0; i < 6; i++)\n");
	fprintf(tc, "	{	if (t->ty[i] == TIMEOUT_F) return %s;\n",
					(Etimeouts)?"(!(trpt->tau&1))":"1");
	fprintf(tc, "		if (t->ty[i] == ALPHA_F)\n");
	fprintf(tc, "#ifdef GLOB_ALPHA\n");
	fprintf(tc, "			return 0;\n");
	fprintf(tc, "#else\n\t\t\treturn ");
	fprintf(tc, "(II+1 == (short) now._nr_pr && II+1 < MAXPROC);\n");
	fprintf(tc, "#endif\n");

	/* we switch on the chan name from the spec (as identified by
	 * the corresponding Nid number) rather than the actual qid
	 * because we cannot predict at compile time which specific qid
	 * will be accessed by the statement at runtime.  that is:
	 * we do not know which qid to pass to q_cond at runtime
	 * but we do know which name is used.  if it's a chan array, we
	 * must check all elements of the array for compliance (bummer)
	 */
	fprintf(tc, "		switch (t->qu[i]) {\n");
	fprintf(tc, "		case 0: break;\n");

	for (walk = all_names; walk; walk = walk->next)
	{	s = walk->entry;
		if (s->owner) continue;
		j = find_id(s->context);
		if (s->type == CHAN)
		{	if (last == s->Nid) continue;	/* chan array */
			last = s->Nid;
			Docase(s, j, last);
		} else if (s->type == STRUCT)
		{	/* struct may contain a chan */
			char pregat[128];
			extern void walk2_struct(char *, Symbol *);
			strcpy(pregat, "");
			if (!(s->hidden&1))
			{	if (s->context)
					sprintf(pregat, "((P%d *)this)->",j);
				else
					sprintf(pregat, "now.");
			}
			walk2_struct(pregat, s);
		}
	}
	fprintf(tc, "	\tdefault: Uerror(\"unknown qid - q_cond\");\n");
	fprintf(tc, "	\t\t\treturn 0;\n");
	fprintf(tc, "	\t}\n");
	fprintf(tc, "	}\n");
	fprintf(tc, "	return 1;\n");
	fprintf(tc, "}\n");
}

static void
putproc(ProcList *p)
{	Pid = p->tn;
	Det = p->det;

	if (Pid == claimnr
	&&  separate == 1)
	{	fprintf(th, "extern uchar reached%d[];\n", Pid);
#if 0
		fprintf(th, "extern short nstates%d;\n", Pid);
#else
		fprintf(th, "\n#define nstates%d	%d\t/* %s */\n",
			Pid, p->s->maxel, p->n->name);
#endif
		fprintf(th, "extern short src_ln%d[];\n", Pid);
		fprintf(th, "extern S_F_MAP src_file%d[];\n", Pid);
		fprintf(th, "#define endstate%d	%d\n",
			Pid, p->s->last?p->s->last->seqno:0);
		fprintf(th, "#define src_claim	src_ln%d\n", claimnr);

		return;
	}
	if (Pid != claimnr
	&&  separate == 2)
	{	fprintf(th, "extern short src_ln%d[];\n", Pid);
		return;
	}

	AllGlobal = (p->prov)?1:0;	/* process has provided clause */

	fprintf(th, "\n#define nstates%d	%d\t/* %s */\n",
		Pid, p->s->maxel, p->n->name);
	if (Pid == claimnr)
	fprintf(th, "#define nstates_claim	nstates%d\n", Pid);
	if (Pid == eventmapnr)
	fprintf(th, "#define nstates_event	nstates%d\n", Pid);

	fprintf(th, "#define endstate%d	%d\n",
		Pid, p->s->last->seqno);
	fprintf(tm, "\n		 /* PROC %s */\n", p->n->name);
	fprintf(tb, "\n		 /* PROC %s */\n", p->n->name);
	fprintf(tt, "\n	/* proctype %d: %s */\n", Pid, p->n->name);
	fprintf(tt, "\n	trans[%d] = (Trans **)", Pid);
	fprintf(tt, " emalloc(%d*sizeof(Trans *));\n\n", p->s->maxel);

	if (Pid == eventmapnr)
	{	fprintf(th, "\n#define in_s_scope(x_y3_)	0");
		fprintf(tc, "\n#define in_r_scope(x_y3_)	0");
	}

	put_seq(p->s, 2, 0);
	if (Pid == eventmapnr)
	{	fprintf(th, "\n\n");
		fprintf(tc, "\n\n");
	}
	dumpsrc(p->s->maxel, Pid);
}

static void
addTpe(int x)
{	int i;

	if (x <= 2) return;

	for (i = 0; i < T_sum; i++)
		if (TPE[i] == x)
			return;
	TPE[(T_sum++)%2] = x;
}

static void
cnt_seq(Sequence *s)
{	Element *f;
	SeqList *h;

	if (s)
	for (f = s->frst; f; f = f->nxt)
	{	Tpe(f->n);	/* sets EPT */
		addTpe(EPT[0]);
		addTpe(EPT[1]);
		for (h = f->sub; h; h = h->nxt)
			cnt_seq(h->this);
		if (f == s->last)
			break;
	}
}

static void
typ_seq(Sequence *s)
{
	T_sum = 0;
	TPE[0] = 2; TPE[1] = 0;
	cnt_seq(s);
	if (T_sum > 2)		/* more than one type */
	{	TPE[0] = 5*DELTA;	/* non-mixing */
		TPE[1] = 0;
	}
}

static int
hidden(Lextok *n)
{
	if (n)
	switch (n->ntyp) {
	case  FULL: case  EMPTY:
	case NFULL: case NEMPTY: case TIMEOUT:
		Nn[(T_mus++)%2] = n;
		break;
	case '!': case UMIN: case '~': case ASSERT: case 'c':
		(void) hidden(n->lft);
		break;
	case '/': case '*': case '-': case '+':
	case '%': case LT:  case GT: case '&': case '^':
	case '|': case LE:  case GE:  case NE: case '?':
	case EQ:  case OR:  case AND: case LSHIFT: case RSHIFT:
		(void) hidden(n->lft);
		(void) hidden(n->rgt);
		break;
	}
	return T_mus;
}

static int
getNid(Lextok *n)
{
	if (n->sym
	&&  n->sym->type == STRUCT
	&&  n->rgt && n->rgt->lft)
		return getNid(n->rgt->lft);

	if (!n->sym || n->sym->Nid == 0)
	{	fatal("bad channel name '%s'",
		(n->sym)?n->sym->name:"no name");
	}
	return n->sym->Nid;
}

static int
valTpe(Lextok *n)
{	int res = 2;
	/*
	2 = local
	2+1	    .. 2+1*DELTA = nfull,  's'	- require q_full==false
	2+1+1*DELTA .. 2+2*DELTA = nempty, 'r'	- require q_len!=0
	2+1+2*DELTA .. 2+3*DELTA = empty	- require q_len==0
	2+1+3*DELTA .. 2+4*DELTA = full		- require q_full==true
	5*DELTA = non-mixing (i.e., always makes the selection global)
	6*DELTA = timeout (conditionally safe)
	7*DELTA = @, process deletion (conditionally safe)
	 */
	switch (n->ntyp) { /* a series of fall-thru cases: */
	case   FULL:	res += DELTA;		/* add 3*DELTA + chan nr */
	case  EMPTY:	res += DELTA;		/* add 2*DELTA + chan nr */
	case    'r':
	case NEMPTY:	res += DELTA;		/* add 1*DELTA + chan nr */
	case    's':
	case  NFULL:	res += getNid(n->lft);	/* add channel nr */
			break;

	case TIMEOUT:	res = 6*DELTA; break;
	case '@':	res = 7*DELTA; break;
	default:	break;
	}
	return res;
}

static void
Tpe(Lextok *n)	/* mixing in selections */
{
	EPT[0] = 2; EPT[1] = 0;

	if (!n) return;

	T_mus = 0;
	Nn[0] = Nn[1] = ZN;

	if (n->ntyp == 'c')
	{	if (hidden(n->lft) > 2)
		{	EPT[0] = 5*DELTA; /* non-mixing */
			EPT[1] = 0;
			return;
		}
	} else
		Nn[0] = n;

	if (Nn[0]) EPT[0] = valTpe(Nn[0]);
	if (Nn[1]) EPT[1] = valTpe(Nn[1]);
}

static void
put_escp(Element *e)
{	int n;
	SeqList *x;

	if (e->esc /* && e->n->ntyp != GOTO */ && e->n->ntyp != '.')
	{	for (x = e->esc, n = 0; x; x = x->nxt, n++)
		{	int i = huntele(x->this->frst, e->status, -1)->seqno;
			fprintf(tt, "\ttrans[%d][%d]->escp[%d] = %d;\n",
				Pid, e->seqno, n, i);
			fprintf(tt, "\treached%d[%d] = 1;\n",
				Pid, i);
		}
		for (x = e->esc, n=0; x; x = x->nxt, n++)
		{	fprintf(tt, "	/* escape #%d: %d */\n", n,
				huntele(x->this->frst, e->status, -1)->seqno);
			put_seq(x->this, 2, 0);	/* args?? */
		}
		fprintf(tt, "	/* end-escapes */\n");
	}
}

static void
put_sub(Element *e, int Tt0, int Tt1)
{	Sequence *s = e->n->sl->this;
	Element *g = ZE;
	int a;

	patch_atomic(s);
	putskip(s->frst->seqno);
	g = huntstart(s->frst);
	a = g->seqno;

	if (0) printf("put_sub %d -> %d -> %d\n", e->seqno, s->frst->seqno, a);

	if ((e->n->ntyp == ATOMIC
	||  e->n->ntyp == D_STEP)
	&&  scan_seq(s))
		mark_seq(s);
	s->last->nxt = e->nxt;

	typ_seq(s);	/* sets TPE */

	if (e->n->ntyp == D_STEP)
	{	int inherit = (e->status&(ATOM|L_ATOM));
		fprintf(tm, "\tcase %d: ", uniq++);
		fprintf(tm, "/* STATE %d - line %d %s - [",
			e->seqno, e->n->ln, e->n->fn->name);
		comment(tm, e->n, 0);
		fprintf(tm, "] */\n\t\t");

		if (s->last->n->ntyp == BREAK)
			OkBreak = target(huntele(s->last->nxt,
				s->last->status, -1))->Seqno;
		else
			OkBreak = -1;

		if (!putcode(tm, s, e->nxt, 0, e->n->ln, e->seqno))
		{
			fprintf(tm, "\n#if defined(C_States) && (HAS_TRACK==1)\n");
			fprintf(tm, "\t\tc_update((uchar *) &(now.c_state[0]));\n");
			fprintf(tm, "#endif\n");

			fprintf(tm, "\t\t_m = %d", getweight(s->frst->n));
			if (m_loss && s->frst->n->ntyp == 's')
				fprintf(tm, "+delta_m; delta_m = 0");
			fprintf(tm, "; goto P999;\n\n");
		}
	
		fprintf(tb, "\tcase %d: ", uniq-1);
		fprintf(tb, "/* STATE %d */\n", e->seqno);
		fprintf(tb, "\t\tsv_restor();\n");
		fprintf(tb, "\t\tgoto R999;\n");
		if (e->nxt)
			a = huntele(e->nxt, e->status, -1)->seqno;
		else
			a = 0;
		tr_map(uniq-1, e);
		fprintf(tt, "/*->*/\ttrans[%d][%d]\t= ",
			Pid, e->seqno);
		fprintf(tt, "settr(%d,%d,%d,%d,%d,\"",
			e->Seqno, D_ATOM|inherit, a, uniq-1, uniq-1);
		comment(tt, e->n, e->seqno);
		fprintf(tt, "\", %d, ", (s->frst->status&I_GLOB)?1:0);
		fprintf(tt, "%d, %d);\n", TPE[0], TPE[1]);
		put_escp(e);
	} else
	{	/* ATOMIC or NON_ATOMIC */
		fprintf(tt, "\tT = trans[ %d][%d] = ", Pid, e->seqno);
		fprintf(tt, "settr(%d,%d,0,0,0,\"",
			e->Seqno, (e->n->ntyp == ATOMIC)?ATOM:0);
		comment(tt, e->n, e->seqno);
		if ((e->status&CHECK2)
		||  (g->status&CHECK2))
			s->frst->status |= I_GLOB;
		fprintf(tt, "\", %d, %d, %d);",
			(s->frst->status&I_GLOB)?1:0, Tt0, Tt1);
		blurb(tt, e);
		fprintf(tt, "\tT->nxt\t= ");
		fprintf(tt, "settr(%d,%d,%d,0,0,\"",
			e->Seqno, (e->n->ntyp == ATOMIC)?ATOM:0, a);
		comment(tt, e->n, e->seqno);
		fprintf(tt, "\", %d, ", (s->frst->status&I_GLOB)?1:0);
		if (e->n->ntyp == NON_ATOMIC)
		{	fprintf(tt, "%d, %d);", Tt0, Tt1);
			blurb(tt, e);
			put_seq(s, Tt0, Tt1);
		} else
		{	fprintf(tt, "%d, %d);", TPE[0], TPE[1]);
			blurb(tt, e);
			put_seq(s, TPE[0], TPE[1]);
		}
	}
}

typedef struct CaseCache {
	int m, b, owner;
	Element *e;
	Lextok *n;
	FSM_use *u;
	struct CaseCache *nxt;
} CaseCache;

CaseCache *casing[6];

static int
identical(Lextok *p, Lextok *q)
{
	if ((!p && q) || (p && !q))
		return 0;
	if (!p)
		return 1;

	if (p->ntyp    != q->ntyp
	||  p->ismtyp  != q->ismtyp
	||  p->val     != q->val
	||  p->indstep != q->indstep
	||  p->sym     != q->sym
	||  p->sq      != q->sq
	||  p->sl      != q->sl)
		return 0;

	return	identical(p->lft, q->lft)
	&&	identical(p->rgt, q->rgt);
}

static int
samedeads(FSM_use *a, FSM_use *b)
{	FSM_use *p, *q;

	for (p = a, q = b; p && q; p = p->nxt, q = q->nxt)
		if (p->var != q->var
		||  p->special != q->special)
			return 0;
	return (!p && !q);
}

static Element *
findnext(Element *f)
{	Element *g;

	if (f->n->ntyp == GOTO)
	{	g = get_lab(f->n, 1);
		return huntele(g, f->status, -1);
	}
	return f->nxt;
}

static Element *
advance(Element *e, int stopat)
{	Element *f = e;

	if (stopat)
	while (f && f->seqno != stopat)
	{	f = findnext(f);
		switch (f->n->ntyp) {
		case GOTO:
		case '.':
		case PRINT:
		case PRINTM:
			break;
		default:
			return f;
		}
	}
	return (Element *) 0;
}

static int
equiv_merges(Element *a, Element *b)
{	Element *f, *g;
	int stopat_a, stopat_b;

	if (a->merge_start)
		stopat_a = a->merge_start;
	else
		stopat_a = a->merge;

	if (b->merge_start)
		stopat_b = b->merge_start;
	else
		stopat_b = b->merge;

	if (!stopat_a && !stopat_b)
		return 1;

	for (;;)
	{
		f = advance(a, stopat_a);
		g = advance(b, stopat_b);
		if (!f && !g)
			return 1;
		if (f && g)
			return identical(f->n, g->n);
		else
			return 0;
	}
	return 1;
}

static CaseCache *
prev_case(Element *e, int owner)
{	int j; CaseCache *nc;

	switch (e->n->ntyp) {
	case 'r':	j = 0; break;
	case 's':	j = 1; break;
	case 'c':	j = 2; break;
	case ASGN:	j = 3; break;
	case ASSERT:	j = 4; break;
	default:	j = 5; break;
	}
	for (nc = casing[j]; nc; nc = nc->nxt)
		if (identical(nc->n, e->n)
		&&  samedeads(nc->u, e->dead)
		&&  equiv_merges(nc->e, e)
		&&  nc->owner == owner)
			return nc;

	return (CaseCache *) 0;
}

static void
new_case(Element *e, int m, int b, int owner)
{	int j; CaseCache *nc;

	switch (e->n->ntyp) {
	case 'r':	j = 0; break;
	case 's':	j = 1; break;
	case 'c':	j = 2; break;
	case ASGN:	j = 3; break;
	case ASSERT:	j = 4; break;
	default:	j = 5; break;
	}
	nc = (CaseCache *) emalloc(sizeof(CaseCache));
	nc->owner = owner;
	nc->m = m;
	nc->b = b;
	nc->e = e;
	nc->n = e->n;
	nc->u = e->dead;
	nc->nxt = casing[j];
	casing[j] = nc;
}

static int
nr_bup(Element *e)
{	FSM_use *u;
	Lextok *v;
	int nr = 0;

	switch (e->n->ntyp) {
	case ASGN:
		nr++;
		break;
	case  'r':
		if (e->n->val >= 1)
			nr++;	/* random recv */
		for (v = e->n->rgt; v; v = v->rgt)
		{	if ((v->lft->ntyp == CONST
			||   v->lft->ntyp == EVAL))
				continue;
			nr++;
		}
		break;
	default:
		break;
	}
	for (u = e->dead; u; u = u->nxt)
	{	switch (u->special) {
		case 2:		/* dead after write */
			if (e->n->ntyp == ASGN
			&&  e->n->rgt->ntyp == CONST
			&&  e->n->rgt->val == 0)
				break;
			nr++;
			break;
		case 1:		/* dead after read */
			nr++;
			break;
	}	}
	return nr;
}

static int
nrhops(Element *e)
{	Element *f = e, *g;
	int cnt = 0;
	int stopat;

	if (e->merge_start)
		stopat = e->merge_start;
	else
		stopat = e->merge;
#if 0
	printf("merge: %d merge_start %d - seqno %d\n",
		e->merge, e->merge_start, e->seqno);
#endif
	do {
		cnt += nr_bup(f);

		if (f->n->ntyp == GOTO)
		{	g = get_lab(f->n, 1);
			if (g->seqno == stopat)
				f = g;
			else
				f = huntele(g, f->status, stopat);
		} else
		{
			f = f->nxt;
		}

		if (f && !f->merge && !f->merge_single && f->seqno != stopat)
		{	fprintf(tm, "\n\t\tbad hop %s:%d -- at %d, <",
				f->n->fn->name,f->n->ln, f->seqno);
			comment(tm, f->n, 0);
			fprintf(tm, "> looking for %d -- merge %d:%d:%d\n\t\t",
				stopat, f->merge, f->merge_start, f->merge_single);
		 	break;
		}
	} while (f && f->seqno != stopat);

	return cnt;
}

static void
check_needed(void)
{
	if (multi_needed)
	{	fprintf(tm, "(trpt+1)->bup.ovals = grab_ints(%d);\n\t\t",
			multi_needed);
		multi_undo = multi_needed;
		multi_needed = 0;
	}
}

static void
doforward(FILE *tm, Element *e)
{	FSM_use *u;

	putstmnt(tm, e->n, e->seqno);

	if (e->n->ntyp != ELSE && Det)
	{	fprintf(tm, ";\n\t\tif (trpt->o_pm&1)\n\t\t");
		fprintf(tm, "\tuerror(\"non-determinism in D_proctype\")");
	}
	if (deadvar && !has_code)
	for (u = e->dead; u; u = u->nxt)
	{	fprintf(tm, ";\n\t\t/* dead %d: %s */  ",
			u->special, u->var->name);

		switch (u->special) {
		case 2:		/* dead after write -- lval already bupped */
			if (e->n->ntyp == ASGN)	/* could be recv or asgn */
			{	if (e->n->rgt->ntyp == CONST
				&&  e->n->rgt->val == 0)
					continue;	/* already set to 0 */
			}
			if (e->n->ntyp != 'r')
			{	XZ.sym = u->var;
				fprintf(tm, "\n#ifdef HAS_CODE\n");
				fprintf(tm, "\t\tif (!readtrail)\n");
				fprintf(tm, "#endif\n\t\t\t");
				putname(tm, "", &XZ, 0, " = 0");
				break;
			} /* else fall through */
		case 1:		/* dead after read -- add asgn of rval -- needs bup */
			YZ[YZmax].sym = u->var;	/* store for pan.b */
			CnT[YZcnt]++;		/* this step added bups */
			if (multi_oval)
			{	check_needed();
				fprintf(tm, "(trpt+1)->bup.ovals[%d] = ",
					multi_oval-1);
				multi_oval++;
			} else
				fprintf(tm, "(trpt+1)->bup.oval = ");
			putname(tm, "", &YZ[YZmax], 0, ";\n");
			fprintf(tm, "#ifdef HAS_CODE\n");
			fprintf(tm, "\t\tif (!readtrail)\n");
			fprintf(tm, "#endif\n\t\t\t");
			putname(tm, "", &YZ[YZmax], 0, " = 0");
			YZmax++;
			break;
	}	}
	fprintf(tm, ";\n\t\t");
}

static int
dobackward(Element *e, int casenr)
{
	if (!any_undo(e->n) && CnT[YZcnt] == 0)
	{	YZcnt--;
		return 0;
	}

	if (!didcase)
	{	fprintf(tb, "\n\tcase %d: ", casenr);
		fprintf(tb, "/* STATE %d */\n\t\t", e->seqno);
		didcase++;
	}

	_isok++;
	while (CnT[YZcnt] > 0)	/* undo dead variable resets */
	{	CnT[YZcnt]--;
		YZmax--;
		if (YZmax < 0)
			fatal("cannot happen, dobackward", (char *)0);
		fprintf(tb, ";\n\t/* %d */\t", YZmax);
		putname(tb, "", &YZ[YZmax], 0, " = trpt->bup.oval");
		if (multi_oval > 0)
		{	multi_oval--;
			fprintf(tb, "s[%d]", multi_oval-1);
		}
	}

	if (e->n->ntyp != '.')
	{	fprintf(tb, ";\n\t\t");
		undostmnt(e->n, e->seqno);
	}
	_isok--;

	YZcnt--;
	return 1;
}

static void
lastfirst(int stopat, Element *fin, int casenr)
{	Element *f = fin, *g;

	if (f->n->ntyp == GOTO)
	{	g = get_lab(f->n, 1);
		if (g->seqno == stopat)
			f = g;
		else
			f = huntele(g, f->status, stopat);
	} else
		f = f->nxt;

	if (!f || f->seqno == stopat
	|| (!f->merge && !f->merge_single))
		return;
	lastfirst(stopat, f, casenr);
#if 0
	fprintf(tb, "\n\t/* merge %d -- %d:%d %d:%d:%d (casenr %d)	",
		YZcnt,
		f->merge_start, f->merge,
		f->seqno, f?f->seqno:-1, stopat,
		casenr);
	comment(tb, f->n, 0);
	fprintf(tb, " */\n");
	fflush(tb);
#endif
	dobackward(f, casenr);
}

static int modifier;

static void
lab_transfer(Element *to, Element *from)
{	Symbol *ns, *s = has_lab(from, (1|2|4));
	Symbol *oc;
	int ltp, usedit=0;

	if (!s) return;

	/* "from" could have all three labels -- rename
	 * to prevent jumps to the transfered copies
	 */
	oc = context;	/* remember */
	for (ltp = 1; ltp < 8; ltp *= 2)	/* 1, 2, and 4 */
		if ((s = has_lab(from, ltp)) != (Symbol *) 0)
		{	ns = (Symbol *) emalloc(sizeof(Symbol));
			ns->name = (char *) emalloc((int) strlen(s->name) + 4);
			sprintf(ns->name, "%s%d", s->name, modifier);

			context = s->context;
			set_lab(ns, to);
			usedit++;
		}
	context = oc;	/* restore */
	if (usedit)
	{	if (modifier++ > 990)
			fatal("modifier overflow error", (char *) 0);
	}
}

static int
case_cache(Element *e, int a)
{	int bupcase = 0, casenr = uniq, fromcache = 0;
	CaseCache *Cached = (CaseCache *) 0;
	Element *f, *g;
	int j, nrbups, mark, target;
	extern int ccache;

	mark = (e->status&ATOM); /* could lose atomicity in a merge chain */

	if (e->merge_mark > 0
	||  (merger && e->merge_in == 0))
	{	/* state nominally unreachable (part of merge chains) */
		if (e->n->ntyp != '.'
		&&  e->n->ntyp != GOTO)
		{	fprintf(tt, "\ttrans[%d][%d]\t= ", Pid, e->seqno);
			fprintf(tt, "settr(0,0,0,0,0,\"");
			comment(tt, e->n, e->seqno);
			fprintf(tt, "\",0,0,0);\n");
		} else
		{	fprintf(tt, "\ttrans[%d][%d]\t= ", Pid, e->seqno);
			casenr = 1; /* mhs example */
			j = a;
			goto haveit; /* pakula's example */
		}

		return -1;
	}

	fprintf(tt, "\ttrans[%d][%d]\t= ", Pid, e->seqno);

	if (ccache
	&&  Pid != claimnr
	&&  Pid != eventmapnr
	&& (Cached = prev_case(e, Pid)))
	{	bupcase = Cached->b;
		casenr  = Cached->m;
		fromcache = 1;

		fprintf(tm, "/* STATE %d - line %d %s - [",
			e->seqno, e->n->ln, e->n->fn->name);
		comment(tm, e->n, 0);
		fprintf(tm, "] (%d:%d - %d) same as %d (%d:%d - %d) */\n",
			e->merge_start, e->merge, e->merge_in,
			casenr,
			Cached->e->merge_start, Cached->e->merge, Cached->e->merge_in);

		goto gotit;
	}

	fprintf(tm, "\tcase %d: /* STATE %d - line %d %s - [",
		uniq++, e->seqno, e->n->ln, e->n->fn->name);
	comment(tm, e->n, 0);
	nrbups = (e->merge || e->merge_start) ? nrhops(e) : nr_bup(e);
	fprintf(tm, "] (%d:%d:%d - %d) */\n\t\t",
		e->merge_start, e->merge, nrbups, e->merge_in);

	if (nrbups > MAXMERGE-1)
		fatal("merge requires more than 256 bups", (char *)0);

	if (e->n->ntyp != 'r' && Pid != claimnr && Pid != eventmapnr)
		fprintf(tm, "IfNotBlocked\n\t\t");

	if (multi_needed != 0 || multi_undo != 0)
		fatal("cannot happen, case_cache", (char *) 0);

	if (nrbups > 1)
	{	multi_oval = 1;
		multi_needed = nrbups; /* allocated after edge condition */
	} else
		multi_oval = 0;

	memset(CnT, 0, sizeof(CnT));
	YZmax = YZcnt = 0;

/* NEW 4.2.6 */
	if (Pid == claimnr)
	{
		fprintf(tm, "\n#if defined(VERI) && !defined(NP)\n\t\t");
		fprintf(tm, "{	static int reported%d = 0;\n\t\t", e->seqno);
		/* source state changes in retrans and must be looked up in frm_st0[t->forw] */
		fprintf(tm, "	if (verbose && !reported%d)\n\t\t", e->seqno);
		fprintf(tm, "	{	printf(\"depth %%d: Claim reached state %%d (line %%d)\\n\",\n\t\t");
		fprintf(tm, "			depth, frm_st0[t->forw], src_claim[%d]);\n\t\t", e->seqno);
		fprintf(tm, "		reported%d = 1;\n\t\t", e->seqno);
		fprintf(tm, "		fflush(stdout);\n\t\t");
		fprintf(tm, "}	}\n");
		fprintf(tm, "#endif\n\t\t");
	}
/* end */

	/* the src xrefs have the numbers in e->seqno builtin */
	fprintf(tm, "reached[%d][%d] = 1;\n\t\t", Pid, e->seqno);

	doforward(tm, e);

	if (e->merge_start)
		target = e->merge_start;
	else
		target = e->merge;

	if (target)
	{	f = e;

more:		if (f->n->ntyp == GOTO)
		{	g = get_lab(f->n, 1);
			if (g->seqno == target)
				f = g;
			else
				f = huntele(g, f->status, target);
		} else
			f = f->nxt;


		if (f && f->seqno != target)
		{	if (!f->merge && !f->merge_single)
			{	fprintf(tm, "/* stop at bad hop %d, %d */\n\t\t",
					f->seqno, target);
				goto out;
			}
			fprintf(tm, "/* merge: ");
			comment(tm, f->n, 0);
			fprintf(tm,  "(%d, %d, %d) */\n\t\t", f->merge, f->seqno, target);
			fprintf(tm, "reached[%d][%d] = 1;\n\t\t", Pid, f->seqno);
			YZcnt++;
			lab_transfer(e, f);
			mark = f->status&(ATOM|L_ATOM); /* last step wins */
			doforward(tm, f);
			if (f->merge_in == 1) f->merge_mark++;

			goto more;
	}	}
out:
	fprintf(tm, "_m = %d", getweight(e->n));
	if (m_loss && e->n->ntyp == 's') fprintf(tm, "+delta_m; delta_m = 0");
	fprintf(tm, "; goto P999; /* %d */\n", YZcnt);

	multi_needed = 0;
	didcase = 0;

	if (target)
		lastfirst(target, e, casenr); /* mergesteps only */

	dobackward(e, casenr);			/* the original step */

	fprintf(tb, ";\n\t\t");

	if (e->merge || e->merge_start)
	{	if (!didcase)
		{	fprintf(tb, "\n\tcase %d: ", casenr);
			fprintf(tb, "/* STATE %d */", e->seqno);
			didcase++;
		} else
			fprintf(tb, ";");
	} else
		fprintf(tb, ";");
	fprintf(tb, "\n\t\t");

	if (multi_undo)
	{	fprintf(tb, "ungrab_ints(trpt->bup.ovals, %d);\n\t\t",
			multi_undo);
		multi_undo = 0;
	}
	if (didcase)
	{	fprintf(tb, "goto R999;\n");
		bupcase = casenr;
	}

	if (!e->merge && !e->merge_start)
		new_case(e, casenr, bupcase, Pid);

gotit:
	j = a;
	if (e->merge_start)
		j = e->merge_start;
	else if (e->merge)
		j = e->merge;
haveit:
	fprintf(tt, "%ssettr(%d,%d,%d,%d,%d,\"", fromcache?"/* c */ ":"",
		e->Seqno, mark, j, casenr, bupcase);

	return (fromcache)?0:casenr;
}

static void
put_el(Element *e, int Tt0, int Tt1)
{	int a, casenr, Global_ref;
	Element *g = ZE;

	if (e->n->ntyp == GOTO)
	{	g = get_lab(e->n, 1);
		g = huntele(g, e->status, -1);
		cross_dsteps(e->n, g->n);
		a = g->seqno;
	} else if (e->nxt)
	{	g = huntele(e->nxt, e->status, -1);
		a = g->seqno;
	} else
		a = 0;
	if (g
	&&  (g->status&CHECK2	/* entering remotely ref'd state */
	||   e->status&CHECK2))	/* leaving  remotely ref'd state */
		e->status |= I_GLOB;

	/* don't remove dead edges in here, to preserve structure of fsm */
	if (e->merge_start || e->merge)
		goto non_generic;

	/*** avoid duplicate or redundant cases in pan.m ***/
	switch (e->n->ntyp) {
	case ELSE:
		casenr = 2; /* standard else */
		putskip(e->seqno);
		goto generic_case;
		/* break; */
	case '.':
	case GOTO:
	case BREAK:
		putskip(e->seqno); 
		casenr = 1; /* standard goto */
generic_case:	fprintf(tt, "\ttrans[%d][%d]\t= ", Pid, e->seqno);
		fprintf(tt, "settr(%d,%d,%d,%d,0,\"",
			e->Seqno, e->status&ATOM, a, casenr);
		break;
#ifndef PRINTF
	case PRINT:
		goto non_generic;
	case PRINTM:
		goto non_generic;
#endif
	case 'c':
		if (e->n->lft->ntyp == CONST
		&&  e->n->lft->val == 1)	/* skip or true */
		{	casenr = 1;
			putskip(e->seqno);
			goto generic_case;
		}
		goto non_generic;

	default:
non_generic:
		casenr = case_cache(e, a);
		if (casenr < 0) return;	/* unreachable state */
		break;
	}
	/* tailend of settr(...); */
	Global_ref = (e->status&I_GLOB)?1:has_global(e->n);
	comment(tt, e->n, e->seqno);
	fprintf(tt, "\", %d, ", Global_ref);
	if (Tt0 != 2)
	{	fprintf(tt, "%d, %d);", Tt0, Tt1);
	} else
	{	Tpe(e->n);	/* sets EPT */
		fprintf(tt, "%d, %d);", EPT[0], EPT[1]);
	}
	if ((e->merge_start && e->merge_start != a)
	||  (e->merge && e->merge != a))
	{	fprintf(tt, " /* m: %d -> %d,%d */\n",
			a, e->merge_start, e->merge);
		fprintf(tt, "	reached%d[%d] = 1;",
			Pid, a); /* Sheinman's example */
	}
	fprintf(tt, "\n");

	if (casenr > 2)
		tr_map(casenr, e);
	put_escp(e);
}

static void
nested_unless(Element *e, Element *g)
{	struct SeqList *y = e->esc, *z = g->esc;

	for ( ; y && z; y = y->nxt, z = z->nxt)
		if (z->this != y->this)
			break;
	if (!y && !z)
		return;

	if (g->n->ntyp != GOTO
	&&  g->n->ntyp != '.'
	&&  e->sub->nxt)
	{	printf("error: (%s:%d) saw 'unless' on a guard:\n",
			(e->n)?e->n->fn->name:"-",
			(e->n)?e->n->ln:0);
		printf("=====>instead of\n");
		printf("	do (or if)\n");
		printf("	:: ...\n");
		printf("	:: stmnt1 unless stmnt2\n");
		printf("	od (of fi)\n");
		printf("=====>use\n");
		printf("	do (or if)\n");
		printf("	:: ...\n");
		printf("	:: stmnt1\n");
		printf("	od (or fi) unless stmnt2\n");
		printf("=====>or rewrite\n");
	}
}

static void
put_seq(Sequence *s, int Tt0, int Tt1)
{	SeqList *h;
	Element *e, *g;
	int a, deadlink;

	if (0) printf("put_seq %d\n", s->frst->seqno);

	for (e = s->frst; e; e = e->nxt)
	{
		if (0) printf("	step %d\n", e->seqno);
		if (e->status & DONE)
		{
			if (0) printf("		done before\n");
			goto checklast;
		}
		e->status |= DONE;

		if (e->n->ln)
			putsrc(e);

		if (e->n->ntyp == UNLESS)
		{
			if (0) printf("		an unless\n");
			put_seq(e->sub->this, Tt0, Tt1);
		} else if (e->sub)
		{
			if (0) printf("		has sub\n");
			fprintf(tt, "\tT = trans[%d][%d] = ",
				Pid, e->seqno);
			fprintf(tt, "settr(%d,%d,0,0,0,\"",
				e->Seqno, e->status&ATOM);
			comment(tt, e->n, e->seqno);
			if (e->status&CHECK2)
				e->status |= I_GLOB;
			fprintf(tt, "\", %d, %d, %d);",
				(e->status&I_GLOB)?1:0, Tt0, Tt1);
			blurb(tt, e);
			for (h = e->sub; h; h = h->nxt)
			{	putskip(h->this->frst->seqno);
				g = huntstart(h->this->frst);
				if (g->esc)
					nested_unless(e, g);
				a = g->seqno;

				if (g->n->ntyp == 'c'
				&&  g->n->lft->ntyp == CONST
				&&  g->n->lft->val == 0		/* 0 or false */
				&& !g->esc)
				{	fprintf(tt, "#if 0\n\t/* dead link: */\n");
					deadlink = 1;
					if (verbose&32)
					printf("spin: line %3d  %s, Warning: condition is always false\n",
						g->n->ln, g->n->fn?g->n->fn->name:"");
				} else
					deadlink = 0;
				if (0) printf("			settr %d %d\n", a, 0);
				if (h->nxt)
					fprintf(tt, "\tT = T->nxt\t= ");
				else
					fprintf(tt, "\t    T->nxt\t= ");
				fprintf(tt, "settr(%d,%d,%d,0,0,\"",
					e->Seqno, e->status&ATOM, a);
				comment(tt, e->n, e->seqno);
				if (g->status&CHECK2)
					h->this->frst->status |= I_GLOB;
				fprintf(tt, "\", %d, %d, %d);",
					(h->this->frst->status&I_GLOB)?1:0,
					Tt0, Tt1);
				blurb(tt, e);
				if (deadlink)
					fprintf(tt, "#endif\n");
			}
			for (h = e->sub; h; h = h->nxt)
				put_seq(h->this, Tt0, Tt1);
		} else
		{
			if (0) printf("		[non]atomic %d\n", e->n->ntyp);
			if (e->n->ntyp == ATOMIC
			||  e->n->ntyp == D_STEP
			||  e->n->ntyp == NON_ATOMIC)
				put_sub(e, Tt0, Tt1);
			else 
			{
				if (0) printf("			put_el %d\n", e->seqno);
				put_el(e, Tt0, Tt1);
			}
		}
checklast:	if (e == s->last)
			break;
	}
	if (0) printf("put_seq done\n");
}

static void
patch_atomic(Sequence *s)	/* catch goto's that break the chain */
{	Element *f, *g;
	SeqList *h;

	for (f = s->frst; f ; f = f->nxt)
	{
		if (f->n && f->n->ntyp == GOTO)
		{	g = get_lab(f->n,1);
			cross_dsteps(f->n, g->n);
			if ((f->status & (ATOM|L_ATOM))
			&& !(g->status & (ATOM|L_ATOM)))
			{	f->status &= ~ATOM;
				f->status |= L_ATOM;
			}
			/* bridge atomics */
			if ((f->status & L_ATOM)
			&&  (g->status & (ATOM|L_ATOM)))
			{	f->status &= ~L_ATOM;
				f->status |= ATOM;
			}
		} else
		for (h = f->sub; h; h = h->nxt)
			patch_atomic(h->this);
		if (f == s->extent)
			break;
	}
}

static void
mark_seq(Sequence *s)
{	Element *f;
	SeqList *h;

	for (f = s->frst; f; f = f->nxt)
	{	f->status |= I_GLOB;

		if (f->n->ntyp == ATOMIC
		||  f->n->ntyp == NON_ATOMIC
		||  f->n->ntyp == D_STEP)
			mark_seq(f->n->sl->this);

		for (h = f->sub; h; h = h->nxt)
			mark_seq(h->this);
		if (f == s->last)
			return;
	}
}

static Element *
find_target(Element *e)
{	Element *f;

	if (!e) return e;

	if (t_cyc++ > 32)
	{	fatal("cycle of goto jumps", (char *) 0);
	}
	switch (e->n->ntyp) {
	case  GOTO:
		f = get_lab(e->n,1);
		cross_dsteps(e->n, f->n);
		f = find_target(f);
		break;
	case BREAK:
		if (e->nxt)
		{	f = find_target(huntele(e->nxt, e->status, -1));
			break;	/* 4.3.0 -- was missing */
		}
		/* else fall through */
	default:
		f = e;
		break;
	}
	return f;
}

Element *
target(Element *e)
{
	if (!e) return e;
	lineno = e->n->ln;
	Fname  = e->n->fn;
	t_cyc = 0;
	return find_target(e);
}

static int
scan_seq(Sequence *s)
{	Element *f, *g;
	SeqList *h;

	for (f = s->frst; f; f = f->nxt)
	{	if ((f->status&CHECK2)
		||  has_global(f->n))
			return 1;
		if (f->n->ntyp == GOTO)	/* may reach other atomic */
		{
#if 0
			/* if jumping from an atomic without globals into
			 * one with globals, this does the wrong thing
			 * example by Claus Traulsen, 22 June 2007
			 */
			g = target(f);
			if (g
			&& !(f->status & L_ATOM)
			&& !(g->status & (ATOM|L_ATOM)))
#endif
			{	fprintf(tt, "	/* mark-down line %d */\n",
					f->n->ln);
				return 1; /* assume worst case */
		}	}
		for (h = f->sub; h; h = h->nxt)
			if (scan_seq(h->this))
				return 1;
		if (f == s->last)
			break;
	}
	return 0;
}

static int
glob_args(Lextok *n)
{	int result = 0;
	Lextok *v;

	for (v = n->rgt; v; v = v->rgt)
	{	if (v->lft->ntyp == CONST)
			continue;
		if (v->lft->ntyp == EVAL)
			result += has_global(v->lft->lft);
		else
			result += has_global(v->lft);
	}
	return result;
}

int
has_global(Lextok *n)
{	Lextok *v; extern int runsafe;

	if (!n) return 0;
	if (AllGlobal) return 1;	/* global provided clause */

	switch (n->ntyp) {
	case ATOMIC:
	case D_STEP:
	case NON_ATOMIC:
		return scan_seq(n->sl->this);

	case '.':
	case BREAK:
	case GOTO:
	case CONST:
		return 0;

	case   ELSE: return n->val; /* true if combined with chan refs */

	case    's': return glob_args(n)!=0 || ((n->sym->xu&(XS|XX)) != XS);
	case    'r': return glob_args(n)!=0 || ((n->sym->xu&(XR|XX)) != XR);
	case    'R': return glob_args(n)!=0 || (((n->sym->xu)&(XR|XS|XX)) != (XR|XS));
	case NEMPTY: return ((n->sym->xu&(XR|XX)) != XR);
	case  NFULL: return ((n->sym->xu&(XS|XX)) != XS);
	case   FULL: return ((n->sym->xu&(XR|XX)) != XR);
	case  EMPTY: return ((n->sym->xu&(XS|XX)) != XS);
	case  LEN:   return (((n->sym->xu)&(XR|XS|XX)) != (XR|XS));

	case   NAME:
		if (n->sym->context
		|| (n->sym->hidden&64)
		||  strcmp(n->sym->name, "_pid") == 0
		||  strcmp(n->sym->name, "_") == 0)
			return 0;
		return 1;

	case RUN: return 1-runsafe;

	case C_CODE: case C_EXPR:
		return glob_inline(n->sym->name);

	case ENABLED: case PC_VAL: case NONPROGRESS:
	case 'p': case 'q':
	case TIMEOUT:
		return 1;

	/* 	@ was 1 (global) since 2.8.5
		in 3.0 it is considered local and
		conditionally safe, provided:
			II is the youngest process
			and nrprocs < MAXPROCS
	*/
	case '@': return 0;

	case '!': case UMIN: case '~': case ASSERT:
		return has_global(n->lft);

	case '/': case '*': case '-': case '+':
	case '%': case LT:  case GT: case '&': case '^':
	case '|': case LE:  case GE:  case NE: case '?':
	case EQ:  case OR:  case AND: case LSHIFT:
	case RSHIFT: case 'c': case ASGN:
		return has_global(n->lft) || has_global(n->rgt);

	case PRINT:
		for (v = n->lft; v; v = v->rgt)
			if (has_global(v->lft)) return 1;
		return 0;
	case PRINTM:
		return has_global(n->lft);
	}
	return 0;
}

static void
Bailout(FILE *fd, char *str)
{
	if (!GenCode)
		fprintf(fd, "continue%s", str);
	else if (IsGuard)
		fprintf(fd, "%s%s", NextLab[Level], str);
	else
		fprintf(fd, "Uerror(\"block in step seq\")%s", str);
}

#define cat0(x)   	putstmnt(fd,now->lft,m); fprintf(fd, x); \
			putstmnt(fd,now->rgt,m)
#define cat1(x)		fprintf(fd,"("); cat0(x); fprintf(fd,")")
#define cat2(x,y)  	fprintf(fd,x); putstmnt(fd,y,m)
#define cat3(x,y,z)	fprintf(fd,x); putstmnt(fd,y,m); fprintf(fd,z)

void
putstmnt(FILE *fd, Lextok *now, int m)
{	Lextok *v;
	int i, j;

	if (!now) { fprintf(fd, "0"); return; }
	lineno = now->ln;
	Fname  = now->fn;

	switch (now->ntyp) {
	case CONST:	fprintf(fd, "%d", now->val); break;
	case '!':	cat3(" !(", now->lft, ")"); break;
	case UMIN:	cat3(" -(", now->lft, ")"); break;
	case '~':	cat3(" ~(", now->lft, ")"); break;

	case '/':	cat1("/");  break;
	case '*':	cat1("*");  break;
	case '-':	cat1("-");  break;
	case '+':	cat1("+");  break;
	case '%':	cat1("%%"); break;
	case '&':	cat1("&");  break;
	case '^':	cat1("^");  break;
	case '|':	cat1("|");  break;
	case LT:	cat1("<");  break;
	case GT:	cat1(">");  break;
	case LE:	cat1("<="); break;
	case GE:	cat1(">="); break;
	case NE:	cat1("!="); break;
	case EQ:	cat1("=="); break;
	case OR:	cat1("||"); break;
	case AND:	cat1("&&"); break;
	case LSHIFT:	cat1("<<"); break;
	case RSHIFT:	cat1(">>"); break;

	case TIMEOUT:
		if (separate == 2)
			fprintf(fd, "((tau)&1)");
		else
			fprintf(fd, "((trpt->tau)&1)");
		if (GenCode)
		 printf("spin: line %3d, warning: 'timeout' in d_step sequence\n",
			lineno);
		/* is okay as a guard */
		break;

	case RUN:
		if (claimproc
		&&  strcmp(now->sym->name, claimproc) == 0)
			fatal("claim %s, (not runnable)", claimproc);
		if (eventmap
		&&  strcmp(now->sym->name, eventmap) == 0)
			fatal("eventmap %s, (not runnable)", eventmap);

		if (GenCode)
		  fatal("'run' in d_step sequence (use atomic)",
			(char *)0);

		fprintf(fd,"addproc(%d", fproc(now->sym->name));
		for (v = now->lft, i = 0; v; v = v->rgt, i++)
		{	cat2(", ", v->lft);
		}
		check_param_count(i, now);

		if (i > Npars)
		{	printf("\t%d parameters used, max %d expected\n", i, Npars);
			fatal("too many parameters in run %s(...)",
			now->sym->name);
		}
		for ( ; i < Npars; i++)
			fprintf(fd, ", 0");
		fprintf(fd, ")");
		break;

	case ENABLED:
		cat3("enabled(II, ", now->lft, ")");
		break;

	case NONPROGRESS:
		/* o_pm&4=progress, tau&128=claim stutter */
		if (separate == 2)
		fprintf(fd, "(!(o_pm&4) && !(tau&128))");
		else
		fprintf(fd, "(!(trpt->o_pm&4) && !(trpt->tau&128))");
		break;

	case PC_VAL:
		cat3("((P0 *) Pptr(", now->lft, "+BASE))->_p");
		break;

	case LEN:
		if (!terse && !TestOnly && has_xu)
		{	fprintf(fd, "\n#ifndef XUSAFE\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&1) || ");
			putname(fd, "q_R_check(", now->lft, m, "");
			fprintf(fd, ", II)) &&\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&2) || ");
			putname(fd, "q_S_check(", now->lft, m, ", II)) &&");
			fprintf(fd, "\n#endif\n\t\t");
		}
		putname(fd, "q_len(", now->lft, m, ")");
		break;

	case FULL:
		if (!terse && !TestOnly && has_xu)
		{	fprintf(fd, "\n#ifndef XUSAFE\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&1) || ");
			putname(fd, "q_R_check(", now->lft, m, "");
			fprintf(fd, ", II)) &&\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&2) || ");
			putname(fd, "q_S_check(", now->lft, m, ", II)) &&");
			fprintf(fd, "\n#endif\n\t\t");
		}
		putname(fd, "q_full(", now->lft, m, ")");
		break;

	case EMPTY:
		if (!terse && !TestOnly && has_xu)
		{	fprintf(fd, "\n#ifndef XUSAFE\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&1) || ");
			putname(fd, "q_R_check(", now->lft, m, "");
			fprintf(fd, ", II)) &&\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&2) || ");
			putname(fd, "q_S_check(", now->lft, m, ", II)) &&");
			fprintf(fd, "\n#endif\n\t\t");
		}
		putname(fd, "(q_len(", now->lft, m, ")==0)");
		break;

	case NFULL:
		if (!terse && !TestOnly && has_xu)
		{	fprintf(fd, "\n#ifndef XUSAFE\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&2) || ");
			putname(fd, "q_S_check(", now->lft, m, ", II)) &&");
			fprintf(fd, "\n#endif\n\t\t");
		}
		putname(fd, "(!q_full(", now->lft, m, "))");
		break;

	case NEMPTY:
		if (!terse && !TestOnly && has_xu)
		{	fprintf(fd, "\n#ifndef XUSAFE\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&1) || ");
			putname(fd, "q_R_check(", now->lft, m, ", II)) &&");
			fprintf(fd, "\n#endif\n\t\t");
		}
		putname(fd, "(q_len(", now->lft, m, ")>0)");
		break;

	case 's':
		if (Pid == eventmapnr)
		{	fprintf(fd, "if ((ot == EVENT_TRACE && _tp != 's') ");
			putname(fd, "|| _qid+1 != ", now->lft, m, "");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v->lft->ntyp != CONST
				&&  v->lft->ntyp != EVAL)
					continue;
				fprintf(fd, " \\\n\t\t|| qrecv(");
				putname(fd, "", now->lft, m, ", ");
				putname(fd, "q_len(", now->lft, m, ")-1, ");
				fprintf(fd, "%d, 0) != ", i);
				if (v->lft->ntyp == CONST)
					putstmnt(fd, v->lft, m);
				else /* EVAL */
					putstmnt(fd, v->lft->lft, m);
			}
			fprintf(fd, ")\n");
			fprintf(fd, "\t\t	continue");
			putname(th, " || (x_y3_ == ", now->lft, m, ")");
			break;
		}
		if (TestOnly)
		{	if (m_loss)
				fprintf(fd, "1");
			else
				putname(fd, "!q_full(", now->lft, m, ")");
			break;
		}
		if (has_xu)
		{	fprintf(fd, "\n#ifndef XUSAFE\n\t\t");
			putname(fd, "if (q_claim[", now->lft, m, "]&2) ");
			putname(fd, "q_S_check(", now->lft, m, ", II);");
			fprintf(fd, "\n#endif\n\t\t");
		}
		fprintf(fd, "if (q_%s",
			(u_sync > 0 && u_async == 0)?"len":"full");
		putname(fd, "(", now->lft, m, "))\n");

		if (m_loss)
			fprintf(fd, "\t\t{ nlost++; delta_m = 1; } else {");
		else
		{	fprintf(fd, "\t\t\t");
			Bailout(fd, ";");
		}

		if (has_enabled)
			fprintf(fd, "\n\t\tif (TstOnly) return 1;");

		if (u_sync && !u_async && rvopt)
			fprintf(fd, "\n\n\t\tif (no_recvs(II)) continue;\n");

		fprintf(fd, "\n#ifdef HAS_CODE\n");
		fprintf(fd, "\t\tif (readtrail && gui) {\n");
		fprintf(fd, "\t\t\tchar simtmp[32];\n");
		putname(fd, "\t\t\tsprintf(simvals, \"%%d!\", ", now->lft, m, ");\n");
		_isok++;
		for (v = now->rgt, i = 0; v; v = v->rgt, i++)
		{	cat3("\t\tsprintf(simtmp, \"%%d\", ", v->lft, "); strcat(simvals, simtmp);");
			if (v->rgt)
			fprintf(fd, "\t\tstrcat(simvals, \",\");\n");
		}
		_isok--;
		fprintf(fd, "\t\t}\n");
		fprintf(fd, "#endif\n\t\t");

		putname(fd, "\n\t\tqsend(", now->lft, m, "");
		fprintf(fd, ", %d", now->val);
		for (v = now->rgt, i = 0; v; v = v->rgt, i++)
		{	cat2(", ", v->lft);
		}
		if (i > Mpars)
		{	terse++;
			putname(stdout, "channel name: ", now->lft, m, "\n");
			terse--;
			printf("	%d msg parameters sent, %d expected\n", i, Mpars);
			fatal("too many pars in send", "");
		}
		for ( ; i < Mpars; i++)
			fprintf(fd, ", 0");
		fprintf(fd, ")");
		if (u_sync)
		{	fprintf(fd, ";\n\t\t");
			if (u_async)
			  putname(fd, "if (q_zero(", now->lft, m, ")) ");
			putname(fd, "{ boq = ", now->lft, m, "");
			if (GenCode)
			  fprintf(fd, "; Uerror(\"rv-attempt in d_step\")");
			fprintf(fd, "; }");
		}
		if (m_loss)
			fprintf(fd, ";\n\t\t}\n\t\t"); /* end of m_loss else */
		break;

	case 'r':
		if (Pid == eventmapnr)
		{	fprintf(fd, "if ((ot == EVENT_TRACE && _tp != 'r') ");
			putname(fd, "|| _qid+1 != ", now->lft, m, "");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v->lft->ntyp != CONST
				&&  v->lft->ntyp != EVAL)
					continue;
				fprintf(fd, " \\\n\t\t|| qrecv(");
				putname(fd, "", now->lft, m, ", ");
				fprintf(fd, "0, %d, 0) != ", i);
				if (v->lft->ntyp == CONST)
					putstmnt(fd, v->lft, m);
				else /* EVAL */
					putstmnt(fd, v->lft->lft, m);
			}
			fprintf(fd, ")\n");
			fprintf(fd, "\t\t	continue");

			putname(tc, " || (x_y3_ == ", now->lft, m, ")");

			break;
		}
		if (TestOnly)
		{	fprintf(fd, "((");
			if (u_sync) fprintf(fd, "(boq == -1 && ");

			putname(fd, "q_len(", now->lft, m, ")");

			if (u_sync && now->val <= 1)
			{ putname(fd, ") || (boq == ",  now->lft,m," && ");
			  putname(fd, "q_zero(", now->lft,m,"))");
			}

			fprintf(fd, ")");
			if (now->val == 0 || now->val == 2)
			{	for (v = now->rgt, i=j=0; v; v = v->rgt, i++)
				{ if (v->lft->ntyp == CONST)
				  { cat3("\n\t\t&& (", v->lft, " == ");
				    putname(fd, "qrecv(", now->lft, m, ", ");
				    fprintf(fd, "0, %d, 0))", i);
				  } else if (v->lft->ntyp == EVAL)
				  { cat3("\n\t\t&& (", v->lft->lft, " == ");
				    putname(fd, "qrecv(", now->lft, m, ", ");
				    fprintf(fd, "0, %d, 0))", i);
				  } else
				  {	j++; continue;
				  }
				}
			} else
			{	fprintf(fd, "\n\t\t&& Q_has(");
				putname(fd, "", now->lft, m, "");
				for (v = now->rgt, i=0; v; v = v->rgt, i++)
				{	if (v->lft->ntyp == CONST)
					{	fprintf(fd, ", 1, ");
						putstmnt(fd, v->lft, m);
					} else if (v->lft->ntyp == EVAL)
					{	fprintf(fd, ", 1, ");
						putstmnt(fd, v->lft->lft, m);
					} else
					{	fprintf(fd, ", 0, 0");
				}	}
				for ( ; i < Mpars; i++)
					fprintf(fd, ", 0, 0");
				fprintf(fd, ")");
			}
			fprintf(fd, ")");
			break;
		}
		if (has_xu)
		{	fprintf(fd, "\n#ifndef XUSAFE\n\t\t");
			putname(fd, "if (q_claim[", now->lft, m, "]&1) ");
			putname(fd, "q_R_check(", now->lft, m, ", II);");
			fprintf(fd, "\n#endif\n\t\t");
		}
		if (u_sync)
		{	if (now->val >= 2)
			{	if (u_async)
				{ fprintf(fd, "if (");
				  putname(fd, "q_zero(", now->lft,m,"))");
				  fprintf(fd, "\n\t\t{\t");
				}
				fprintf(fd, "uerror(\"polling ");
				fprintf(fd, "rv chan\");\n\t\t");
				if (u_async)
				  fprintf(fd, "	continue;\n\t\t}\n\t\t");
				fprintf(fd, "IfNotBlocked\n\t\t");
			} else
			{	fprintf(fd, "if (");
				if (u_async == 0)
				  putname(fd, "boq != ", now->lft,m,") ");
				else
				{ putname(fd, "q_zero(", now->lft,m,"))");
				  fprintf(fd, "\n\t\t{\tif (boq != ");
				  putname(fd, "",  now->lft,m,") ");
				  Bailout(fd, ";\n\t\t} else\n\t\t");
				  fprintf(fd, "{\tif (boq != -1) ");
				}
				Bailout(fd, ";\n\t\t");
				if (u_async)
					fprintf(fd, "}\n\t\t");
		}	}
		putname(fd, "if (q_len(", now->lft, m, ") == 0) ");
		Bailout(fd, "");

		for (v = now->rgt, j=0; v; v = v->rgt)
		{	if (v->lft->ntyp != CONST
			&&  v->lft->ntyp != EVAL)
				j++;	/* count settables */
		}
		fprintf(fd, ";\n\n\t\tXX=1");
/* test */	if (now->val == 0 || now->val == 2)
		{	for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v->lft->ntyp == CONST)
				{ fprintf(fd, ";\n\t\t");
				  cat3("if (", v->lft, " != ");
				  putname(fd, "qrecv(", now->lft, m, ", ");
				  fprintf(fd, "0, %d, 0)) ", i);
				  Bailout(fd, "");
				} else if (v->lft->ntyp == EVAL)
				{ fprintf(fd, ";\n\t\t");
				  cat3("if (", v->lft->lft, " != ");
				  putname(fd, "qrecv(", now->lft, m, ", ");
				  fprintf(fd, "0, %d, 0)) ", i);
				  Bailout(fd, "");
			}	}
		} else	/* random receive: val 1 or 3 */
		{	fprintf(fd, ";\n\t\tif (!(XX = Q_has(");
			putname(fd, "", now->lft, m, "");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v->lft->ntyp == CONST)
				{	fprintf(fd, ", 1, ");
					putstmnt(fd, v->lft, m);
				} else if (v->lft->ntyp == EVAL)
				{	fprintf(fd, ", 1, ");
					putstmnt(fd, v->lft->lft, m);
				} else
				{	fprintf(fd, ", 0, 0");
			}	}
			for ( ; i < Mpars; i++)
				fprintf(fd, ", 0, 0");
			fprintf(fd, "))) ");
			Bailout(fd, "");
			fprintf(fd, ";\n\t\t");
			if (multi_oval)
			{	check_needed();
				fprintf(fd, "(trpt+1)->bup.ovals[%d] = ",
					multi_oval-1);
				multi_oval++;
			} else
				fprintf(fd, "(trpt+1)->bup.oval = ");
			fprintf(fd, "XX");
		}

		if (has_enabled)
			fprintf(fd, ";\n\t\tif (TstOnly) return 1");

		if (j == 0 && now->val >= 2)
		{	fprintf(fd, ";\n\t\t");
			break;	/* poll without side-effect */
		}

		if (!GenCode)
		{	int jj = 0;
			fprintf(fd, ";\n\t\t");
			/* no variables modified */
			if (j == 0 && now->val == 0)
			{	fprintf(fd, "if (q_flds[((Q0 *)qptr(");
				putname(fd, "", now->lft, m, "-1))->_t]");
				fprintf(fd, " != %d)\n\t", i);
				fprintf(fd, "\t\tUerror(\"wrong nr of msg fields in rcv\");\n\t\t");
			}

			for (v = now->rgt; v; v = v->rgt)
				if ((v->lft->ntyp != CONST
				&&   v->lft->ntyp != EVAL))
					jj++;	/* nr of vars needing bup */

			if (jj)
			for (v = now->rgt, i = 0; v; v = v->rgt, i++)
			{	char tempbuf[64];

				if ((v->lft->ntyp == CONST
				||   v->lft->ntyp == EVAL))
					continue;

				if (multi_oval)
				{	check_needed();
					sprintf(tempbuf, "(trpt+1)->bup.ovals[%d] = ",
						multi_oval-1);
					multi_oval++;
				} else
					sprintf(tempbuf, "(trpt+1)->bup.oval = ");

				if (v->lft->sym && !strcmp(v->lft->sym->name, "_"))
				{	fprintf(fd, tempbuf);
					putname(fd, "qrecv(", now->lft, m, "");
					fprintf(fd, ", XX-1, %d, 0);\n\t\t", i);
				} else
				{	_isok++;
					cat3(tempbuf, v->lft, ";\n\t\t");
					_isok--;
				}
			}

			if (jj)	/* check for double entries q?x,x */
			{	Lextok *w;

				for (v = now->rgt; v; v = v->rgt)
				{	if (v->lft->ntyp != CONST
					&&  v->lft->ntyp != EVAL
					&&  v->lft->sym
					&&  v->lft->sym->type != STRUCT	/* not a struct */
					&&  v->lft->sym->nel == 1	/* not an array */
					&&  strcmp(v->lft->sym->name, "_") != 0)
					for (w = v->rgt; w; w = w->rgt)
						if (v->lft->sym == w->lft->sym)
						{	fatal("cannot use var ('%s') in multiple msg fields",
								v->lft->sym->name);
			}	}		}
		}
/* set */	for (v = now->rgt, i = 0; v; v = v->rgt, i++)
		{	if ((v->lft->ntyp == CONST
			||   v->lft->ntyp == EVAL) && v->rgt)
				continue;
			fprintf(fd, ";\n\t\t");

			if (v->lft->ntyp != CONST
			&&  v->lft->ntyp != EVAL
			&&  strcmp(v->lft->sym->name, "_") != 0)
			{	nocast=1;
				_isok++;
				putstmnt(fd, v->lft, m);
				_isok--;
				nocast=0;
				fprintf(fd, " = ");
			}
			putname(fd, "qrecv(", now->lft, m, ", ");
			fprintf(fd, "XX-1, %d, ", i);
			fprintf(fd, "%d)", (v->rgt || now->val >= 2)?0:1);

			if (v->lft->ntyp != CONST
			&&  v->lft->ntyp != EVAL
			&& strcmp(v->lft->sym->name, "_") != 0
			&&  (v->lft->ntyp != NAME
			||   v->lft->sym->type != CHAN))
			{	fprintf(fd, ";\n#ifdef VAR_RANGES");
				fprintf(fd, "\n\t\tlogval(\"");
				withprocname = terse = nocast = 1;
				_isok++;
				putstmnt(fd,v->lft,m);
				withprocname = terse = nocast = 0;
				fprintf(fd, "\", ");
				putstmnt(fd,v->lft,m);
				_isok--;
				fprintf(fd, ");\n#endif\n");
				fprintf(fd, "\t\t");
			}
		}
		fprintf(fd, ";\n\t\t");

		fprintf(fd, "\n#ifdef HAS_CODE\n");
		fprintf(fd, "\t\tif (readtrail && gui) {\n");
		fprintf(fd, "\t\t\tchar simtmp[32];\n");
		putname(fd, "\t\t\tsprintf(simvals, \"%%d?\", ", now->lft, m, ");\n");
		_isok++;
		for (v = now->rgt, i = 0; v; v = v->rgt, i++)
		{	if (v->lft->ntyp != EVAL)
			{ cat3("\t\tsprintf(simtmp, \"%%d\", ", v->lft, "); strcat(simvals, simtmp);");
			} else
			{ cat3("\t\tsprintf(simtmp, \"%%d\", ", v->lft->lft, "); strcat(simvals, simtmp);");
			}
			if (v->rgt)
			fprintf(fd, "\t\tstrcat(simvals, \",\");\n");
		}
		_isok--;
		fprintf(fd, "\t\t}\n");
		fprintf(fd, "#endif\n\t\t");

		if (u_sync)
		{	putname(fd, "if (q_zero(", now->lft, m, "))");
			fprintf(fd, "\n\t\t{	boq = -1;\n");

			fprintf(fd, "#ifndef NOFAIR\n"); /* NEW 3.0.8 */
			fprintf(fd, "\t\t\tif (fairness\n");
			fprintf(fd, "\t\t\t&& !(trpt->o_pm&32)\n");
			fprintf(fd, "\t\t\t&& (now._a_t&2)\n");
			fprintf(fd, "\t\t\t&&  now._cnt[now._a_t&1] == II+2)\n");
			fprintf(fd, "\t\t\t{	now._cnt[now._a_t&1] -= 1;\n");
			fprintf(fd, "#ifdef VERI\n");
			fprintf(fd, "\t\t\t	if (II == 1)\n");
			fprintf(fd, "\t\t\t		now._cnt[now._a_t&1] = 1;\n");
			fprintf(fd, "#endif\n");
			fprintf(fd, "#ifdef DEBUG\n");
			fprintf(fd, "\t\t\tprintf(\"%%3d: proc %%d fairness \", depth, II);\n");
			fprintf(fd, "\t\t\tprintf(\"Rule 2: --cnt to %%d (%%d)\\n\",\n");
			fprintf(fd, "\t\t\t	now._cnt[now._a_t&1], now._a_t);\n");
			fprintf(fd, "#endif\n");
			fprintf(fd, "\t\t\t	trpt->o_pm |= (32|64);\n");
			fprintf(fd, "\t\t\t}\n");
			fprintf(fd, "#endif\n");

			fprintf(fd, "\n\t\t}");
		}
		break;

	case 'R':
		if (!terse && !TestOnly && has_xu)
		{	fprintf(fd, "\n#ifndef XUSAFE\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&1) || ");
			fprintf(fd, "q_R_check(");
			putname(fd, "", now->lft, m, ", II)) &&\n\t\t");
			putname(fd, "(!(q_claim[", now->lft, m, "]&2) || ");
			putname(fd, "q_S_check(", now->lft, m, ", II)) &&");
			fprintf(fd, "\n#endif\n\t\t");
		}
		if (u_sync>0)
			putname(fd, "not_RV(", now->lft, m, ") && \\\n\t\t");

		for (v = now->rgt, i=j=0; v; v = v->rgt, i++)
			if (v->lft->ntyp != CONST
			&&  v->lft->ntyp != EVAL)
			{	j++; continue;
			}
		if (now->val == 0 || i == j)
		{	putname(fd, "(q_len(", now->lft, m, ") > 0");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v->lft->ntyp != CONST
				&&  v->lft->ntyp != EVAL)
					continue;
				fprintf(fd, " \\\n\t\t&& qrecv(");
				putname(fd, "", now->lft, m, ", ");
				fprintf(fd, "0, %d, 0) == ", i);
				if (v->lft->ntyp == CONST)
					putstmnt(fd, v->lft, m);
				else /* EVAL */
					putstmnt(fd, v->lft->lft, m);
			}
			fprintf(fd, ")");
		} else
		{	putname(fd, "Q_has(", now->lft, m, "");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v->lft->ntyp == CONST)
				{	fprintf(fd, ", 1, ");
					putstmnt(fd, v->lft, m);
				} else if (v->lft->ntyp == EVAL)
				{	fprintf(fd, ", 1, ");
					putstmnt(fd, v->lft->lft, m);
				} else
					fprintf(fd, ", 0, 0");
			}	
			for ( ; i < Mpars; i++)
				fprintf(fd, ", 0, 0");
			fprintf(fd, ")");
		}
		break;

	case 'c':
		preruse(fd, now->lft);	/* preconditions */
		cat3("if (!(", now->lft, "))\n\t\t\t");
		Bailout(fd, "");
		break;

	case  ELSE:
		if (!GenCode)
		{	if (separate == 2)
				fprintf(fd, "if (o_pm&1)\n\t\t\t");
			else
				fprintf(fd, "if (trpt->o_pm&1)\n\t\t\t");
			Bailout(fd, "");
		} else
		{	fprintf(fd, "/* else */");
		}
		break;

	case '?':
		if (now->lft)
		{	cat3("( (", now->lft, ") ? ");
		}
		if (now->rgt)
		{	cat3("(", now->rgt->lft, ") : ");
			cat3("(", now->rgt->rgt, ") )");
		}
		break;

	case ASGN:
		if (has_enabled)
		fprintf(fd, "if (TstOnly) return 1;\n\t\t");
		_isok++;

		if (!GenCode)
		{	if (multi_oval)
			{	char tempbuf[64];
				check_needed();
				sprintf(tempbuf, "(trpt+1)->bup.ovals[%d] = ",
					multi_oval-1);
				multi_oval++;
				cat3(tempbuf, now->lft, ";\n\t\t");
			} else
			{	cat3("(trpt+1)->bup.oval = ", now->lft, ";\n\t\t");
		}	}
		nocast = 1; putstmnt(fd,now->lft,m); nocast = 0;
		fprintf(fd," = ");
		_isok--;
		putstmnt(fd,now->rgt,m);

		if (now->sym->type != CHAN
		||  verbose > 0)
		{	fprintf(fd, ";\n#ifdef VAR_RANGES");
			fprintf(fd, "\n\t\tlogval(\"");
			withprocname = terse = nocast = 1;
			_isok++;
			putstmnt(fd,now->lft,m);
			withprocname = terse = nocast = 0;
			fprintf(fd, "\", ");
			putstmnt(fd,now->lft,m);
			_isok--;
			fprintf(fd, ");\n#endif\n");
			fprintf(fd, "\t\t");
		}
		break;

	case PRINT:
		if (has_enabled)
		fprintf(fd, "if (TstOnly) return 1;\n\t\t");
#ifdef PRINTF
		fprintf(fd, "printf(%s", now->sym->name);
#else
		fprintf(fd, "Printf(%s", now->sym->name);
#endif
		for (v = now->lft; v; v = v->rgt)
		{	cat2(", ", v->lft);
		}
		fprintf(fd, ")");
		break;

	case PRINTM:
		if (has_enabled)
		fprintf(fd, "if (TstOnly) return 1;\n\t\t");
		fprintf(fd, "printm(");
		if (now->lft && now->lft->ismtyp)
			fprintf(fd, "%d", now->lft->val);
		else
			putstmnt(fd, now->lft, m);
		fprintf(fd, ")");
		break;

	case NAME:
		if (!nocast && now->sym && Sym_typ(now) < SHORT)
			putname(fd, "((int)", now, m, ")");
		else
			putname(fd, "", now, m, "");
		break;

	case   'p':
		putremote(fd, now, m);
		break;

	case   'q':
		if (terse)
			fprintf(fd, "%s", now->sym->name);
		else
			fprintf(fd, "%d", remotelab(now));
		break;

	case C_EXPR:
		fprintf(fd, "(");
		plunk_expr(fd, now->sym->name);
#if 1
		fprintf(fd, ")");
#else
		fprintf(fd, ") /* %s */ ", now->sym->name);
#endif
		break;

	case C_CODE:
		fprintf(fd, "/* %s */\n\t\t", now->sym->name);
		if (has_enabled)
			fprintf(fd, "if (TstOnly) return 1;\n\t\t");
		if (!GenCode)	/* not in d_step */
		{	fprintf(fd, "sv_save();\n\t\t");
			/* store the old values for reverse moves */
		}
		plunk_inline(fd, now->sym->name, 1);
		if (!GenCode)
		{	fprintf(fd, "\n");	/* state changed, capture it */
			fprintf(fd, "#if defined(C_States) && (HAS_TRACK==1)\n");
			fprintf(fd, "\t\tc_update((uchar *) &(now.c_state[0]));\n");
			fprintf(fd, "#endif\n");
		}
		break;

	case ASSERT:
		if (has_enabled)
			fprintf(fd, "if (TstOnly) return 1;\n\t\t");

		cat3("assert(", now->lft, ", ");
		terse = nocast = 1;
		cat3("\"", now->lft, "\", II, tt, t)");
		terse = nocast = 0;
		break;

	case '.':
	case BREAK:
	case GOTO:
		if (Pid == eventmapnr)
			fprintf(fd, "Uerror(\"cannot get here\")");
		putskip(m);
		break;

	case '@':
		if (Pid == eventmapnr)
		{	fprintf(fd, "return 0");
			break;
		}

		if (has_enabled)
		{	fprintf(fd, "if (TstOnly)\n\t\t\t");
			fprintf(fd, "return (II+1 == now._nr_pr);\n\t\t");
		}
		fprintf(fd, "if (!delproc(1, II)) ");
		Bailout(fd, "");
		break;

	default:
		printf("spin: bad node type %d (.m) - line %d\n",
			now->ntyp, now->ln);
		fflush(tm);
		alldone(1);
	}
}

void
putname(FILE *fd, char *pre, Lextok *n, int m, char *suff) /* varref */
{	Symbol *s = n->sym;
	lineno = n->ln; Fname = n->fn;

	if (!s)
		fatal("no name - putname", (char *) 0);

	if (s->context && context && s->type)
		s = findloc(s);		/* it's a local var */

	if (!s)
	{	fprintf(fd, "%s%s%s", pre, n->sym->name, suff);
		return;
	}
	if (!s->type)	/* not a local name */
		s = lookup(s->name);	/* must be a global */

	if (!s->type)
	{	if (strcmp(pre, ".") != 0)
		non_fatal("undeclared variable '%s'", s->name);
		s->type = INT;
	}

	if (s->type == PROCTYPE)
		fatal("proctype-name '%s' used as array-name", s->name);

	fprintf(fd, pre);
	if (!terse && !s->owner && evalindex != 1)
	{	if (s->context
		||  strcmp(s->name, "_p") == 0
		||  strcmp(s->name, "_pid") == 0)
		{	fprintf(fd, "((P%d *)this)->", Pid);
		} else
		{	int x = strcmp(s->name, "_");
			if (!(s->hidden&1) && x != 0)
				fprintf(fd, "now.");
			if (x == 0 && _isok == 0)
				fatal("attempt to read value of '_'", 0);
	}	}

	if (withprocname
	&&  s->context
	&&  strcmp(pre, "."))
		fprintf(fd, "%s:", s->context->name);

	if (evalindex != 1)
		fprintf(fd, "%s", s->name);

	if (s->nel != 1)
	{	if (no_arrays)
		{
		non_fatal("ref to array element invalid in this context",
			(char *)0);
		printf("\thint: instead of, e.g., x[rs] qu[3], use\n");
		printf("\tchan nm_3 = qu[3]; x[rs] nm_3;\n");
		printf("\tand use nm_3 in sends/recvs instead of qu[3]\n");
		}
		/* an xr or xs reference to an array element
		 * becomes an exclusion tag on the array itself -
		 * which could result in invalidly labeling
		 * operations on other elements of this array to
		 * be also safe under the partial order reduction
		 * (see procedure has_global())
		 */

		if (evalindex == 2)
		{	fprintf(fd, "[%%d]");
		} else if (evalindex == 1)
		{	evalindex = 0;		/* no good if index is indexed array */
			fprintf(fd, ", ");
			putstmnt(fd, n->lft, m);
			evalindex = 1;
		} else
		{	if (terse
			|| (n->lft
			&&  n->lft->ntyp == CONST
			&&  n->lft->val < s->nel)
			|| (!n->lft && s->nel > 0))
			{	cat3("[", n->lft, "]");
			} else
			{	cat3("[ Index(", n->lft, ", ");
				fprintf(fd, "%d) ]", s->nel);
			}
		}
	}
	if (s->type == STRUCT && n->rgt && n->rgt->lft)
	{	putname(fd, ".", n->rgt->lft, m, "");
	}
	fprintf(fd, suff);
}

void
putremote(FILE *fd, Lextok *n, int m)	/* remote reference */
{	int promoted = 0;
	int pt;

	if (terse)
	{	fprintf(fd, "%s", n->lft->sym->name);	/* proctype name */
		if (n->lft->lft)
		{	fprintf(fd, "[");
			putstmnt(fd, n->lft->lft, m);	/* pid */
			fprintf(fd, "]");
		}
		fprintf(fd, ".%s", n->sym->name);
	} else
	{	if (Sym_typ(n) < SHORT)
		{	promoted = 1;
			fprintf(fd, "((int)");
		}

		pt = fproc(n->lft->sym->name);
		fprintf(fd, "((P%d *)Pptr(", pt);
		if (n->lft->lft)
		{	fprintf(fd, "BASE+");
			putstmnt(fd, n->lft->lft, m);
		} else
			fprintf(fd, "f_pid(%d)", pt);
		fprintf(fd, "))->%s", n->sym->name);
	}
	if (n->rgt)
	{	fprintf(fd, "[");
		putstmnt(fd, n->rgt, m);	/* array var ref */
		fprintf(fd, "]");
	}
	if (promoted) fprintf(fd, ")");
}

static int
getweight(Lextok *n)
{	/* this piece of code is a remnant of early versions
	 * of the verifier -- in the current version of Spin
	 * only non-zero values matter - so this could probably
	 * simply return 1 in all cases.
	 */
	switch (n->ntyp) {
	case 'r':     return 4;
	case 's':     return 2;
	case TIMEOUT: return 1;
	case 'c':     if (has_typ(n->lft, TIMEOUT)) return 1;
	}
	return 3;
}

int
has_typ(Lextok *n, int m)
{
	if (!n) return 0;
	if (n->ntyp == m) return 1;
	return (has_typ(n->lft, m) || has_typ(n->rgt, m));
}

static int runcount, opcount;

static void
do_count(Lextok *n, int checkop)
{
	if (!n) return;

	switch (n->ntyp) {
	case RUN:
		runcount++;
		break;
	default:
		if (checkop) opcount++;
		break;
	}
	do_count(n->lft, checkop && (n->ntyp != RUN));
	do_count(n->rgt, checkop);
}

void
count_runs(Lextok *n)
{
	runcount = opcount = 0;
	do_count(n, 1);
	if (runcount > 1)
		fatal("more than one run operator in expression", "");
	if (runcount == 1 && opcount > 1)
		fatal("use of run operator in compound expression", "");
}

void
any_runs(Lextok *n)
{
	runcount = opcount = 0;
	do_count(n, 0);
	if (runcount >= 1)
		fatal("run operator used in invalid context", "");
}
