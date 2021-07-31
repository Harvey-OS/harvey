/***** spin: main.c *****/

/* Copyright (c) 1989-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

#include <stdlib.h>
#include "spin.h"
#include "version.h"
#include <signal.h>
/* #include <malloc.h> */
#include <time.h>
#ifdef PC
#include <io.h>
extern int unlink(const char *);
#else
#include <unistd.h>
#endif
#include "y.tab.h"

extern int	DstepStart, lineno, tl_terse;
extern FILE	*yyin, *yyout, *tl_out;
extern Symbol	*context;
extern char	*claimproc;
extern void	repro_src(void);
extern void	qhide(int);

Symbol	*Fname, *oFname;

int	Etimeouts;	/* nr timeouts in program */
int	Ntimeouts;	/* nr timeouts in never claim */
int	analyze, columns, dumptab, has_remote, has_remvar;
int	interactive, jumpsteps, m_loss, nr_errs, cutoff;
int	s_trail, ntrail, verbose, xspin, notabs, rvopt;
int	no_print, no_wrapup, Caccess, limited_vis, like_java;
int	separate;	/* separate compilation */
int	export_ast;	/* pangen5.c */
int	inlineonly;	/* show inlined code */
int	seedy;		/* be verbose about chosen seed */

int	dataflow = 1, merger = 1, deadvar = 1, ccache = 1;

static int preprocessonly, SeedUsed;

#if 0
meaning of flags on verbose:
	1	-g global variable values
	2	-l local variable values
	4	-p all process actions
	8	-r receives
	16	-s sends
	32	-v verbose
	64	-w very verbose
#endif

static char	Operator[] = "operator: ";
static char	Keyword[]  = "keyword: ";
static char	Function[] = "function-name: ";
static char	**add_ltl  = (char **)0;
static char	**ltl_file = (char **)0;
static char	**nvr_file = (char **)0;
static char	*PreArg[64];
static int	PreCnt = 0;
static char	out1[64];
static void	explain(int);

#ifndef CPP
		/* OS2:		"spin -Picc -E/Pd+ -E/Q+"    */
		/* Visual C++:	"spin -PCL  -E/E             */
#ifdef PC
#define CPP	"gcc -E -x c"	/* most systems have gcc anyway */
				/* else use "cpp" */
#else
#ifdef SOLARIS
#define CPP	"/usr/ccs/lib/cpp"
#else
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define CPP	"cpp"
#else
#define CPP	"/bin/cpp"	/* classic Unix systems */
#endif
#endif
#endif

#endif
static char	*PreProc = CPP;
extern int	depth; /* at least some steps were made */

void
alldone(int estatus)
{
	if (preprocessonly == 0
	&&  strlen(out1) > 0)
		(void) unlink((const char *)out1);

	if (seedy && !analyze && !export_ast
	&& !s_trail && !preprocessonly && depth > 0)
		printf("seed used: %d\n", SeedUsed);

	if (xspin && (analyze || s_trail))
	{	if (estatus)
			printf("spin: %d error(s) - aborting\n",
			estatus);
		else
			printf("Exit-Status 0\n");
	}
	exit(estatus);
}

void
preprocess(char *a, char *b, int a_tmp)
{	char precmd[512], cmd[1024]; int i;
#ifdef PC
	extern int try_zpp(char *, char *);
	if (PreCnt == 0 && try_zpp(a, b)) goto out;
#endif
	strcpy(precmd, PreProc);
	for (i = 1; i <= PreCnt; i++)
	{	strcat(precmd, " ");
		strcat(precmd, PreArg[i]);
	}
	sprintf(cmd, "%s %s > %s", precmd, a, b);
	if (system((const char *)cmd))
	{	(void) unlink((const char *) b);
		if (a_tmp) (void) unlink((const char *) a);
		fprintf(stdout, "spin: preprocessing failed\n");	/* 4.1.2 was stderr */
		alldone(1); /* no return, error exit */
	}
#ifdef PC
out:
#endif
	if (a_tmp) (void) unlink((const char *) a);
}

FILE *
cpyfile(char *src, char *tgt)
{	FILE *inp, *out;
	char buf[1024];

	inp = fopen(src, "r");
	out = fopen(tgt, "w");
	if (!inp || !out)
	{	printf("spin: cannot cp %s to %s\n", src, tgt);
		alldone(1);
	}
	while (fgets(buf, 1024, inp))
		fprintf(out, "%s", buf);
	fclose(inp);
	return out;
}

void
usage(void)
{
	printf("use: spin [-option] ... [-option] file\n");
	printf("\tNote: file must always be the last argument\n");
	printf("\t-A apply slicing algorithm\n");
	printf("\t-a generate a verifier in pan.c\n");
	printf("\t-B no final state details in simulations\n");
	printf("\t-b don't execute printfs in simulation\n");
	printf("\t-C print channel access info (combine with -g etc.)\n");
	printf("\t-c columnated -s -r simulation output\n");
	printf("\t-d produce symbol-table information\n");
	printf("\t-Dyyy pass -Dyyy to the preprocessor\n");
	printf("\t-Eyyy pass yyy to the preprocessor\n");
	printf("\t-f \"..formula..\"  translate LTL ");
	printf("into never claim\n");
	printf("\t-F file  like -f, but with the LTL ");
	printf("formula stored in a 1-line file\n");
	printf("\t-g print all global variables\n");
	printf("\t-h  at end of run, print value of seed for random nr generator used\n");
	printf("\t-i interactive (random simulation)\n");
	printf("\t-I show result of inlining and preprocessing\n");
	printf("\t-J reverse eval order of nested unlesses\n");
	printf("\t-jN skip the first N steps ");
	printf("in simulation trail\n");
	printf("\t-l print all local variables\n");
	printf("\t-M print msc-flow in Postscript\n");
	printf("\t-m lose msgs sent to full queues\n");
	printf("\t-N file use never claim stored in file\n");
	printf("\t-nN seed for random nr generator\n");
	printf("\t-o1 turn off dataflow-optimizations in verifier\n");
	printf("\t-o2 don't hide write-only variables in verifier\n");
	printf("\t-o3 turn off statement merging in verifier\n");
	printf("\t-Pxxx use xxx for preprocessing\n");
	printf("\t-p print all statements\n");
	printf("\t-qN suppress io for queue N in printouts\n");
	printf("\t-r print receive events\n");
	printf("\t-S1 and -S2 separate pan source for claim and model\n");
	printf("\t-s print send events\n");
	printf("\t-T do not indent printf output\n");
	printf("\t-t[N] follow [Nth] simulation trail\n");
	printf("\t-Uyyy pass -Uyyy to the preprocessor\n");
	printf("\t-uN stop a simulation run after N steps\n");
	printf("\t-v verbose, more warnings\n");
	printf("\t-w very verbose (when combined with -l or -g)\n");
	printf("\t-[XYZ] reserved for use by xspin interface\n");
	printf("\t-V print version number and exit\n");
	alldone(1);
}

void
optimizations(char nr)
{
	switch (nr) {
	case '1':
		dataflow = 1 - dataflow; /* dataflow */
		if (verbose&32)
		printf("spin: dataflow optimizations turned %s\n",
			dataflow?"on":"off");
		break;
	case '2':
		/* dead variable elimination */
		deadvar = 1 - deadvar;
		if (verbose&32)
		printf("spin: dead variable elimination turned %s\n",
			deadvar?"on":"off");
		break;
	case '3':
		/* statement merging */
		merger = 1 - merger;
		if (verbose&32)
		printf("spin: statement merging turned %s\n",
			merger?"on":"off");
		break;

	case '4':
		/* rv optimization */
		rvopt = 1 - rvopt;
		if (verbose&32)
		printf("spin: rendezvous optimization turned %s\n",
			rvopt?"on":"off");
		break;
	case '5':
		/* case caching */
		ccache = 1 - ccache;
		if (verbose&32)
		printf("spin: case caching turned %s\n",
			ccache?"on":"off");
		break;
	default:
		printf("spin: bad or missing parameter on -o\n");
		usage();
		break;
	}
}

#if 0
static int
Rename(const char *old, char *new)
{	FILE *fo, *fn;
	char buf[1024];

	if ((fo = fopen(old, "r")) == NULL)
	{	printf("spin: cannot open %s\n", old);
		return 1;
	}
	if ((fn = fopen(new, "w")) == NULL)
	{	printf("spin: cannot create %s\n", new);
		fclose(fo);
		return 2;
	}
	while (fgets(buf, 1024, fo))
		fputs(buf, fn);

	fclose(fo);
	fclose(fn);

	return 0;	/* success */
}
#endif

int
main(int argc, char *argv[])
{	Symbol *s;
	int T = (int) time((time_t *)0);
	int usedopts = 0;
	extern void ana_src(int, int);

	yyin  = stdin;
	yyout = stdout;
	tl_out = stdout;

	/* unused flags: e, w, x, y, z, A, G, I, L, O, Q, R, S, T, W */
	while (argc > 1 && argv[1][0] == '-')
	{	switch (argv[1][1]) {

		/* generate code for separate compilation: S1 or S2 */
		case 'S': separate = atoi(&argv[1][2]);
			  /* fall through */
		case 'a': analyze  = 1; break;

		case 'A': export_ast = 1; break;
		case 'B': no_wrapup = 1; break;
		case 'b': no_print = 1; break;
		case 'C': Caccess = 1; break;
		case 'c': columns = 1; break;
		case 'D': PreArg[++PreCnt] = (char *) &argv[1][0];
			  break;	/* define */
		case 'd': dumptab =  1; break;
		case 'E': PreArg[++PreCnt] = (char *) &argv[1][2];
			  break;
		case 'F': ltl_file = (char **) (argv+2);
			  argc--; argv++; break;
		case 'f': add_ltl = (char **) argv;
			  argc--; argv++; break;
		case 'g': verbose +=  1; break;
		case 'h': seedy = 1; break;
		case 'i': interactive = 1; break;
		case 'I': inlineonly = 1; break;
		case 'J': like_java = 1; break;
		case 'j': jumpsteps = atoi(&argv[1][2]); break;
		case 'l': verbose +=  2; break;
		case 'M': columns = 2; break;
		case 'm': m_loss   =  1; break;
		case 'N': nvr_file = (char **) (argv+2);
			  argc--; argv++; break;
		case 'n': T = atoi(&argv[1][2]); tl_terse = 1; break;
		case 'o': optimizations(argv[1][2]);
			  usedopts = 1; break;
		case 'P': PreProc = (char *) &argv[1][2]; break;
		case 'p': verbose +=  4; break;
		case 'q': if (isdigit(argv[1][2]))
				qhide(atoi(&argv[1][2]));
			  break;
		case 'r': verbose +=  8; break;
		case 's': verbose += 16; break;
		case 'T': notabs = 1; break;
		case 't': s_trail  =  1;
			  if (isdigit(argv[1][2]))
				ntrail = atoi(&argv[1][2]);
			  break;
		case 'U': PreArg[++PreCnt] = (char *) &argv[1][0];
			  break;	/* undefine */
		case 'u': cutoff = atoi(&argv[1][2]); break;	/* new 3.4.14 */
		case 'v': verbose += 32; break;
		case 'V': printf("%s\n", Version);
			  alldone(0);
			  break;
		case 'w': verbose += 64; break;
		case 'X': xspin = notabs = 1;
#ifndef PC
			  signal(SIGPIPE, alldone); /* not posix... */
#endif
			  break;
		case 'Y': limited_vis = 1; break;	/* used by xspin */
		case 'Z': preprocessonly = 1; break;	/* used by xspin */

		default : usage(); break;
		}
		argc--; argv++;
	}
	if (usedopts && !analyze)
		printf("spin: warning -o[123] option ignored in simulations\n");
	
	if (ltl_file)
	{	char formula[4096];
		add_ltl = ltl_file-2; add_ltl[1][1] = 'f';
		if (!(tl_out = fopen(*ltl_file, "r")))
		{	printf("spin: cannot open %s\n", *ltl_file);
			alldone(1);
		}
		fgets(formula, 4096, tl_out);
		fclose(tl_out);
		tl_out = stdout;
		*ltl_file = (char *) formula;
	}
	if (argc > 1)
	{	char cmd[128], out2[64];

		/* must remain in current dir */
		strcpy(out1, "pan.pre");

		if (add_ltl || nvr_file)
			strcpy(out2, "pan.___");

		if (add_ltl)
		{	tl_out = cpyfile(argv[1], out2);
			nr_errs = tl_main(2, add_ltl);	/* in tl_main.c */
			fclose(tl_out);
			preprocess(out2, out1, 1);
		} else if (nvr_file)
		{	FILE *fd; char buf[1024];
			
			if ((fd = fopen(*nvr_file, "r")) == NULL)
			{	printf("spin: cannot open %s\n",
					*nvr_file);
				alldone(1);
			}
			tl_out = cpyfile(argv[1], out2);
			while (fgets(buf, 1024, fd))
				fprintf(tl_out, "%s", buf);
			fclose(tl_out);
			fclose(fd);
			preprocess(out2, out1, 1);
		} else
			preprocess(argv[1], out1, 0);

		if (preprocessonly)
			alldone(0);

		if (!(yyin = fopen(out1, "r")))
		{	printf("spin: cannot open %s\n", out1);
			alldone(1);
		}

		if (strncmp(argv[1], "progress", 8) == 0
		||  strncmp(argv[1], "accept", 6) == 0)
			sprintf(cmd, "_%s", argv[1]);
		else
			strcpy(cmd, argv[1]);
		oFname = Fname = lookup(cmd);
		if (oFname->name[0] == '\"')
		{	int i = (int) strlen(oFname->name);
			oFname->name[i-1] = '\0';
			oFname = lookup(&oFname->name[1]);
		}
	} else
	{	oFname = Fname = lookup("<stdin>");
		if (add_ltl)
		{	if (argc > 0)
				exit(tl_main(2, add_ltl));
			printf("spin: missing argument to -f\n");
			alldone(1);
		}
		printf("%s\n", Version);
		printf("reading input from stdin:\n");
		fflush(stdout);
	}
	if (columns == 2)
	{	extern void putprelude(void);
		if (xspin || verbose&(1|4|8|16|32))
		{	printf("spin: -c precludes all flags except -t\n");
			alldone(1);
		}
		putprelude();
	}
	if (columns && !(verbose&8) && !(verbose&16))
		verbose += (8+16);
	if (columns == 2 && limited_vis)
		verbose += (1+4);
	Srand(T);	/* defined in run.c */
	SeedUsed = T;
	s = lookup("_");	s->type = PREDEF; /* write-only global var */
	s = lookup("_p");	s->type = PREDEF;
	s = lookup("_pid");	s->type = PREDEF;
	s = lookup("_last");	s->type = PREDEF;
	s = lookup("_nr_pr");	s->type = PREDEF; /* new 3.3.10 */

	yyparse();
	fclose(yyin);
	loose_ends();

	if (inlineonly)
	{	repro_src();
		return 0;
	}

	chanaccess();
	if (!Caccess)
	{	if (!s_trail && (dataflow || merger))
			ana_src(dataflow, merger);
		sched();
		alldone(nr_errs);
	}
	return 0;
}

int
yywrap(void)	/* dummy routine */
{
	return 1;
}

void
non_fatal(char *s1, char *s2)
{	extern char yytext[];

	printf("spin: line %3d %s, Error: ",
		lineno, Fname?Fname->name:"nofilename");
	if (s2)
		printf(s1, s2);
	else
		printf(s1);
	if (yytext && strlen(yytext)>1)
		printf(" near '%s'", yytext);
	printf("\n");
	nr_errs++;
}

void
fatal(char *s1, char *s2)
{
	non_fatal(s1, s2);
	alldone(1);
}

char *
emalloc(int n)
{	char *tmp;

	if (n == 0)
		return NULL;	/* robert shelton 10/20/06 */

	if (!(tmp = (char *) malloc(n)))
		fatal("not enough memory", (char *)0);
	memset(tmp, 0, n);
	return tmp;
}

void
trapwonly(Lextok *n, char *unused)
{	extern int realread;
	short i = (n->sym)?n->sym->type:0;

	if (i != MTYPE
	&&  i != BIT
	&&  i != BYTE
	&&  i != SHORT
	&&  i != INT
	&&  i != UNSIGNED)
		return;

	if (realread)
	n->sym->hidden |= 128;	/* var is read at least once */
}

void
setaccess(Symbol *sp, Symbol *what, int cnt, int t)
{	Access *a;

	for (a = sp->access; a; a = a->lnk)
		if (a->who == context
		&&  a->what == what
		&&  a->cnt == cnt
		&&  a->typ == t)
			return;

	a = (Access *) emalloc(sizeof(Access));
	a->who = context;
	a->what = what;
	a->cnt = cnt;
	a->typ = t;
	a->lnk = sp->access;
	sp->access = a;
}

Lextok *
nn(Lextok *s, int t, Lextok *ll, Lextok *rl)
{	Lextok *n = (Lextok *) emalloc(sizeof(Lextok));
	static int warn_nn = 0;

	n->ntyp = (short) t;
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

	if (!context) return n;

	if (t == 'r' || t == 's')
		setaccess(n->sym, ZS, 0, t);
	if (t == 'R')
		setaccess(n->sym, ZS, 0, 'P');

	if (context->name == claimproc)
	{	int forbidden = separate;
		switch (t) {
		case ASGN:
			printf("spin: Warning, never claim has side-effect\n");
			break;
		case 'r': case 's':
			non_fatal("never claim contains i/o stmnts",(char *)0);
			break;
		case TIMEOUT:
			/* never claim polls timeout */
			if (Ntimeouts && Etimeouts)
				forbidden = 0;
			Ntimeouts++; Etimeouts--;
			break;
		case LEN: case EMPTY: case FULL:
		case 'R': case NFULL: case NEMPTY:
			/* status becomes non-exclusive */
			if (n->sym && !(n->sym->xu&XX))
			{	n->sym->xu |= XX;
				if (separate == 2) {
				printf("spin: warning, make sure that the S1 model\n");
				printf("      also polls channel '%s' in its claim\n",
				n->sym->name); 
			}	}
			forbidden = 0;
			break;
		case 'c':
			AST_track(n, 0);	/* register as a slice criterion */
			/* fall thru */
		default:
			forbidden = 0;
			break;
		}
		if (forbidden)
		{	printf("spin: never, saw "); explain(t); printf("\n");
			fatal("incompatible with separate compilation",(char *)0);
		}
	} else if ((t == ENABLED || t == PC_VAL) && !(warn_nn&t))
	{	printf("spin: Warning, using %s outside never claim\n",
			(t == ENABLED)?"enabled()":"pc_value()");
		warn_nn |= t;
	} else if (t == NONPROGRESS)
	{	fatal("spin: Error, using np_ outside never claim\n", (char *)0);
	}
	return n;
}

Lextok *
rem_lab(Symbol *a, Lextok *b, Symbol *c)	/* proctype name, pid, label name */
{	Lextok *tmp1, *tmp2, *tmp3;

	has_remote++;
	c->type = LABEL;	/* refered to in global context here */
	fix_dest(c, a);		/* in case target of rem_lab is jump */
	tmp1 = nn(ZN, '?',   b, ZN); tmp1->sym = a;
	tmp1 = nn(ZN, 'p', tmp1, ZN);
	tmp1->sym = lookup("_p");
	tmp2 = nn(ZN, NAME,  ZN, ZN); tmp2->sym = a;
	tmp3 = nn(ZN, 'q', tmp2, ZN); tmp3->sym = c;
	return nn(ZN, EQ, tmp1, tmp3);
#if 0
	      .---------------EQ-------.
	     /                          \
	   'p' -sym-> _p               'q' -sym-> c (label name)
	   /                           /
	 '?' -sym-> a (proctype)     NAME -sym-> a (proctype name)
	 / 
	b (pid expr)
#endif
}

Lextok *
rem_var(Symbol *a, Lextok *b, Symbol *c, Lextok *ndx)
{	Lextok *tmp1;

	has_remote++;
	has_remvar++;
	dataflow = 0;	/* turn off dead variable resets 4.2.5 */
	tmp1 = nn(ZN, '?', b, ZN); tmp1->sym = a;
	tmp1 = nn(ZN, 'p', tmp1, ndx);
	tmp1->sym = c;
	return tmp1;
#if 0
	cannot refer to struct elements
	only to scalars and arrays

	    'p' -sym-> c (variable name)
	    / \______  possible arrayindex on c
	   /
	 '?' -sym-> a (proctype)
	 / 
	b (pid expr)
#endif
}

static void
explain(int n)
{	FILE *fd = stdout;
	switch (n) {
	default:	if (n > 0 && n < 256)
				fprintf(fd, "'%c' = '", n);
			fprintf(fd, "%d'", n);
			break;
	case '\b':	fprintf(fd, "\\b"); break;
	case '\t':	fprintf(fd, "\\t"); break;
	case '\f':	fprintf(fd, "\\f"); break;
	case '\n':	fprintf(fd, "\\n"); break;
	case '\r':	fprintf(fd, "\\r"); break;
	case 'c':	fprintf(fd, "condition"); break;
	case 's':	fprintf(fd, "send"); break;
	case 'r':	fprintf(fd, "recv"); break;
	case 'R':	fprintf(fd, "recv poll %s", Operator); break;
	case '@':	fprintf(fd, "@"); break;
	case '?':	fprintf(fd, "(x->y:z)"); break;
	case ACTIVE:	fprintf(fd, "%sactive",	Keyword); break;
	case AND:	fprintf(fd, "%s&&",	Operator); break;
	case ASGN:	fprintf(fd, "%s=",	Operator); break;
	case ASSERT:	fprintf(fd, "%sassert",	Function); break;
	case ATOMIC:	fprintf(fd, "%satomic",	Keyword); break;
	case BREAK:	fprintf(fd, "%sbreak",	Keyword); break;
	case C_CODE:	fprintf(fd, "%sc_code",	Keyword); break;
	case C_DECL:	fprintf(fd, "%sc_decl",	Keyword); break;
	case C_EXPR:	fprintf(fd, "%sc_expr",	Keyword); break;
	case C_STATE:	fprintf(fd, "%sc_state",Keyword); break;
	case C_TRACK:	fprintf(fd, "%sc_track",Keyword); break;
	case CLAIM:	fprintf(fd, "%snever",	Keyword); break;
	case CONST:	fprintf(fd, "a constant"); break;
	case DECR:	fprintf(fd, "%s--",	Operator); break;
	case D_STEP:	fprintf(fd, "%sd_step",	Keyword); break;
	case D_PROCTYPE: fprintf(fd, "%sd_proctype", Keyword); break;
	case DO:	fprintf(fd, "%sdo",	Keyword); break;
	case DOT:	fprintf(fd, "."); break;
	case ELSE:	fprintf(fd, "%selse",	Keyword); break;
	case EMPTY:	fprintf(fd, "%sempty",	Function); break;
	case ENABLED:	fprintf(fd, "%senabled",Function); break;
	case EQ:	fprintf(fd, "%s==",	Operator); break;
	case EVAL:	fprintf(fd, "%seval",	Function); break;
	case FI:	fprintf(fd, "%sfi",	Keyword); break;
	case FULL:	fprintf(fd, "%sfull",	Function); break;
	case GE:	fprintf(fd, "%s>=",	Operator); break;
	case GOTO:	fprintf(fd, "%sgoto",	Keyword); break;
	case GT:	fprintf(fd, "%s>",	Operator); break;
	case HIDDEN:	fprintf(fd, "%shidden",	Keyword); break;
	case IF:	fprintf(fd, "%sif",	Keyword); break;
	case INCR:	fprintf(fd, "%s++",	Operator); break;
	case INAME:	fprintf(fd, "inline name"); break;
	case INLINE:	fprintf(fd, "%sinline",	Keyword); break;
	case INIT:	fprintf(fd, "%sinit",	Keyword); break;
	case ISLOCAL:	fprintf(fd, "%slocal",  Keyword); break;
	case LABEL:	fprintf(fd, "a label-name"); break;
	case LE:	fprintf(fd, "%s<=",	Operator); break;
	case LEN:	fprintf(fd, "%slen",	Function); break;
	case LSHIFT:	fprintf(fd, "%s<<",	Operator); break;
	case LT:	fprintf(fd, "%s<",	Operator); break;
	case MTYPE:	fprintf(fd, "%smtype",	Keyword); break;
	case NAME:	fprintf(fd, "an identifier"); break;
	case NE:	fprintf(fd, "%s!=",	Operator); break;
	case NEG:	fprintf(fd, "%s! (not)",Operator); break;
	case NEMPTY:	fprintf(fd, "%snempty",	Function); break;
	case NFULL:	fprintf(fd, "%snfull",	Function); break;
	case NON_ATOMIC: fprintf(fd, "sub-sequence"); break;
	case NONPROGRESS: fprintf(fd, "%snp_",	Function); break;
	case OD:	fprintf(fd, "%sod",	Keyword); break;
	case OF:	fprintf(fd, "%sof",	Keyword); break;
	case OR:	fprintf(fd, "%s||",	Operator); break;
	case O_SND:	fprintf(fd, "%s!!",	Operator); break;
	case PC_VAL:	fprintf(fd, "%spc_value",Function); break;
	case PNAME:	fprintf(fd, "process name"); break;
	case PRINT:	fprintf(fd, "%sprintf",	Function); break;
	case PRINTM:	fprintf(fd, "%sprintm",	Function); break;
	case PRIORITY:	fprintf(fd, "%spriority", Keyword); break;
	case PROCTYPE:	fprintf(fd, "%sproctype",Keyword); break;
	case PROVIDED:	fprintf(fd, "%sprovided",Keyword); break;
	case RCV:	fprintf(fd, "%s?",	Operator); break;
	case R_RCV:	fprintf(fd, "%s??",	Operator); break;
	case RSHIFT:	fprintf(fd, "%s>>",	Operator); break;
	case RUN:	fprintf(fd, "%srun",	Operator); break;
	case SEP:	fprintf(fd, "token: ::"); break;
	case SEMI:	fprintf(fd, ";"); break;
	case SHOW:	fprintf(fd, "%sshow", Keyword); break;
	case SND:	fprintf(fd, "%s!",	Operator); break;
	case STRING:	fprintf(fd, "a string"); break;
	case TRACE:	fprintf(fd, "%strace", Keyword); break;
	case TIMEOUT:	fprintf(fd, "%stimeout",Keyword); break;
	case TYPE:	fprintf(fd, "data typename"); break;
	case TYPEDEF:	fprintf(fd, "%stypedef",Keyword); break;
	case XU:	fprintf(fd, "%sx[rs]",	Keyword); break;
	case UMIN:	fprintf(fd, "%s- (unary minus)", Operator); break;
	case UNAME:	fprintf(fd, "a typename"); break;
	case UNLESS:	fprintf(fd, "%sunless",	Keyword); break;
	}
}
