/***** spin: pangen3.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include "spin.h"
#include "y.tab.h"
#include <assert.h>

extern FILE	*fd_th, *fd_tc;
extern int	eventmapnr, old_priority_rules, in_settr;

typedef struct SRC {
	int ln, st;	/* linenr, statenr */
	Symbol *fn;	/* filename */
	struct SRC *nxt;
} SRC;

static int	col;
static Symbol	*lastfnm;
static Symbol	lastdef;
static int	lastfrom;
static SRC	*frst = (SRC *) 0;
static SRC	*skip = (SRC *) 0;

extern int	ltl_mode;

extern void	sr_mesg(FILE *, int, int, const char *);
extern Lextok	**find_mtype_list(const char *);

static void
putnr(int n)
{
	if (col++ == 8)
	{	fprintf(fd_tc, "\n\t");	/* was th */
		col = 1;
	}
	fprintf(fd_tc, "%3d, ", n);	/* was th */
}

static void
putfnm(int j, Symbol *s)
{
	if (lastfnm && lastfnm == s && j != -1)
		return;

	if (lastfnm)
		fprintf(fd_tc, "{ \"%s\", %d, %d },\n\t",	/* was th */
			lastfnm->name,
			lastfrom,
			j-1);
	lastfnm = s;
	lastfrom = j;
}

static void
putfnm_flush(int j)
{
	if (lastfnm)
		fprintf(fd_tc, "{ \"%s\", %d, %d }\n",	/* was th */
			lastfnm->name,
			lastfrom, j);
}

static SRC *
newsrc(int m, SRC *n)
{	SRC *tmp;
	tmp = (SRC *) emalloc(sizeof(SRC));
	tmp->st  = m;
	tmp->nxt = n;
	return tmp;
}

void
putskip(int m)	/* states that need not be reached */
{	SRC *tmp, *lst = (SRC *)0;
	/* 6.4.0: now an ordered list */
	for (tmp = skip; tmp; lst = tmp, tmp = tmp->nxt)
	{	if (tmp->st == m)
		{	return;
		}
		if (tmp->st > m)	/* insert before */
		{	if (tmp == skip)
			{	tmp = newsrc(m, skip);
				skip = tmp;
			} else
			{	assert(lst);
				tmp = newsrc(m, lst->nxt);
				lst->nxt = tmp;
			}
			return;	
	}	}
	/* insert at the end */
	if (lst)
	{	lst->nxt = newsrc(m, 0);
	} else	/* empty list */
	{	skip = newsrc(m, 0);
	}
}

void
unskip(int m)	/* a state that needs to be reached after all */
{	SRC *tmp, *lst = (SRC *)0;

	for (tmp = skip; tmp; lst = tmp, tmp = tmp->nxt)
	{	if (tmp->st == m)
		{	if (tmp == skip)
				skip = skip->nxt;
			else if (lst)	/* always true, but helps coverity */
				lst->nxt = tmp->nxt;
			break;
		}
		if (tmp->st > m)
		{	break;	/* m is not in list */
	}	}
}

void
putsrc(Element *e)	/* match states to source lines */
{	SRC *tmp, *lst = (SRC *)0;
	int n, m;

	if (!e || !e->n) return;

	n = e->n->ln;
	m = e->seqno;
	/* 6.4.0: now an ordered list */
	for (tmp = frst; tmp; lst = tmp, tmp = tmp->nxt)
	{	if (tmp->st == m)
		{	if (tmp->ln != n || tmp->fn != e->n->fn)
			printf("putsrc mismatch seqno %d, line %d - %d, file %s\n", m, n,
				tmp->ln, tmp->fn->name);
			return;
		}
		if (tmp->st > m) /* insert before */
		{	if (tmp == frst)
			{	tmp = newsrc(m, frst);
				frst = tmp;
			} else
			{	assert(lst);
				tmp = newsrc(m, lst->nxt);
				lst->nxt = tmp;
			}
			tmp->ln = n;
			tmp->fn = e->n->fn;
			return;
	}	}
	/* insert at the end */
	tmp = newsrc(m, lst?lst->nxt:0);
	tmp->ln = n;
	tmp->fn = e->n->fn;
	if (lst)
	{	lst->nxt = tmp;
	} else
	{	frst = tmp;
	}
}

static void
dumpskip(int n, int m)
{	SRC *tmp, *lst;
	FILE *tz = fd_tc;	/* was fd_th */
	int j;

	fprintf(tz, "uchar reached%d [] = {\n\t", m);
	tmp = skip;
	lst = (SRC *) 0;
	for (j = 0, col = 0; j <= n; j++)
	{	/* find j in the sorted list */
		for ( ; tmp; lst = tmp, tmp = tmp->nxt)
		{	if (tmp->st == j)
			{	putnr(1);
				if (lst)
					lst->nxt = tmp->nxt;
				else
					skip = tmp->nxt;
				break;
			}
			if (tmp->st > j)
			{	putnr(0);
				break;	/* j is not in the list */
		}	}

		if (!tmp)
		{	putnr(0);
	}	}
	fprintf(tz, "};\n");
	fprintf(tz, "uchar *loopstate%d;\n", m);

	if (m == eventmapnr)
		fprintf(fd_th, "#define reached_event	reached%d\n", m);

	skip = (SRC *) 0;
}

void
dumpsrc(int n, int m)
{	SRC *tmp, *lst;
	int j;
	static int did_claim = 0;
	FILE *tz = fd_tc;	/* was fd_th */

	fprintf(tz, "\nshort src_ln%d [] = {\n\t", m);
	tmp = frst;
	for (j = 0, col = 0; j <= n; j++)
	{	for ( ; tmp; tmp = tmp->nxt)
		{	if (tmp->st == j)
			{	putnr(tmp->ln);
				break;
			}
			if (tmp->st > j)
			{	putnr(0);
				break;
		}	}
		if (!tmp)
		{	putnr(0);
	}	}
	fprintf(tz, "};\n");

	lastfnm = (Symbol *) 0;
	lastdef.name = "-";
	fprintf(tz, "S_F_MAP src_file%d [] = {\n\t", m);
	tmp = frst;
	lst = (SRC *) 0;
	for (j = 0, col = 0; j <= n; j++)
	{	for ( ; tmp; lst = tmp, tmp = tmp->nxt)
		{	if (tmp->st == j)
			{	putfnm(j, tmp->fn);
				if (lst)
					lst->nxt = tmp->nxt;
				else
					frst = tmp->nxt;
				break;
			}
			if (tmp->st > j)
			{	putfnm(j, &lastdef);
				break;
		}	}
		if (!tmp)
		{	putfnm(j, &lastdef);
	}	}
	putfnm_flush(j);
	fprintf(tz, "};\n");

	if (pid_is_claim(m) && !did_claim)
	{	fprintf(tz, "short *src_claim;\n");
		did_claim++;
	}
	if (m == eventmapnr)
		fprintf(fd_th, "#define src_event	src_ln%d\n", m);

	frst = (SRC *) 0;
	dumpskip(n, m);
}

#define Cat0(x)   	comwork(fd,now->lft,m); fprintf(fd, x); \
			comwork(fd,now->rgt,m)
#define Cat1(x)		fprintf(fd,"("); Cat0(x); fprintf(fd,")")
#define Cat2(x,y)  	fprintf(fd,x); comwork(fd,y,m)
#define Cat3(x,y,z)	fprintf(fd,x); comwork(fd,y,m); fprintf(fd,z)

static int
symbolic(FILE *fd, Lextok *tv)
{	Lextok *n, *Mtype;
	int cnt = 1;

	if (tv->ismtyp)
	{	char *s = "_unnamed_";
		if (tv->sym && tv->sym->mtype_name)
		{	s = tv->sym->mtype_name->name;
		}
		Mtype = *find_mtype_list(s);
		for (n = Mtype; n; n = n->rgt, cnt++)
		{	if (cnt == tv->val)
			{	fprintf(fd, "%s", n->lft->sym->name);
				return 1;
	}	}	}

	return 0;
}

static void
comwork(FILE *fd, Lextok *now, int m)
{	Lextok *v;
	char *s = 0;
	int i, j;

	if (!now) { fprintf(fd, "0"); return; }
	switch (now->ntyp) {
	case CONST:	if (now->ismtyp
			&&  now->sym
			&&  now->sym->mtype_name)
			{	s = now->sym->mtype_name->name;
			}
			sr_mesg(fd, now->val, now->ismtyp, s);
			break;

	case '!':	Cat3("!(", now->lft, ")"); break;
	case UMIN:	Cat3("-(", now->lft, ")"); break;
	case '~':	Cat3("~(", now->lft, ")"); break;

	case '/':	Cat1("/");  break;
	case '*':	Cat1("*");  break;
	case '-':	Cat1("-");  break;
	case '+':	Cat1("+");  break;
	case '%':	Cat1("%%"); break;
	case '&':	Cat1("&");  break;
	case '^':	Cat1("^");  break;
	case '|':	Cat1("|");  break;
	case LE:	Cat1("<="); break;
	case GE:	Cat1(">="); break;
	case GT:	Cat1(">"); break;
	case LT:	Cat1("<"); break;
	case NE:	Cat1("!="); break;
	case EQ:
			if (ltl_mode
			&&  now->lft->ntyp == 'p'
			&&  now->rgt->ntyp == 'q')	/* remote ref */
			{	Lextok *p = now->lft->lft;

				fprintf(fd, "(");
				fprintf(fd, "%s", p->sym->name);
				if (p->lft)
				{	fprintf(fd, "[");
					putstmnt(fd, p->lft, 0); /* pid */
					fprintf(fd, "]");
				}
				fprintf(fd, "@");
				fprintf(fd, "%s", now->rgt->sym->name);
				fprintf(fd, ")");
				break;
			}
			Cat1("==");
			break;

	case OR:	Cat1("||"); break;
	case AND:	Cat1("&&"); break;
	case LSHIFT:	Cat1("<<"); break;
	case RSHIFT:	Cat1(">>"); break;

	case RUN:	fprintf(fd, "run %s(", now->sym->name);
			for (v = now->lft; v; v = v->rgt)
				if (v == now->lft)
				{	comwork(fd, v->lft, m);
				} else
				{	Cat2(",", v->lft);
				}
			fprintf(fd, ")");
			break;

	case LEN:	putname(fd, "len(", now->lft, m, ")");
			break;
	case FULL:	putname(fd, "full(", now->lft, m, ")");
			break;
	case EMPTY:	putname(fd, "empty(", now->lft, m, ")");
			break;
	case NFULL:	putname(fd, "nfull(", now->lft, m, ")");
			break;
	case NEMPTY:	putname(fd, "nempty(", now->lft, m, ")");
			break;

	case 's':	putname(fd, "", now->lft, m, now->val?"!!":"!");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v != now->rgt) fprintf(fd,",");
				if (!symbolic(fd, v->lft))
					comwork(fd,v->lft,m);
			}
			break;
	case 'r':	putname(fd, "", now->lft, m, "?");
			switch (now->val) {
			case 0: break;
			case 1: fprintf(fd, "?");  break;
			case 2: fprintf(fd, "<");  break;
			case 3: fprintf(fd, "?<"); break;
			}
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v != now->rgt) fprintf(fd,",");
				if (!symbolic(fd, v->lft))
					comwork(fd,v->lft,m);
			}
			if (now->val >= 2)
				fprintf(fd, ">");
			break;
	case 'R':	putname(fd, "", now->lft, m,  now->val?"??[":"?[");
			for (v = now->rgt, i=0; v; v = v->rgt, i++)
			{	if (v != now->rgt) fprintf(fd,",");
				if (!symbolic(fd, v->lft))
					comwork(fd,v->lft,m);
			}
			fprintf(fd, "]");
			break;

	case ENABLED:	Cat3("enabled(", now->lft, ")");
			break;

	case GET_P:	if (old_priority_rules)
			{	fprintf(fd, "1");
			} else
			{	Cat3("get_priority(", now->lft, ")");
			}
			break;

	case SET_P:	if (!old_priority_rules)
			{	fprintf(fd, "set_priority(");
				comwork(fd, now->lft->lft, m);
				fprintf(fd, ", ");
				comwork(fd, now->lft->rgt, m);
				fprintf(fd, ")");
			}
			break;

	case EVAL:	if (now->lft->ntyp == ',')
			{	Cat3("eval(", now->lft->lft, ")");
			} else
			{	Cat3("eval(", now->lft, ")");
			}
			break;

	case NONPROGRESS:
			fprintf(fd, "np_");
			break;

	case PC_VAL:	Cat3("pc_value(", now->lft, ")");
			break;

	case 'c':	Cat3("(", now->lft, ")");
			break;

	case '?':	if (now->lft)
			{	Cat3("( (", now->lft, ") -> ");
			}
			if (now->rgt)
			{	Cat3("(", now->rgt->lft, ") : ");
				Cat3("(", now->rgt->rgt, ") )");
			}
			break;	

	case ASGN:
			if (check_track(now) == STRUCT) { break; }
			comwork(fd,now->lft,m);
			fprintf(fd," = ");
			comwork(fd,now->rgt,m);
			break;

	case PRINT:	{	char c, buf[1024];
				strncpy(buf, now->sym->name, 510);
				for (i = j = 0; i < 510; i++, j++)
				{	c = now->sym->name[i];
					buf[j] = c;
					if (c == '\\') buf[++j] = c;
					if (c == '\"') buf[j] = '\'';
					if (c == '\0') break;
				}
				if (now->ntyp == PRINT)
					fprintf(fd, "printf");
				else
					fprintf(fd, "annotate");
				fprintf(fd, "(%s", buf);
			}
			for (v = now->lft; v; v = v->rgt)
			{	Cat2(",", v->lft);
			}
			fprintf(fd, ")");
			break;
	case PRINTM:	fprintf(fd, "printm(");
			{ char *s = 0;
		  	  if (now->lft->sym
		  	  &&  now->lft->sym->mtype_name)
		  	  {	s = now->lft->sym->mtype_name->name;
		  	  }

			  if (now->lft && now->lft->ismtyp)
			  {	fprintf(fd, "%d", now->lft->val);
			  } else
			  {	comwork(fd, now->lft, m);
			  }

			  if (s)
			  {	if (in_settr)
				{	fprintf(fd, ", '%s')", s);
				} else
				{	fprintf(fd, ", \"%s\")", s);
				}
			  } else
			  {	fprintf(fd, ", 0)");
			  }
			}
			break;
	case NAME:
			putname(fd, "", now, m, "");
			break;

	case   'p':
			if (ltl_mode)
			{	fprintf(fd, "%s", now->lft->sym->name); /* proctype */
				if (now->lft->lft)
				{	fprintf(fd, "[");
					putstmnt(fd, now->lft->lft, 0); /* pid */
					fprintf(fd, "]");
				}
				fprintf(fd, ":");	/* remote varref */
				fprintf(fd, "%s", now->sym->name);	/* varname */
				break;
			}
			putremote(fd, now, m);
			break;
	case   'q':	fprintf(fd, "%s", now->sym->name);
			break;
	case C_EXPR:	
	case C_CODE:	fprintf(fd, "{%s}", now->sym->name);
			break;
	case ASSERT:	Cat3("assert(", now->lft, ")");
			break;
	case   '.':	fprintf(fd, ".(goto)"); break;
	case  GOTO:	fprintf(fd, "goto %s", now->sym->name); break;
	case BREAK:	fprintf(fd, "break"); break;
	case  ELSE:	fprintf(fd, "else"); break;
	case   '@':	fprintf(fd, "-end-"); break;

	case D_STEP:	fprintf(fd, "D_STEP%d", now->ln); break;
	case ATOMIC:	fprintf(fd, "ATOMIC"); break;
	case NON_ATOMIC: fprintf(fd, "sub-sequence"); break;
	case IF:	fprintf(fd, "IF"); break;
	case DO:	fprintf(fd, "DO"); break;
	case UNLESS:	fprintf(fd, "unless"); break;
	case TIMEOUT:	fprintf(fd, "timeout"); break;
	default:	if (isprint(now->ntyp))
				fprintf(fd, "'%c'", now->ntyp);
			else
				fprintf(fd, "%d", now->ntyp);
			break;
	}
}

void
comment(FILE *fd, Lextok *now, int m)
{	extern short terse, nocast;

	terse=nocast=1;
	comwork(fd, now, m);
	terse=nocast=0;
}
