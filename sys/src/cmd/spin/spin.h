/***** spin: spin.h *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

#ifndef SEEN_SPIN_H
#define SEEN_SPIN_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct Lextok {
	unsigned short	ntyp;	/* node type */
	short	ismtyp;		/* CONST derived from MTYP */
	int	val;		/* value attribute */
	int	ln;		/* line number */
	int	indstep;	/* part of d_step sequence */
	struct Symbol	*fn;	/* file name */
	struct Symbol	*sym;	/* symbol reference */
        struct Sequence *sq;	/* sequence */
        struct SeqList	*sl;	/* sequence list */
	struct Lextok	*lft, *rgt; /* children in parse tree */
} Lextok;

typedef struct Slicer {
	Lextok	*n;		/* global var, usable as slice criterion */
	short	code;		/* type of use: DEREF_USE or normal USE */
	short	used;		/* set when handled */
	struct Slicer *nxt;	/* linked list */
} Slicer;

typedef struct Access {
	struct Symbol	*who;	/* proctype name of accessor */
	struct Symbol	*what;	/* proctype name of accessed */
	int	cnt, typ;	/* parameter nr and, e.g., 's' or 'r' */
	struct Access	*lnk;	/* linked list */
} Access;

typedef struct Symbol {
	char	*name;
	int	Nid;		/* unique number for the name */
	unsigned short	type;	/* bit,short,.., chan,struct  */
	unsigned char	hidden; /* bit-flags:
				   1=hide, 2=show,
				   4=bit-equiv,   8=byte-equiv,
				  16=formal par, 32=inline par,
				  64=treat as if local; 128=read at least once
				 */
	unsigned char	colnr;	/* for use with xspin during simulation */
	int	nbits;		/* optional width specifier */
	int	nel;		/* 1 if scalar, >1 if array   */
	int	setat;		/* last depth value changed   */
	int	*val;		/* runtime value(s), initl 0  */
	Lextok	**Sval;	/* values for structures */

	int	xu;		/* exclusive r or w by 1 pid  */
	struct Symbol	*xup[2];  /* xr or xs proctype  */
	struct Access	*access;/* e.g., senders and receives of chan */
	Lextok	*ini;	/* initial value, or chan-def */
	Lextok	*Slst;	/* template for structure if struct */
	struct Symbol	*Snm;	/* name of the defining struct */
	struct Symbol	*owner;	/* set for names of subfields in typedefs */
	struct Symbol	*context; /* 0 if global, or procname */
	struct Symbol	*next;	/* linked list */
} Symbol;

typedef struct Ordered {	/* links all names in Symbol table */ 
	struct Symbol	*entry;
	struct Ordered	*next;
} Ordered;

typedef struct Queue {
	short	qid;		/* runtime q index */
	int	qlen;		/* nr messages stored */
	int	nslots, nflds;	/* capacity, flds/slot */
	int	setat;		/* last depth value changed */
	int	*fld_width;	/* type of each field */
	int	*contents;	/* the values stored */
	int	*stepnr;	/* depth when each msg was sent */
	struct Queue	*nxt;	/* linked list */
} Queue;

typedef struct FSM_state {	/* used in pangen5.c - dataflow */
	int from;		/* state number */
	int seen;		/* used for dfs */
	int in;			/* nr of incoming edges */
	int cr;			/* has reachable 1-relevant successor */
	int scratch;
	unsigned long *dom, *mod; /* to mark dominant nodes */
	struct FSM_trans *t;	/* outgoing edges */
	struct FSM_trans *p;	/* incoming edges, predecessors */
	struct FSM_state *nxt;	/* linked list of all states */
} FSM_state;

typedef struct FSM_trans {	/* used in pangen5.c - dataflow */
	int to;
	short	relevant;	/* when sliced */
	short	round;		/* ditto: iteration when marked */
	struct FSM_use *Val[2];	/* 0=reads, 1=writes */
	struct Element *step;
	struct FSM_trans *nxt;
} FSM_trans;

typedef struct FSM_use {	/* used in pangen5.c - dataflow */
	Lextok *n;
	Symbol *var;
	int special;
	struct FSM_use *nxt;
} FSM_use;

typedef struct Element {
	Lextok	*n;		/* defines the type & contents */
	int	Seqno;		/* identifies this el within system */
	int	seqno;		/* identifies this el within a proc */
	int	merge;		/* set by -O if step can be merged */
	int	merge_start;
	int	merge_single;
	short	merge_in;	/* nr of incoming edges */
	short	merge_mark;	/* state was generated in merge sequence */
	unsigned char	status;	/* used by analyzer generator  */
	struct FSM_use	*dead;	/* optional dead variable list */
	struct SeqList	*sub;	/* subsequences, for compounds */
	struct SeqList	*esc;	/* zero or more escape sequences */
	struct Element	*Nxt;	/* linked list - for global lookup */
	struct Element	*nxt;	/* linked list - program structure */
} Element;

typedef struct Sequence {
	Element	*frst;
	Element	*last;		/* links onto continuations */
	Element *extent;	/* last element in original */
	int	maxel;		/* 1+largest id in sequence */
} Sequence;

typedef struct SeqList {
	Sequence	*this;	/* one sequence */
	struct SeqList	*nxt;	/* linked list  */
} SeqList;

typedef struct Label {
	Symbol	*s;
	Symbol	*c;
	Element	*e;
	int	visible;	/* label referenced in claim (slice relevant) */
	struct Label	*nxt;
} Label;

typedef struct Lbreak {
	Symbol	*l;
	struct Lbreak	*nxt;
} Lbreak;

typedef struct RunList {
	Symbol	*n;		/* name            */
	int	tn;		/* ordinal of type */
	int	pid;		/* process id      */
	int	priority;	/* for simulations only */
	Element	*pc;		/* current stmnt   */
	Sequence *ps;		/* used by analyzer generator */
	Lextok	*prov;		/* provided clause */
	Symbol	*symtab;	/* local variables */
	struct RunList	*nxt;	/* linked list */
} RunList;

typedef struct ProcList {
	Symbol	*n;		/* name       */
	Lextok	*p;		/* parameters */
	Sequence *s;		/* body       */
	Lextok	*prov;		/* provided clause */
	short	tn;		/* ordinal number */
	short	det;		/* deterministic */
	struct ProcList	*nxt;	/* linked list */
} ProcList;

typedef	Lextok *Lexptr;

#define YYSTYPE	Lexptr

#define ZN	(Lextok *)0
#define ZS	(Symbol *)0
#define ZE	(Element *)0

#define DONE	  1     	/* status bits of elements */
#define ATOM	  2     	/* part of an atomic chain */
#define L_ATOM	  4     	/* last element in a chain */
#define I_GLOB    8		/* inherited global ref    */
#define DONE2	 16		/* used in putcode and main*/
#define D_ATOM	 32		/* deterministic atomic    */
#define ENDSTATE 64		/* normal endstate         */
#define CHECK2	128

#define Nhash	255    		/* slots in symbol hash-table */

#define XR	  	1	/* non-shared receive-only */
#define XS	  	2	/* non-shared send-only    */
#define XX	  	4	/* overrides XR or XS tag  */

#define CODE_FRAG	2	/* auto-numbered code-fragment */
#define CODE_DECL	4	/* auto-numbered c_decl */
#define PREDEF	  	3	/* predefined name: _p, _last */

#define UNSIGNED  5		/* val defines width in bits */
#define BIT	  1		/* also equal to width in bits */
#define BYTE	  8		/* ditto */
#define SHORT	 16		/* ditto */
#define INT	 32		/* ditto */
#define	CHAN	 64		/* not */
#define STRUCT	128		/* user defined structure name */

#define SOMETHINGBIG	65536
#define RATHERSMALL	512

#ifndef max
#define max(a,b) (((a)<(b)) ? (b) : (a))
#endif

enum	{ INIV, PUTV, LOGV };	/* for pangen[14].c */

/***** prototype definitions *****/
Element	*eval_sub(Element *);
Element	*get_lab(Lextok *, int);
Element	*huntele(Element *, int, int);
Element	*huntstart(Element *);
Element	*target(Element *);

Lextok	*do_unless(Lextok *, Lextok *);
Lextok	*expand(Lextok *, int);
Lextok	*getuname(Symbol *);
Lextok	*mk_explicit(Lextok *, int, int);
Lextok	*nn(Lextok *, int, Lextok *, Lextok *);
Lextok	*rem_lab(Symbol *, Lextok *, Symbol *);
Lextok	*rem_var(Symbol *, Lextok *, Symbol *, Lextok *);
Lextok	*tail_add(Lextok *, Lextok *);

ProcList *ready(Symbol *, Lextok *, Sequence *, int, Lextok *);

SeqList	*seqlist(Sequence *, SeqList *);
Sequence *close_seq(int);

Symbol	*break_dest(void);
Symbol	*findloc(Symbol *);
Symbol	*has_lab(Element *, int);
Symbol	*lookup(char *);
Symbol	*prep_inline(Symbol *, Lextok *);

char	*emalloc(int);
long	Rand(void);

int	any_oper(Lextok *, int);
int	any_undo(Lextok *);
int	c_add_sv(FILE *);
int	cast_val(int, int, int);
int	checkvar(Symbol *, int);
int	Cnt_flds(Lextok *);
int	cnt_mpars(Lextok *);
int	complete_rendez(void);
int	enable(Lextok *);
int	Enabled0(Element *);
int	eval(Lextok *);
int	find_lab(Symbol *, Symbol *, int);
int	find_maxel(Symbol *);
int	full_name(FILE *, Lextok *, Symbol *, int);
int	getlocal(Lextok *);
int	getval(Lextok *);
int	glob_inline(char *);
int	has_typ(Lextok *, int);
int	in_bound(Symbol *, int);
int	interprint(FILE *, Lextok *);
int	printm(FILE *, Lextok *);
int	ismtype(char *);
int	isproctype(char *);
int	isutype(char *);
int	Lval_struct(Lextok *, Symbol *, int, int);
int	main(int, char **);
int	pc_value(Lextok *);
int	proper_enabler(Lextok *);
int	putcode(FILE *, Sequence *, Element *, int, int, int);
int	q_is_sync(Lextok *);
int	qlen(Lextok *);
int	qfull(Lextok *);
int	qmake(Symbol *);
int	qrecv(Lextok *, int);
int	qsend(Lextok *);
int	remotelab(Lextok *);
int	remotevar(Lextok *);
int	Rval_struct(Lextok *, Symbol *, int);
int	setlocal(Lextok *, int);
int	setval(Lextok *, int);
int	sputtype(char *, int);
int	Sym_typ(Lextok *);
int	tl_main(int, char *[]);
int	Width_set(int *, int, Lextok *);
int	yyparse(void);
int	yywrap(void);
int	yylex(void);

void	AST_track(Lextok *, int);
void	add_seq(Lextok *);
void	alldone(int);
void	announce(char *);
void	c_state(Symbol *, Symbol *, Symbol *);
void	c_add_def(FILE *);
void	c_add_loc(FILE *, char *);
void	c_add_locinit(FILE *, int, char *);
void	c_add_use(FILE *);
void	c_chandump(FILE *);
void	c_preview(void);
void	c_struct(FILE *, char *, Symbol *);
void	c_track(Symbol *, Symbol *, Symbol *);
void	c_var(FILE *, char *, Symbol *);
void	c_wrapper(FILE *);
void	chanaccess(void);
void	check_param_count(int, Lextok *);
void	checkrun(Symbol *, int);
void	comment(FILE *, Lextok *, int);
void	cross_dsteps(Lextok *, Lextok *);
void	doq(Symbol *, int, RunList *);
void	dotag(FILE *, char *);
void	do_locinits(FILE *);
void	do_var(FILE *, int, char *, Symbol *, char *, char *, char *);
void	dump_struct(Symbol *, char *, RunList *);
void	dumpclaims(FILE *, int, char *);
void	dumpglobals(void);
void	dumplabels(void);
void	dumplocal(RunList *);
void	dumpsrc(int, int);
void	fatal(char *, char *);
void	fix_dest(Symbol *, Symbol *);
void	genaddproc(void);
void	genaddqueue(void);
void	gencodetable(FILE *);
void	genheader(void);
void	genother(void);
void	gensrc(void);
void	gensvmap(void);
void	genunio(void);
void	ini_struct(Symbol *);
void	loose_ends(void);
void	make_atomic(Sequence *, int);
void	match_trail(void);
void	no_side_effects(char *);
void	nochan_manip(Lextok *, Lextok *, int);
void	non_fatal(char *, char *);
void	ntimes(FILE *, int, int, char *c[]);
void	open_seq(int);
void	p_talk(Element *, int);
void	pickup_inline(Symbol *, Lextok *);
void	plunk_c_decls(FILE *);
void	plunk_c_fcts(FILE *);
void	plunk_expr(FILE *, char *);
void	plunk_inline(FILE *, char *, int);
void	prehint(Symbol *);
void	preruse(FILE *, Lextok *);
void	prune_opts(Lextok *);
void	pstext(int, char *);
void	pushbreak(void);
void	putname(FILE *, char *, Lextok *, int, char *);
void	putremote(FILE *, Lextok *, int);
void	putskip(int);
void	putsrc(Element *);
void	putstmnt(FILE *, Lextok *, int);
void	putunames(FILE *);
void	rem_Seq(void);
void	runnable(ProcList *, int, int);
void	sched(void);
void	setaccess(Symbol *, Symbol *, int, int);
void	set_lab(Symbol *, Element *);
void	setmtype(Lextok *);
void	setpname(Lextok *);
void	setptype(Lextok *, int, Lextok *);
void	setuname(Lextok *);
void	setutype(Lextok *, Symbol *, Lextok *);
void	setxus(Lextok *, int);
void	Srand(unsigned);
void	start_claim(int);
void	struct_name(Lextok *, Symbol *, int, char *);
void	symdump(void);
void	symvar(Symbol *);
void	trackchanuse(Lextok *, Lextok *, int);
void	trackvar(Lextok *, Lextok *);
void	trackrun(Lextok *);
void	trapwonly(Lextok *, char *);	/* spin.y and main.c */
void	typ2c(Symbol *);
void	typ_ck(int, int, char *);
void	undostmnt(Lextok *, int);
void	unrem_Seq(void);
void	unskip(int);
void	varcheck(Element *, Element *);
void	whoruns(int);
void	wrapup(int);
void	yyerror(char *, ...);
#endif
