/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/***** spin: pangen3.h *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */
/* (c) 2007: small additions for V5.0 to support multi-core verifications */

static char* Head0[] = {
    "#if defined(BFS) && defined(REACH)",
    "	#undef REACH", /* redundant with bfs */
    "#endif", "#ifdef VERI", "	#define BASE	1", "#else",
    "	#define BASE	0", "#endif", "typedef struct Trans {",
    "	short atom;	/* if &2 = atomic trans; if &8 local */",
    "#ifdef HAS_UNLESS",
    "	short escp[HAS_UNLESS];	/* lists the escape states */",
    "	short e_trans;	/* if set, this is an escp-trans */", "#endif",
    "	short tpe[2];	/* class of operation (for reduction) */",
    "	short qu[6];	/* for conditional selections: qid's  */",
    "	uchar ty[6];	/* ditto: type's */", "#ifdef NIBIS",
    "	short om;	/* completion status of preselects */", "#endif",
    "	char *tp;	/* src txt of statement */",
    "	int st;		/* the nextstate */",
    "	int t_id;	/* transition id, unique within proc */",
    "	int forw;	/* index forward transition */",
    "	int back;	/* index return  transition */",
    "	struct Trans *nxt;", "} Trans;\n",

    "#ifdef TRIX", "	#define qptr(x)	(channels[x]->body)",
    "	#define pptr(x)	(processes[x]->body)", "#else",
    "	#define qptr(x)	(((uchar *)&now)+(int)q_offset[x])",
    "	#define pptr(x)	(((uchar *)&now)+(int)proc_offset[x])",
    /*	"	#define Pptr(x)	((proc_offset[x])?pptr(x):noptr)",	*/
    "#endif", "extern uchar *Pptr(int);", "extern uchar *Qptr(int);",

    "#define q_sz(x)	(((Q0 *)qptr(x))->Qlen)", "", "#ifdef TRIX",
    "	#ifdef VECTORSZ",
    "		#undef VECTORSZ", /* backward compatibility */
    "	#endif", "	#if WS==4",
    "		#define VECTORSZ	2056	/* "
    "((MAXPROC+MAXQ+4)*sizeof(uchar *)) */",
    "	#else", "		#define VECTORSZ	4112	/* the formula "
                "causes probs in preprocessing */",
    "	#endif", "#else", "	#ifndef VECTORSZ",
    "		#define VECTORSZ	1024	/* sv size in bytes */",
    "	#endif", "#endif\n", 0,
};

static char* Header[] = {
    "#ifdef VERBOSE", "	#ifndef CHECK", "		#define CHECK",
    "	#endif", "	#ifndef DEBUG", "		#define DEBUG",
    "	#endif", "#endif", "#ifdef SAFETY", "	#ifndef NOFAIR",
    "		#define NOFAIR", "	#endif", "#endif", "#ifdef NOREDUCE",
    "	#ifndef XUSAFE", "		#define XUSAFE", "	#endif",
    "	#if !defined(SAFETY) && !defined(MA)",
    "		#define FULLSTACK", "	#endif", "#else",
    "	#ifdef BITSTATE",
    "		#if defined(SAFETY) && !defined(HASH64)",
    "			#define CNTRSTACK", "		#else",
    "			#define FULLSTACK", "		#endif", "	#else",
    "		#define FULLSTACK", "	#endif", "#endif", "#ifdef BITSTATE",
    "	#ifndef NOCOMP", "		#define NOCOMP", "	#endif",
    "	#if !defined(LC) && defined(SC)", "		#define LC", "	#endif",
    "#endif",
    "#if defined(COLLAPSE2) || defined(COLLAPSE3) || defined(COLLAPSE4)",
    "	/* accept the above for backward compatibility */",
    "	#define COLLAPSE", "#endif", "#ifdef HC", "	#undef HC",
    "	#define HC4", "#endif", "#ifdef HC0",       /* 32 bits */
    "	#define HC	0", "#endif", "#ifdef HC1", /* 32+8 bits */
    "	#define HC	1", "#endif", "#ifdef HC2", /* 32+16 bits */
    "	#define HC	2", "#endif", "#ifdef HC3", /* 32+24 bits */
    "	#define HC	3", "#endif",
    "#ifdef HC4", /* 32+32 bits - combine with -DMA=8 */
    "	#define HC	4", "#endif", "#ifdef COLLAPSE",
    "	#if NCORE>1 && !defined(SEP_STATE)",
    "		unsigned long *ncomps;	/* in shared memory */", "	#else",
    "		unsigned long ncomps[256+2];", "	#endif", "#endif",

    "#define MAXQ   	255", "#define MAXPROC	255", "",
    "typedef struct _Stack  {	 /* for queues and processes */",
    "#if VECTORSZ>32000", "	int o_delta;", "	#ifndef TRIX",
    "		int o_offset;", "		int o_skip;", "	#endif",
    "	int o_delqs;", "#else", "	short o_delta;", "	#ifndef TRIX",
    "		short o_offset;", "		short o_skip;", "	#endif",
    "	short o_delqs;", "#endif", "	short o_boq;", "#ifdef TRIX",
    "	short parent;",
    "	char *b_ptr;",        /* used in delq/q_restor and delproc/p_restor */
    "#else", "	char *body;", /* full copy of state vector in non-trix mode */
    "#endif", "#ifndef XUSAFE", "	char *o_name;", "#endif",
    "	struct _Stack *nxt;", "	struct _Stack *lst;", "} _Stack;\n",
    "typedef struct Svtack { /* for complete state vector */",
    "#if VECTORSZ>32000", "	int o_delta;", "	int m_delta;", "#else",
    "	short o_delta;	 /* current size of frame */",
    "	short m_delta;	 /* maximum size of frame */", "#endif", "#if SYNC",
    "	short o_boq;", "#endif", 0,
};

static char* Header0[] = {
    "	char *body;", "	struct Svtack *nxt;", "	struct Svtack *lst;",
    "} Svtack;\n", "Trans ***trans;	/* 1 ptr per state per proctype */\n",
    "struct H_el *Lstate;", "int depthfound = -1;	/* loop detection */",

    "#ifndef TRIX", "	#if VECTORSZ>32000",
    "		int proc_offset[MAXPROC];", "		int q_offset[MAXQ];",
    "	#else", "		short proc_offset[MAXPROC];",
    "		short q_offset[MAXQ];", "	#endif",
    "	uchar proc_skip[MAXPROC];", "	uchar q_skip[MAXQ];", "#endif",

    "unsigned long  vsize;	/* vector size in bytes */", "#ifdef SVDUMP",
    "	int vprefix=0, svfd;	/* runtime option -pN */", "#endif",
    "char *tprefix = \"trail\";	/* runtime option -tsuffix */",
    "short boq = -1;		/* blocked_on_queue status */", 0,
};

static char* Head1[] = {
    "typedef struct State {", "	uchar _nr_pr;", "	uchar _nr_qs;",
    "	uchar   _a_t;	/* cycle detection */",
#if 0
	in _a_t: bits 0,4, and 5 =(1|16|32) are set during a 2nd dfs
	bit 1 is used as the A-bit for fairness
	bit 7 (128) is the proviso bit, for reduced 2nd dfs (acceptance)
#endif
    "#ifndef NOFAIR", "	uchar   _cnt[NFAIR];	/* counters, weak fairness */",
    "#endif",

    "#ifndef NOVSZ",
#ifdef SOLARIS
    "#if 0",
/* v3.4
 * noticed alignment problems with some Solaris
 * compilers, if widest field isn't wordsized
 */
#else
    "#if VECTORSZ<65536",
#endif
    "	unsigned short _vsz;", "#else", "	unsigned long  _vsz;", "#endif",
    "#endif",

    "#ifdef HAS_LAST", /* cannot go before _cnt - see hstore() */
    "	uchar  _last;	/* pid executed in last step */", "#endif",

    "#if defined(BITSTATE) && defined(BCS) && defined(STORE_CTX)",
    "	uchar  _ctx;	/* nr of context switches so far */", "#endif",

    "#ifdef EVENT_TRACE", "	#if nstates_event<256",
    "		uchar _event;", "	#else",
    "		unsigned short _event;", "	#endif", "#endif", 0,
};

static char* Addp0[] = {
    /* addproc(....parlist... */ ")", "{	int j, h = now._nr_pr;",
    "#ifndef NOCOMP", "	int k;", "#endif", "	uchar *o_this = this;\n",
    "#ifndef INLINE", "	if (TstOnly) return (h < MAXPROC);", "#endif",
    "#ifndef NOBOUNDCHECK",
    "	/* redefine Index only within this procedure */", "	#undef Index",
    "	#define Index(x, y)	Boundcheck(x, y, 0, 0, 0)", "#endif",
    "	if (h >= MAXPROC)", "		Uerror(\"too many processes\");",
    "#ifdef V_TRIX", "	printf(\"%%4d: add process %%d\\n\", depth, h);",
    "#endif", "	switch (n) {", "	case 0: j = sizeof(P0); break;", 0,
};

static char* Addp1[] = {
    "	default: Uerror(\"bad proc - addproc\");", "	}",

    "#ifdef TRIX", "	vsize += sizeof(struct H_el *);", "#else",
    "	if (vsize%%WS)", "		proc_skip[h] = WS-(vsize%%WS);",
    "	else", "		proc_skip[h] = 0;", "	#ifndef NOCOMP",
    "		for (k = vsize + (int) proc_skip[h]; k > vsize; k--)",
    "			Mask[k-1] = 1; /* align */", "	#endif",
    "	vsize += (int) proc_skip[h];", "	proc_offset[h] = vsize;",
    "	vsize += j;", "	#if defined(SVDUMP) && defined(VERBOSE)",
    "	if (vprefix > 0)", "	{	int dummy = 0;",
    "		write(svfd, (uchar *) &dummy, sizeof(int)); /* mark */",
    "		write(svfd, (uchar *) &h, sizeof(int));",
    "		write(svfd, (uchar *) &n, sizeof(int));",
    "	#if VECTORSZ>32000",
    "		write(svfd, (uchar *) &proc_offset[h], sizeof(int));",
    "		write(svfd, (uchar *) &now, vprefix-4*sizeof(int)); /* padd */",
    "	#else",
    "		write(svfd, (uchar *) &proc_offset[h], sizeof(short));",
    "		write(svfd, (uchar *) &now, "
    "vprefix-3*sizeof(int)-sizeof(short)); /* padd */",
    "	#endif", "	}", "	#endif", "#endif",

    "	now._nr_pr += 1;", "#if defined(BCS) && defined(CONSERVATIVE)",
    "	if (now._nr_pr >= CONSERVATIVE*8)",
    "	{	printf(\"pan: error: too many processes -- recompile with \");",
    "		printf(\"-DCONSERVATIVE=%%d\\n\", CONSERVATIVE+1);",
    "		pan_exit(1);", "	}", "#endif",
    "	if (fairness && ((int) now._nr_pr + 1 >= (8*NFAIR)/2))",
    "	{	printf(\"pan: error: too many processes -- current\");",
    "		printf(\" max is %%d procs (-DNFAIR=%%d)\\n\",",
    "			(8*NFAIR)/2 - 2, NFAIR);",
    "		printf(\"\\trecompile with -DNFAIR=%%d\\n\",",
    "			NFAIR+1);", "		pan_exit(1);", "	}",
    "#ifndef NOVSZ", "	now._vsz = vsize;", "#endif",
    "	hmax = max(hmax, vsize);",

    "#ifdef TRIX", "	#ifndef BFS", "		if (freebodies)",
    "		{	processes[h] = freebodies;",
    "			freebodies = freebodies->nxt;", "		} else",
    "		{	processes[h] = (TRIX_v6 *) emalloc(sizeof(TRIX_v6));",
    "			processes[h]->body = (uchar *) emalloc(Maxbody * "
    "sizeof(char));",
    "		}", "		processes[h]->modified = 1;	/* addproc */",
    "	#endif", "	processes[h]->psize = j;",
    "	processes[h]->parent_pid = calling_pid;",
    "	processes[h]->nxt = (TRIX_v6 *) 0;", "#else", "	#ifndef NOCOMP",
    "		for (k = 1; k <= Air[n]; k++)",
    "			Mask[vsize - k] = 1; /* pad */",
    "		Mask[vsize-j] = 1; /* _pid */", "	#endif",
    "	if (vsize >= VECTORSZ)",
    "	{	printf(\"pan: error, VECTORSZ too small, recompile pan.c\");",
    "		printf(\" with -DVECTORSZ=N with N>%%d\\n\", (int) vsize);",
    "		Uerror(\"aborting\");", "	}", "#endif",

    "	memset((char *)pptr(h), 0, j);", "	this = pptr(h);",
    "	if (BASE > 0 && h > 0)", "		((P0 *)this)->_pid = h-BASE;",
    "	else", "		((P0 *)this)->_pid = h;", "	switch (n) {",
    0,
};

static char* Addq0[] = {
    "int", "addqueue(int calling_pid, int n, int is_rv)",
    "{	int j=0, i = now._nr_qs;", "#if !defined(NOCOMP) && !defined(TRIX)",
    "	int k;", "#endif", "	if (i >= MAXQ)",
    "		Uerror(\"too many queues\");", "#ifdef V_TRIX",
    "	printf(\"%%4d: add queue %%d\\n\", depth, i);", "#endif",
    "	switch (n) {", 0,
};

static char* Addq1[] = {
    "	default: Uerror(\"bad queue - addqueue\");", "	}",

    "#ifdef TRIX", "	vsize += sizeof(struct H_el *);", "#else",
    "	if (vsize%%WS)", "		q_skip[i] = WS-(vsize%%WS);", "	else",
    "		q_skip[i] = 0;", "	#ifndef NOCOMP",
    "		k = vsize;", "		#ifndef BFS",
    "			if (is_rv) k += j;", "		#endif",
    "		for (k += (int) q_skip[i]; k > vsize; k--)",
    "			Mask[k-1] = 1;", "	#endif",
    "	vsize += (int) q_skip[i];", "	q_offset[i] = vsize;",
    "	vsize += j;", "#endif",

    "	now._nr_qs += 1;", "#ifndef NOVSZ", "	now._vsz = vsize;", "#endif",
    "	hmax = max(hmax, vsize);",

    "#ifdef TRIX", "	#ifndef BFS", "		if (freebodies)",
    "		{	channels[i] = freebodies;",
    "			freebodies = freebodies->nxt;", "		} else",
    "		{	channels[i] = (TRIX_v6 *) emalloc(sizeof(TRIX_v6));",
    "			channels[i]->body = (uchar *) emalloc(Maxbody * "
    "sizeof(char));",
    "		}", "		channels[i]->modified = 1;	/* addq */",
    "	#endif", "	channels[i]->psize = j;",
    "	channels[i]->parent_pid = calling_pid;",
    "	channels[i]->nxt = (TRIX_v6 *) 0;", "#else", "	if (vsize >= VECTORSZ)",
    "		Uerror(\"VECTORSZ is too small, edit pan.h\");", "#endif",

    "	if (j > 0)", /* zero if there are no queues */
    "	{	memset((char *)qptr(i), 0, j);", "	}",
    "	((Q0 *)qptr(i))->_t = n;", "	return i+1;", "}\n", 0,
};

static char* Addq11[] = {
    "{	int j; uchar *z;\n", "#ifdef HAS_SORTED", "	int k;", "#endif",
    "	if (!into--)",
    "	uerror(\"ref to uninitialized chan name (sending)\");",
    "	if (into >= (int) now._nr_qs || into < 0)",
    "		Uerror(\"qsend bad queue#\");",
    "#if defined(TRIX) && !defined(BFS)", "	#ifndef TRIX_ORIG",
    "		(trpt+1)->q_bup = now._ids_[now._nr_pr+into];",
    "		#ifdef V_TRIX",
    "			printf(\"%%4d: channel %%d s save %%p from %%d\\n\",",
    "			depth, into, (trpt+1)->q_bup, now._nr_pr+into);",
    "		#endif", "	#endif",
    "	channels[into]->modified = 1;	/* qsend */", "	#ifdef V_TRIX",
    "		printf(\"%%4d: channel %%d modified\\n\", depth, into);",
    "	#endif", "#endif", "	z = qptr(into);",
    "	j = ((Q0 *)qptr(into))->Qlen;",
    "	switch (((Q0 *)qptr(into))->_t) {", 0,
};

static char* Addq2[] = {
    "	case 0: printf(\"queue %%d was deleted\\n\", into+1);",
    "	default: Uerror(\"bad queue - qsend\");", "	}",
    "#ifdef EVENT_TRACE", "	if (in_s_scope(into+1))",
    "		require('s', into);", "#endif", "}", "#endif\n", "#if SYNC",
    "int", "q_zero(int from)", "{	if (!from--)",
    "	{	uerror(\"ref to uninitialized chan name (q_zero)\");",
    "		return 0;", "	}", "	switch(((Q0 *)qptr(from))->_t) {", 0,
};

static char* Addq3[] = {
    "	case 0: printf(\"queue %%d was deleted\\n\", from+1);", "	}",
    "	Uerror(\"bad queue q-zero\");", "	return -1;", "}", "int",
    "not_RV(int from)", "{	if (q_zero(from))",
    "	{	printf(\"==>> a test of the contents of a rv \");",
    "		printf(\"channel always returns FALSE\\n\");",
    "		uerror(\"error to poll rendezvous channel\");", "	}",
    "	return 1;", "}", "#endif", "#ifndef XUSAFE", "void",
    "setq_claim(int x, int m, char *s, int y, char *p)", "{	if (x == 0)",
    "	uerror(\"x[rs] claim on uninitialized channel\");",
    "	if (x < 0 || x > MAXQ)",
    "		Uerror(\"cannot happen setq_claim\");",
    "	q_claim[x] |= m;", "	p_name[y] = p;", "	q_name[x] = s;",
    "	if (m&2) q_S_check(x, y);", "	if (m&1) q_R_check(x, y);", "}",
    "short q_sender[MAXQ+1];", "int", "q_S_check(int x, int who)",
    "{	if (!q_sender[x])", "	{	q_sender[x] = who+1;", "#if SYNC",
    "		if (q_zero(x))",
    "		{	printf(\"chan %%s (%%d), \",",
    "				q_name[x], x-1);",
    "			printf(\"sndr proc %%s (%%d)\\n\",",
    "				p_name[who], who);",
    "			uerror(\"xs chans cannot be used for rv\");",
    "		}", "#endif", "	} else", "	if (q_sender[x] != who+1)",
    "	{	printf(\"pan: xs assertion violated: \");",
    "		printf(\"access to chan <%%s> (%%d)\\npan: by \",",
    "			q_name[x], x-1);",
    "		if (q_sender[x] > 0 && p_name[q_sender[x]-1])",
    "			printf(\"%%s (proc %%d) and by \",",
    "			p_name[q_sender[x]-1], q_sender[x]-1);",
    "		printf(\"%%s (proc %%d)\\n\",",
    "			p_name[who], who);",
    "		uerror(\"error, partial order reduction invalid\");", "	}",
    "	return 1;", "}", "short q_recver[MAXQ+1];", "int",
    "q_R_check(int x, int who)", "{	if (!q_recver[x])",
    "	{	q_recver[x] = who+1;", "#if SYNC", "		if (q_zero(x))",
    "		{	printf(\"chan %%s (%%d), \",",
    "				q_name[x], x-1);",
    "			printf(\"recv proc %%s (%%d)\\n\",",
    "				p_name[who], who);",
    "			uerror(\"xr chans cannot be used for rv\");",
    "		}", "#endif", "	} else", "	if (q_recver[x] != who+1)",
    "	{	printf(\"pan: xr assertion violated: \");",
    "		printf(\"access to chan %%s (%%d)\\npan: \",",
    "			q_name[x], x-1);",
    "		if (q_recver[x] > 0 && p_name[q_recver[x]-1])",
    "			printf(\"by %%s (proc %%d) and \",",
    "			p_name[q_recver[x]-1], q_recver[x]-1);",
    "		printf(\"by %%s (proc %%d)\\n\",",
    "			p_name[who], who);",
    "		uerror(\"error, partial order reduction invalid\");", "	}",
    "	return 1;", "}", "#endif", "int", "q_len(int x)", "{	if (!x--)",
    "	uerror(\"ref to uninitialized chan name (len)\");",
    "	return ((Q0 *)qptr(x))->Qlen;", "}\n", "int", "q_full(int from)",
    "{	if (!from--)",
    "	uerror(\"ref to uninitialized chan name (qfull)\");",
    "	switch(((Q0 *)qptr(from))->_t) {", 0,
};

static char* Addq4[] = {
    "	case 0: printf(\"queue %%d was deleted\\n\", from+1);", "	}",
    "	Uerror(\"bad queue - q_full\");", "	return 0;", "}\n",
    "#ifdef HAS_UNLESS", "int", "q_e_f(int from)", "{	/* empty or full */",
    "	return !q_len(from) || q_full(from);", "}", "#endif", "#if NQS>0",
    "int", "qrecv(int from, int slot, int fld, int done)", "{	uchar *z;",
    "	int j, k, r=0;\n", "	if (!from--)",
    "	uerror(\"ref to uninitialized chan name (receiving)\");",
    "#if defined(TRIX) && !defined(BFS)", "	#ifndef TRIX_ORIG",
    "		(trpt+1)->q_bup = now._ids_[now._nr_pr+from];",
    "		#ifdef V_TRIX",
    "			printf(\"%%4d: channel %%d r save %%p from %%d\\n\",",
    "			depth, from, (trpt+1)->q_bup, now._nr_pr+from);",
    "		#endif", "	#endif",
    "	channels[from]->modified = 1;	/* qrecv */", "	#ifdef V_TRIX",
    "		printf(\"%%4d: channel %%d modified\\n\", depth, from);",
    "	#endif", "#endif", "	if (from >= (int) now._nr_qs || from < 0)",
    "		Uerror(\"qrecv bad queue#\");", "	z = qptr(from);",
    "#ifdef EVENT_TRACE", "	if (done && (in_r_scope(from+1)))",
    "		require('r', from);", "#endif",
    "	switch (((Q0 *)qptr(from))->_t) {", 0,
};

static char* Addq5[] = {
    "	case 0: printf(\"queue %%d was deleted\\n\", from+1);",
    "	default: Uerror(\"bad queue - qrecv\");", "	}", "	return r;", "}",
    "#endif\n", "#ifndef BITSTATE", "#ifdef COLLAPSE", "long",
    "col_q(int i, char *z)", "{	int j=0, k;", "	char *x, *y;",
    "	Q0 *ptr = (Q0 *) qptr(i);", "	switch (ptr->_t) {", 0,
};

static char* Code0[] = {
    "void", "run(void)", "{	/* int i; */",
    "	memset((char *)&now, 0, sizeof(State));",
    "	vsize = (unsigned long) (sizeof(State) - VECTORSZ);", "#ifndef NOVSZ",
    "	now._vsz = vsize;", "#endif", "#ifdef TRIX",
    "	if (VECTORSZ != sizeof(now._ids_))",
    "	{	printf(\"VECTORSZ is %%d, but should be %%d in this mode\\n\",",
    "			VECTORSZ, sizeof(now._ids_));",
    "		Uerror(\"VECTORSZ set incorrectly, recompile Spin (not "
    "pan.c)\");",
    "	}", "#endif", "/* optional provisioning statements, e.g. to */",
    "/* set hidden variables, used as constants */", "#ifdef PROV",
    "#include PROV", "#endif", "	settable();", 0,
};

static char* R0[] = {
    "	Maxbody = max(Maxbody, ((int) sizeof(P%d)));",
    "	reached[%d] = reached%d;",
    "	accpstate[%d] = (uchar *) emalloc(nstates%d);",
    "	progstate[%d] = (uchar *) emalloc(nstates%d);",
    "	loopstate%d = loopstate[%d] = (uchar *) emalloc(nstates%d);",
    "	stopstate[%d] = (uchar *) emalloc(nstates%d);",
    "	visstate[%d] = (uchar *) emalloc(nstates%d);",
    "	mapstate[%d] = (short *) emalloc(nstates%d * sizeof(short));",
    "#ifdef HAS_CODE", "	NrStates[%d] = nstates%d;", "#endif",
    "	stopstate[%d][endstate%d] = 1;", 0,
};

static char* R0a[] = {
    "	retrans(%d, nstates%d, start%d, src_ln%d, reached%d, loopstate%d);", 0,
};

static char* Code1[] = {
    "#ifdef NP", "	#define ACCEPT_LAB	1 /* at least 1 in np_ */",
    "#else", "	#define ACCEPT_LAB	%d /* user-defined accept labels */",
    "#endif", "#ifdef MEMCNT", "	#ifdef MEMLIM",
    "		#warning -DMEMLIM takes precedence over -DMEMCNT",
    "		#undef MEMCNT", "	#else", "		#if MEMCNT<20",
    "			#warning using minimal value -DMEMCNT=20 (=1MB)",
    "			#define MEMLIM	(1)", "			#undef MEMCNT",
    "		#else", "			#if MEMCNT==20",
    "				#define MEMLIM	(1)",
    "				#undef MEMCNT", "			#else",
    "			 #if MEMCNT>=50",
    "				#error excessive value for MEMCNT",
    "			 #else",
    "				#define MEMLIM	(1<<(MEMCNT-20))",
    "			 #endif", "			#endif",
    "		#endif", "	#endif", "#endif",

    "#if NCORE>1 && !defined(MEMLIM)",
    "	#define MEMLIM	(2048)	/* need a default, using 2 GB */", "#endif", 0,
};

static char* Code3[] = {
    "#define PROG_LAB	%d /* progress labels */", 0,
};

static char* R2[] = {
    "uchar *accpstate[%d];", "uchar *progstate[%d];", "uchar *loopstate[%d];",
    "uchar *reached[%d];", "uchar *stopstate[%d];", "uchar *visstate[%d];",
    "short *mapstate[%d];", "#ifdef HAS_CODE", "	int NrStates[%d];",
    "#endif", 0,
};
static char* R3[] = {
    "	Maxbody = max(Maxbody, ((int) sizeof(Q%d)));", 0,
};
static char* R4[] = {
    "	r_ck(reached%d, nstates%d, %d, src_ln%d, src_file%d);", 0,
};
static char* R5[] = {
    "	case %d: j = sizeof(P%d); break;", 0,
};
static char* R6[] = {
    "	}", "	this = o_this;", "#ifdef TRIX",
    "	re_mark_all(1); /* addproc */", "#endif", "	return h-BASE;",
    "#ifndef NOBOUNDCHECK", "	#undef Index",
    "	#define Index(x, y)	Boundcheck(x, y, II, tt, t)", "#endif", "}\n",
    "#if defined(BITSTATE) && defined(COLLAPSE)",
    "	/* just to allow compilation, to generate the error */",
    "	long col_p(int i, char *z) { return 0; }",
    "	long col_q(int i, char *z) { return 0; }", "#endif", "#ifndef BITSTATE",
    "	#ifdef COLLAPSE", "long", "col_p(int i, char *z)",
    "{	int j, k; unsigned long ordinal(char *, long, short);",
    "	char *x, *y;", "	P0 *ptr = (P0 *) pptr(i);",
    "	switch (ptr->_t) {", "	case 0: j = sizeof(P0); break;", 0,
};
static char* R8a[] = {
    "	default: Uerror(\"bad proctype - collapse\");", "	}",
    "	if (z) x = z; else x = scratch;",
    "	y = (char *) ptr; k = proc_offset[i];",

    "	for ( ; j > 0; j--, y++)", "		if (!Mask[k++]) *x++ = *y;",

    "	for (j = 0; j < WS-1; j++)", "		*x++ = 0;", "	x -= j;",
    "	if (z) return (long) (x - z);",
    "	return ordinal(scratch, x-scratch, (short) (2+ptr->_t));", "}",
    "	#endif", "#endif", 0,
};
static char* R8b[] = {
    "	default: Uerror(\"bad qtype - collapse\");", "	}",
    "	if (z) x = z; else x = scratch;",
    "	y = (char *) ptr; k = q_offset[i];",

    "	/* no need to store the empty slots at the end */",
    "	j -= (q_max[ptr->_t] - ptr->Qlen) * ((j - 2)/q_max[ptr->_t]);",

    "	for ( ; j > 0; j--, y++)", "		if (!Mask[k++]) *x++ = *y;",

    "	for (j = 0; j < WS-1; j++)", "		*x++ = 0;", "	x -= j;",
    "	if (z) return (long) (x - z);",
    "	return ordinal(scratch, x-scratch, 1); /* chan */", "}", "	#endif",
    "#endif", 0,
};

static char* R12[] = {
    "\t\tcase %d: r = ((Q%d *)z)->contents[slot].fld%d; break;", 0,
};
char* R13[] = {
    "int ", "unsend(int into)", "{	int _m=0, j; uchar *z;\n",
    "#ifdef HAS_SORTED", "	int k;", "#endif", "	if (!into--)",
    "		uerror(\"ref to uninitialized chan (unsend)\");",
    "#if defined(TRIX) && !defined(BFS)", "	#ifndef TRIX_ORIG",
    "		now._ids_[now._nr_pr+into] = trpt->q_bup;",
    "		#ifdef V_TRIX", "			printf(\"%%4d: channel "
                                "%%d s restore %%p into %%d\\n\",",
    "				depth, into, trpt->q_bup, now._nr_pr+into);",
    "		#endif", "	#else",
    "		channels[into]->modified = 1;	/* unsend */",
    "		#ifdef V_TRIX", "			printf(\"%%4d: channel "
                                "%%d unmodify\\n\", depth, into);",
    "		#endif", "	#endif", "#endif", "	z = qptr(into);",
    "	j = ((Q0 *)z)->Qlen;", "	((Q0 *)z)->Qlen = --j;",
    "	switch (((Q0 *)qptr(into))->_t) {", 0,
};
char* R14[] = {
    "	default: Uerror(\"bad queue - unsend\");", "	}", "	return _m;",
    "}\n", "void", "unrecv(int from, int slot, int fld, int fldvar, int strt)",
    "{	int j; uchar *z;\n", "	if (!from--)",
    "		uerror(\"ref to uninitialized chan (unrecv)\");",
    "#if defined(TRIX) && !defined(BFS)", "	#ifndef TRIX_ORIG",
    "		now._ids_[now._nr_pr+from] = trpt->q_bup;",
    "		#ifdef V_TRIX", "			printf(\"%%4d: channel "
                                "%%d r restore %%p into %%d\\n\",",
    "				depth, from, trpt->q_bup, now._nr_pr+from);",
    "		#endif", "	#else",
    "		channels[from]->modified = 1;	/* unrecv */",
    "		#ifdef V_TRIX", "			printf(\"%%4d: channel "
                                "%%d unmodify\\n\", depth, from);",
    "		#endif", "	#endif", "#endif", "	z = qptr(from);",
    "	j = ((Q0 *)z)->Qlen;", "	if (strt) ((Q0 *)z)->Qlen = j+1;",
    "	switch (((Q0 *)qptr(from))->_t) {", 0,
};
char* R15[] = {
    "	default: Uerror(\"bad queue - qrecv\");", "	}", "}", 0,
};
static char* Proto[] = {
    "", "/** function prototypes **/", "char *emalloc(unsigned long);",
    "char *Malloc(unsigned long);",
    "int Boundcheck(int, int, int, int, Trans *);",
    "int addqueue(int, int, int);", "/* int atoi(char *); */",
    "/* int abort(void); */",
    "int close(int);", /* should probably remove this */
#if 0
	"#ifndef SC",
	"	int creat(char *, unsigned short);",
	"	int write(int, void *, unsigned);",
	"#endif",
#endif
    "int delproc(int, int);", "int endstate(void);", "int hstore(char *, int);",
    "#ifdef MA", "int gstore(char *, int, uchar);", "#endif",
    "int q_cond(short, Trans *);", "int q_full(int);", "int q_len(int);",
    "int q_zero(int);", "int qrecv(int, int, int, int);", "int unsend(int);",
    "/* void *sbrk(int); */", "void Uerror(char *);",
    "void spin_assert(int, char *, int, int, Trans *);",
    "void c_chandump(int);", "void c_globals(void);",
    "void c_locals(int, int);", "void checkcycles(void);",
    "void crack(int, int, Trans *, short *);", "void d_sfh(const char *, int);",
    "void sfh(const char *, int);", "void d_hash(uchar *, int);",
    "void s_hash(uchar *, int);", "void r_hash(uchar *, int);",
    "void delq(int);", "void dot_crack(int, int, Trans *);",
    "void do_reach(void);", "void pan_exit(int);", "void exit(int);",
    "void hinit(void);", "void imed(Trans *, int, int, int);",
    "void new_state(void);", "void p_restor(int);", "void putpeg(int, int);",
    "void putrail(void);", "void q_restor(void);",
    "void retrans(int, int, int, short *, uchar *, uchar *);",
    "void settable(void);", "void setq_claim(int, int, char *, int, char *);",
    "void sv_restor(void);", "void sv_save(void);",
    "void tagtable(int, int, int, short *, uchar *);",
    "void do_dfs(int, int, int, short *, uchar *, uchar *);",
    "void uerror(char *);", "void unrecv(int, int, int, int, int);",
    "void usage(FILE *);", "void wrap_stats(void);",
    "#if defined(FULLSTACK) && defined(BITSTATE)",
    "	int  onstack_now(void);", "	void onstack_init(void);",
    "	void onstack_put(void);", "	void onstack_zap(void);", "#endif",
    "#ifndef XUSAFE", "	int q_S_check(int, int);",
    "	int q_R_check(int, int);", "	uchar q_claim[MAXQ+1];",
    "	char *q_name[MAXQ+1];", "	char *p_name[MAXPROC+1];", "#endif", 0,
};

static char* SvMap[] = {
    "void", "to_compile(void)", "{	char ctd[1024], carg[64];",
    "#ifdef BITSTATE", "	strcpy(ctd, \"-DBITSTATE \");", "#else",
    "	strcpy(ctd, \"\");", "#endif", "#ifdef NOVSZ",
    "	strcat(ctd, \"-DNOVSZ \");", "#endif", "#ifdef REVERSE",
    "	strcat(ctd, \"-DREVERSE \");", "#endif", "#ifdef T_REVERSE",
    "	strcat(ctd, \"-DT_REVERSE \");", "#endif", "#ifdef T_RAND",
    "	#if T_RAND>0", "	sprintf(carg, \"-DT_RAND=%%d \", T_RAND);",
    "	strcat(ctd, carg);", "	#else", "	strcat(ctd, \"-DT_RAND \");",
    "	#endif", "#endif", "#ifdef P_RAND", "	#if P_RAND>0",
    "	sprintf(carg, \"-DP_RAND=%%d \", P_RAND);", "	strcat(ctd, carg);",
    "	#else", "	strcat(ctd, \"-DP_RAND \");", "	#endif", "#endif",
    "#ifdef BCS", "	sprintf(carg, \"-DBCS=%%d \", BCS);",
    "	strcat(ctd, carg);", "#endif", "#ifdef BFS",
    "	strcat(ctd, \"-DBFS \");", "#endif", "#ifdef MEMLIM",
    "	sprintf(carg, \"-DMEMLIM=%%d \", MEMLIM);", "	strcat(ctd, carg);",
    "#else", "#ifdef MEMCNT", "	sprintf(carg, \"-DMEMCNT=%%d \", MEMCNT);",
    "	strcat(ctd, carg);", "#endif", "#endif", "#ifdef NOCLAIM",
    "	strcat(ctd, \"-DNOCLAIM \");", "#endif", "#ifdef SAFETY",
    "	strcat(ctd, \"-DSAFETY \");", "#else", "#ifdef NOFAIR",
    "	strcat(ctd, \"-DNOFAIR \");", "#else", "#ifdef NFAIR",
    "	if (NFAIR != 2)",
    "	{	sprintf(carg, \"-DNFAIR=%%d \", NFAIR);",
    "		strcat(ctd, carg);", "	}", "#endif", "#endif", "#endif",
    "#ifdef NOREDUCE", "	strcat(ctd, \"-DNOREDUCE \");", "#else",
    "#ifdef XUSAFE", "	strcat(ctd, \"-DXUSAFE \");", "#endif", "#endif",
    "#ifdef NP", "	strcat(ctd, \"-DNP \");", "#endif", "#ifdef PEG",
    "	strcat(ctd, \"-DPEG \");", "#endif", "#ifdef VAR_RANGES",
    "	strcat(ctd, \"-DVAR_RANGES \");", "#endif", "#ifdef HC0",
    "	strcat(ctd, \"-DHC0 \");", "#endif", "#ifdef HC1",
    "	strcat(ctd, \"-DHC1 \");", "#endif", "#ifdef HC2",
    "	strcat(ctd, \"-DHC2 \");", "#endif", "#ifdef HC3",
    "	strcat(ctd, \"-DHC3 \");", "#endif", "#ifdef HC4",
    "	strcat(ctd, \"-DHC4 \");", "#endif", "#ifdef CHECK",
    "	strcat(ctd, \"-DCHECK \");", "#endif", "#ifdef CTL",
    "	strcat(ctd, \"-DCTL \");", "#endif", "#ifdef TRIX",
    "	strcat(ctd, \"-DTRIX \");", "#endif", "#ifdef NIBIS",
    "	strcat(ctd, \"-DNIBIS \");", "#endif", "#ifdef NOBOUNDCHECK",
    "	strcat(ctd, \"-DNOBOUNDCHECK \");", "#endif", "#ifdef NOSTUTTER",
    "	strcat(ctd, \"-DNOSTUTTER \");", "#endif", "#ifdef REACH",
    "	strcat(ctd, \"-DREACH \");", "#endif", "#ifdef PRINTF",
    "	strcat(ctd, \"-DPRINTF \");", "#endif", "#ifdef OTIM",
    "	strcat(ctd, \"-DOTIM \");", "#endif", "#ifdef COLLAPSE",
    "	strcat(ctd, \"-DCOLLAPSE \");", "#endif", "#ifdef MA",
    "	sprintf(carg, \"-DMA=%%d \", MA);", "	strcat(ctd, carg);", "#endif",
    "#ifdef SVDUMP", "	strcat(ctd, \"-DSVDUMP \");", "#endif",
    "#if defined(VECTORSZ) && !defined(TRIX)", "	if (VECTORSZ != 1024)",
    "	{	sprintf(carg, \"-DVECTORSZ=%%d \", VECTORSZ);",
    "		strcat(ctd, carg);", "	}", "#endif", "#ifdef VERBOSE",
    "	strcat(ctd, \"-DVERBOSE \");", "#endif", "#ifdef CHECK",
    "	strcat(ctd, \"-DCHECK \");", "#endif", "#ifdef SDUMP",
    "	strcat(ctd, \"-DSDUMP \");", "#endif", "#if NCORE>1",
    "	sprintf(carg, \"-DNCORE=%%d \", NCORE);", "	strcat(ctd, carg);",
    "#endif", "#ifdef SFH", "	sprintf(carg, \"-DSFH \");",
    "	strcat(ctd, carg);", "#endif", "#ifdef VMAX", "	if (VMAX != 256)",
    "	{	sprintf(carg, \"-DVMAX=%%d \", VMAX);",
    "		strcat(ctd, carg);", "	}", "#endif", "#ifdef PMAX",
    "	if (PMAX != 16)", "	{	sprintf(carg, \"-DPMAX=%%d \", PMAX);",
    "		strcat(ctd, carg);", "	}", "#endif", "#ifdef QMAX",
    "	if (QMAX != 16)", "	{	sprintf(carg, \"-DQMAX=%%d \", QMAX);",
    "		strcat(ctd, carg);", "	}", "#endif", "#ifdef SET_WQ_SIZE",
    "	sprintf(carg, \"-DSET_WQ_SIZE=%%d \", SET_WQ_SIZE);",
    "	strcat(ctd, carg);", "#endif",
    "	printf(\"Compiled as: cc -o pan %%span.c\\n\", ctd);", "}", 0,
};
