/***** spin: spin.h *****/

/* Copyright (c) 1991,1995 by AT&T Corporation.  All Rights Reserved.     */
/* This software is for educational purposes only.                        */
/* Permission is given to distribute this code provided that this intro-  */
/* ductory message is not removed and no monies are exchanged.            */
/* No guarantee is expressed or implied by the distribution of this code. */
/* Software written by Gerard J. Holzmann as part of the book:            */
/* `Design and Validation of Computer Protocols,' ISBN 0-13-539925-4,     */
/* Prentice Hall, Englewood Cliffs, NJ, 07632.                            */
/* Send bug-reports and/or questions to: gerard@research.att.com          */

#include <stdio.h>
#include <string.h>
#define Version	"Spin Version 2.2.2 -- 24 February 1995"

typedef struct Lextok {
	unsigned short	ntyp;	/* node type */
	short	ismtyp;		/* CONST derived from MTYP */
	int	val;		/* value attribute */
	int	ln;		/* line number */
	int	indstep;	/* part of d_step sequence */
	struct Symbol	*fn;	/* file name   */
	struct Symbol	*sym;	/* fields 1,4-7 used as union */
        struct Sequence *sq;	/* sequence    */
        struct SeqList	*sl;	/* sequence list */
	struct Lextok	*lft, *rgt; /* children in parse tree */
} Lextok;

typedef struct Symbol {
	char	*name;
	int	Nid;		/* unique number for the name */
	short	type;		/* bit,short,.., chan,struct  */
	short	hidden;		/* 1 if outside statevector   */
	int	nel;		/* 1 if scalar, >1 if array   */
	int	setat;		/* last depth value changed   */
	int	*val;		/* runtime value(s), initl 0  */
	struct Lextok	**Sval;	/* values for structures */

	int	xu;		/* exclusive r or w by 1 pid  */
	struct Symbol	*xup[2];  /* xr or xs proctype  */

	struct Lextok	*ini;	/* initial value, or chan-def */
	struct Lextok	*Slst;	/* template for structure if struct */
	struct Symbol	*Snm;	/* name of the defining struct */
	struct Symbol	*owner;	/* set for names of subfields in typedefs */
	struct Symbol	*context; /* 0 if global, or procname */
	struct Symbol	*next;	/* linked list */
} Symbol;

typedef struct Queue {
	short	qid;		/* runtime q index      */
	short	qlen;		/* nr messages stored   */
	short	nslots, nflds;	/* capacity, flds/slot  */
	int	setat;		/* last depth value changed   */
	int	*fld_width;	/* type of each field   */
	int	*contents;	/* the actual buffer    */
	struct Queue	*nxt;	/* linked list */
} Queue;

typedef struct Element {
	Lextok	*n;		/* defines the type & contents */
	int	Seqno;		/* identifies this el within system */
	int	seqno;		/* identifies this el within a proc */
	unsigned char	status;	/* used by analyzer generator  */
	struct SeqList	*sub;	/* subsequences, for compounds */
	struct SeqList	*esc;	/* zero or more escape sequences */
	struct Element	*Nxt;	/* linked list - for global lookup */
	struct Element	*nxt;	/* linked list - program structure */
} Element;

typedef struct Sequence {
	Element	*frst;
	Element	*last;		/* links onto continuations */
	Element *extent;	/* last element in original */
	int	maxel;	/* 1+largest id in the sequence */
} Sequence;

typedef struct SeqList {
	Sequence	*this;	/* one sequence */
	struct SeqList	*nxt;	/* linked list  */
} SeqList;

typedef struct Label {
	Symbol	*s;
	Symbol	*c;
	Element	*e;
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
	Element	*pc;		/* current stmnt   */
	Sequence *ps;		/* used by analyzer generator */
	Symbol	*symtab;	/* local variables */
	struct RunList	*nxt;	/* linked list */
} RunList;

typedef struct ProcList {
	Symbol	*n;		/* name       */
	Lextok	*p;		/* parameters */
	Sequence *s;		/* body       */
	int	tn;		/* ordinal number */
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
#define CHECK1	 64
#define CHECK2	128

#define Nhash	255    		/* slots in symbol hash-table */

#define XR	  1		/* non-shared receive-only */
#define XS	  2		/* non-shared send-only    */
#define XX	  4		/* overrides XR or XS tag  */

#define PREDEF	  3		/* predefined names: _p, _last */
#define BIT	  1		/* also equal to width in bits */
#define BYTE	  8		/* ditto */
#define SHORT	 16		/* ditto */
#define INT	 32		/* ditto */
#define	CHAN	 64		/* not */
#define STRUCT	128		/* user defined structure name */

#define max(a,b) (((a)<(b)) ? (b) : (a))

/***** ANSI C - prototype definitions *****/
Element *colons(Lextok *);
Element *eval_sub(Element *);
Element *get_lab(Lextok *, int);
Element *huntele(Element *, int);
Element *huntstart(Element *);
Element *if_seq(Lextok *);
Element *new_el(Lextok *);
Element *target(Element *);
Element *unless_seq(Lextok *);

Lextok *cpnn(Lextok *, int, int, int);
Lextok *do_unless(Lextok *, Lextok *);
Lextok *expand(Lextok *, int);
Lextok *getuname(Symbol *);
Lextok *mk_explicit(Lextok *, int, int);
Lextok *nn(Lextok *, int, Lextok *, Lextok *);
Lextok *rem_lab(Symbol *, Lextok *, Symbol *);
Lextok *tail_add(Lextok *, Lextok *);

ProcList *ready(Symbol *, Lextok *, Sequence *);

SeqList *seqlist(Sequence *, SeqList *);
Sequence *close_seq(int);

Symbol *break_dest(void);
Symbol *findloc(Symbol *);
Symbol *has_lab(Element *);
Symbol *lookup(char *);

char *emalloc(int);
long time(long *);

int a_rcv(Queue *, Lextok *, int);
int a_snd(Queue *, Lextok *);
int any_oper(Lextok *, int);
int any_undo(Lextok *);
int atoi(char *);
int blog(int);
int cast_val(int, int);
int check_name(char *);
int checkvar(Symbol *, int);
int Cnt_flds(Lextok *);
int cnt_mpars(Lextok *);
int complete_rendez(void);
int dolocal(int, int, char *);
int enable(Symbol *, Lextok *);
int Enabled0(Element *);
int Enabled1(Lextok *);
int eval(Lextok *);
int eval_sync(Element *);
int fill_struct(Symbol *, int, int, char *, Symbol **);
int find_lab(Symbol *, Symbol *);
int find_maxel(Symbol *);
int fproc(char *);
int getglobal(Lextok *);
int getlocal(Lextok *);
int getval(Lextok *);
int getweight(Lextok *);
int has_global(Lextok *);
int has_typ(Lextok *, int);
int hash(char *);
int interprint(Lextok *);
int ismtype(char *);
int isproctype(char *);
int isutype(char *);
int Lval_struct(Lextok *, Symbol *, int, int);
int newer(char *, char *);
int pc_value(Lextok *);
int putcode(FILE *, Sequence *, Element *, int);
int puttype(int);
int q_is_sync(Lextok *);
int qlen(Lextok *);
int qfull(Lextok *);
int qmake(Symbol *);
int qrecv(Lextok *, int);
int qsend(Lextok *);
int Rval_struct(Lextok *, Symbol *, int);
int remotelab(Lextok *);
int remotevar(Lextok *);
int sa_snd(Queue *, Lextok *);
int s_snd(Queue *, Lextok *);
int scan_seq(Sequence *);
int setglobal(Lextok *, int);
int setlocal(Lextok *, int);
int setval(Lextok *, int);
int sputtype(char *, int);
int Sym_typ(Lextok *);
int symbolic(FILE *, Lextok *);
int Width_set(int *, int, Lextok *);
int yyparse(void);
int yywrap(void);
int yylex(void);
long Rand(void);

void add_el(Element *, Sequence *);
void add_seq(Lextok *);
void addsymbol(RunList *, Symbol *);
void attach_escape(Sequence *, Sequence *);
void check_proc(Lextok *, int);
void comment(FILE *, Lextok *, int);
void comwork(FILE *, Lextok *, int);
void cross_dsteps(Lextok *, Lextok *);
void do_init(Symbol *);
void do_var(int, char *, Symbol *);
void doglobal(int);
void dohidden(void);
void doq(Symbol *, int, RunList *);
void dump_struct(Symbol *, char *, RunList *);
void dumpclaims(FILE *, int, char *);
void dumpglobals(void);
void dumplabels(void);
void dumplocal(RunList *);
void dumpskip(int, int);
void dumpsrc(int, int);
void end_labs(Symbol *, int);
void exit(int);
void explain(int);
void fatal(char *, char *);
void full_name(FILE *, Lextok *, Symbol *, int);
void fix_dest(Symbol *, Symbol *);
void genaddproc(void);
void genaddqueue(void);
void genconditionals(void);
void genheader(void);
void genother(void);
void gensrc(void);
void genunio(void);
void ini_struct(Symbol *);
void lost_trail(void);
void main(int, char **);
void make_atomic(Sequence *, int);
void mark_seq(Sequence *);
void match_trail(void);
void mov_lab(Symbol *, Element *, Element *);
void ncases(FILE *, int, int, int, char **);
void non_fatal(char *, char *);
void ntimes(FILE *, int, int, char *c[]);
void open_seq(int);
void p_talk(Element *, int);
void patch_atomic(Sequence *);
void pushbreak(void);
void put_pinit(Element *, Symbol *, Lextok *, int);
void put_ptype(char *, int, int, int);
void put_seq(Sequence *, int, int);
void putCode(FILE *, Element *, Element *, Element *, int);
void putname(FILE *, char *, Lextok *, int, char *);
void putnr(int);
void putproc(Symbol *, Sequence *, int, int, int);
void putremote(FILE *, Lextok *, int);
void putskip(int);
void putsrc(int, int);
void putstmnt(FILE *, Lextok *, int);
void putunames(FILE *);
void rem_Seq(void);
void runnable(ProcList *);
void sched(void);
void set_lab(Symbol *, Element *);
void setmtype(Lextok *);
void setparams(RunList *, ProcList *, Lextok *);
void setpname(Lextok *);
void setptype(Lextok *, int, Lextok *);	/* predefined  types */
void setuname(Lextok *);	/* userdefined types */
void setutype(Lextok *, Symbol *, Lextok *);
void setxus(Lextok *, int);
void sr_mesg(int, int);
void sr_talk(Lextok *, int, char *, char *, int, Queue *);
void Srand(unsigned);
void start_claim(int);
void symdump(void);
void formdump(void);
void struct_name(Lextok *, Symbol *, int);
void symvar(Symbol *);
void talk(RunList *);
void typ_ck(int, int, char *);
void typ2c(Symbol *);
void undostmnt(Lextok *, int);
void unrem_Seq(void);
void unskip(int);
void varcheck(Element *, Element *);
void walk_atomic(Element *, Element *, int);
void whichproc(int);
void whoruns(int);
void wrapup(int);
void yyerror(char *, ...);
