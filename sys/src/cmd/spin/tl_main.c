/***** tl_spin: tl_main.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 *
 * Based on the translation algorithm by Gerth, Peled, Vardi, and Wolper,
 * presented at the PSTV Conference, held in 1995, Warsaw, Poland 1995.
 */

#include "tl.h"

extern FILE	*tl_out;

int	newstates  = 0;	/* debugging only */
int	tl_errs    = 0;
int	tl_verbose = 0;
int	tl_terse   = 0;
int	tl_clutter = 0;
int	state_cnt = 0;

unsigned long	All_Mem = 0;
char	*claim_name;

static char	*uform;
static int	hasuform=0, cnt=0;

extern void cache_stats(void);
extern void a_stats(void);

int
tl_Getchar(void)
{
	if (cnt < hasuform)
		return uform[cnt++];
	cnt++;
	return -1;
}

int
tl_peek(int n)
{
	if (cnt+n < hasuform)
	{	return uform[cnt+n];
	}
	return -1;
}

void
tl_balanced(void)
{	int i;
	int k = 0;

	for (i = 0; i < hasuform; i++)
	{	if (uform[i] == '(')
		{	if (i > 0
			&& ((uform[i-1] == '"'  && uform[i+1] == '"')
			||  (uform[i-1] == '\'' && uform[i+1] == '\'')))
			{	continue;
			}
			k++;
		} else if (uform[i] == ')')
		{	if (i > 0
			&& ((uform[i-1] == '"'  && uform[i+1] == '"')
			||  (uform[i-1] == '\'' && uform[i+1] == '\'')))
			{	continue;
			}
			k--;
	}	}

	if (k != 0)
	{	tl_errs++;
		tl_yyerror("parentheses not balanced");
	}
}

void
put_uform(void)
{
	fprintf(tl_out, "%s", uform);
}

void
tl_UnGetchar(void)
{
	if (cnt > 0) cnt--;
}

static void
tl_stats(void)
{	extern int Stack_mx;
	printf("total memory used: %9ld\n", All_Mem);
	printf("largest stack sze: %9d\n", Stack_mx);
	cache_stats();
	a_stats();
}

int
tl_main(int argc, char *argv[])
{	int i;
	extern int xspin, s_trail;

	tl_verbose = 0; /* was: tl_verbose = verbose; */
	if (xspin && s_trail)
	{	tl_clutter = 1;
		/* generating claims for a replay should
		   be done the same as when generating the
		   pan.c that produced the error-trail */
	} else
	{	tl_clutter = 1-xspin;	/* use -X -f to turn off uncluttering */
	}
	newstates  = 0;
	state_cnt  = 0;
	tl_errs    = 0;
	tl_terse   = 0;
	All_Mem = 0;
	hasuform=0;
	cnt=0;
	claim_name = (char *) 0;

	ini_buchi();
	ini_cache();
	ini_rewrt();
	ini_trans();

	while (argc > 1 && argv[1][0] == '-')
	{
		switch (argv[1][1]) {
		case 'd':	newstates = 1;	/* debugging mode */
				break;
		case 'f':	argc--; argv++;
				for (i = 0; argv[1][i]; i++)
				{	if (argv[1][i] == '\t'
					||  argv[1][i] == '\n')
						argv[1][i] = ' ';
				}
				size_t len = strlen(argv[1]);
                uform = tl_emalloc(len + 1);
				strcpy(uform, argv[1]);
				hasuform = (int) len;
				break;
		case 'v':	tl_verbose++;
				break;
		case 'n':	tl_terse = 1;
				break;
		case 'c':	argc--; argv++;
				claim_name = (char *) emalloc(strlen(argv[1])+1);
				strcpy(claim_name, argv[1]);
				break;
		default :	printf("spin -f: saw '-%c'\n", argv[1][1]);
				goto nogood;
		}
		argc--; argv++;
	}
	if (hasuform == 0)
	{
nogood:		printf("usage:\tspin [-v] [-n] -f formula\n");
		printf("	-v verbose translation\n");
		printf("	-n normalize tl formula and exit\n");
		exit(1);
	}
	tl_balanced();

	if (tl_errs == 0)
		tl_parse();

	if (tl_verbose) tl_stats();
	return tl_errs;
}

#define Binop(a)		\
		fprintf(tl_out, "(");	\
		dump(n->lft);		\
		fprintf(tl_out, a);	\
		dump(n->rgt);		\
		fprintf(tl_out, ")")

void
dump(Node *n)
{
	if (!n) return;

	switch(n->ntyp) {
	case OR:	Binop(" || "); break;
	case AND:	Binop(" && "); break;
	case U_OPER:	Binop(" U ");  break;
	case V_OPER:	Binop(" V ");  break;
#ifdef NXT
	case NEXT:
		fprintf(tl_out, "X");
		fprintf(tl_out, " (");
		dump(n->lft);
		fprintf(tl_out, ")");
		break;
#endif
	case NOT:
		fprintf(tl_out, "!");
		fprintf(tl_out, " (");
		dump(n->lft);
		fprintf(tl_out, ")");
		break;
	case FALSE:
		fprintf(tl_out, "false");
		break;
	case TRUE:
		fprintf(tl_out, "true");
		break;
	case PREDICATE:
		fprintf(tl_out, "(%s)", n->sym->name);
		break;
	case CEXPR:
		fprintf(tl_out, "c_expr");
		fprintf(tl_out, " {");
		dump(n->lft);
		fprintf(tl_out, "}");
		break;
	case -1:
		fprintf(tl_out, " D ");
		break;
	default:
		printf("Unknown token: ");
		tl_explain(n->ntyp);
		break;
	}
}

void
tl_explain(int n)
{
	switch (n) {
	case ALWAYS:	printf("[]"); break;
	case EVENTUALLY: printf("<>"); break;
	case IMPLIES:	printf("->"); break;
	case EQUIV:	printf("<->"); break;
	case PREDICATE:	printf("predicate"); break;
	case OR:	printf("||"); break;
	case AND:	printf("&&"); break;
	case NOT:	printf("!"); break;
	case U_OPER:	printf("U"); break;
	case V_OPER:	printf("V"); break;
#ifdef NXT
	case NEXT:	printf("X"); break;
#endif
	case CEXPR:	printf("c_expr"); break;
	case TRUE:	printf("true"); break;
	case FALSE:	printf("false"); break;
	case ';':	printf("end of formula"); break;
	default:	printf("%c", n); break;
	}
}

static void
tl_non_fatal(char *s1, char *s2)
{	extern int tl_yychar;
	int i;

	printf("tl_spin: ");
#if 1
	printf(s1, s2);	/* prevent a compiler warning */
#else
	if (s2)
		printf(s1, s2);
	else
		printf(s1);
#endif
	if (tl_yychar != -1 && tl_yychar != 0)
	{	printf(", saw '");
		tl_explain(tl_yychar);
		printf("'");
	}
	printf("\ntl_spin: %s\n---------", uform);
	for (i = 0; i < cnt; i++)
		printf("-");
	printf("^\n");
	fflush(stdout);
	tl_errs++;
}

void
tl_yyerror(char *s1)
{
	Fatal(s1, (char *) 0);
}

void
Fatal(char *s1, char *s2)
{
	tl_non_fatal(s1, s2);
	/* tl_stats(); */
	exit(1);
}
