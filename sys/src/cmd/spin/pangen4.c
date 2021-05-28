/***** spin: pangen4.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include "spin.h"
#include "y.tab.h"

extern FILE	*fd_tc, *fd_tb;
extern Queue	*qtab;
extern Symbol	*Fname;
extern int	lineno, m_loss, Pid_nr, eventmapnr, multi_oval;
extern short	nocast, has_provided, has_sorted;
extern const char *R13_[], *R14_[], *R15_[];

static void	check_proc(Lextok *, int);

void
undostmnt(Lextok *now, int m)
{	Lextok *v;
	int i, j;

	if (!now)
	{	fprintf(fd_tb, "0");
		return;
	}
	lineno = now->ln;
	Fname  = now->fn;
	switch (now->ntyp) {
	case CONST:	case '!':	case UMIN:
	case '~':	case '/':	case '*':
	case '-':	case '+':	case '%':
	case LT:	case GT:	case '&':
	case '|':	case LE:	case GE:
	case NE:	case EQ:	case OR:
	case AND:	case LSHIFT:	case RSHIFT:
	case TIMEOUT:	case LEN:	case NAME:
	case FULL:	case EMPTY:	case 'R':
	case NFULL:	case NEMPTY:	case ENABLED:
	case '?':	case PC_VAL:	case '^':
	case C_EXPR:	case GET_P:
	case NONPROGRESS:
		putstmnt(fd_tb, now, m);
		break;

	case RUN:
		fprintf(fd_tb, "delproc(0, now._nr_pr-1)");
		break;

	case 's':
		if (Pid_nr == eventmapnr) break;

		if (m_loss)
			fprintf(fd_tb, "if (_m == 2) ");
		putname(fd_tb, "_m = unsend(", now->lft, m, ")");
		break;

	case 'r':
		if (Pid_nr == eventmapnr) break;

		for (v = now->rgt, i=j=0; v; v = v->rgt, i++)
			if (v->lft->ntyp != CONST
			&&  v->lft->ntyp != EVAL)
				j++;
		if (j == 0 && now->val >= 2)
			break;	/* poll without side-effect */

		{	int ii = 0, jj;

			for (v = now->rgt; v; v = v->rgt)
				if ((v->lft->ntyp != CONST
				&&   v->lft->ntyp != EVAL))
					ii++;	/* nr of things bupped */
			if (now->val == 1)
			{	ii++;
				jj = multi_oval - ii - 1;
				fprintf(fd_tb, "XX = trpt->bup.oval");
				if (multi_oval > 0)
				{	fprintf(fd_tb, "s[%d]", jj);
					jj++;
				}
				fprintf(fd_tb, ";\n\t\t");
			} else
			{	fprintf(fd_tb, "XX = 1;\n\t\t");
				jj = multi_oval - ii - 1;
			}

			if (now->val < 2)	/* not for channel poll */
			for (v = now->rgt, i = 0; v; v = v->rgt, i++)
			{	switch(v->lft->ntyp) {
				case CONST:
				case EVAL:
					fprintf(fd_tb, "unrecv");
					putname(fd_tb, "(", now->lft, m, ", XX-1, ");
					fprintf(fd_tb, "%d, ", i);
					if (v->lft->ntyp == EVAL)
					{	if (v->lft->lft->ntyp == ',')
						{	undostmnt(v->lft->lft->lft, m);
						} else
						{	undostmnt(v->lft->lft, m);
						}
					} else
					{	undostmnt(v->lft, m);
					}
					fprintf(fd_tb, ", %d);\n\t\t", (i==0)?1:0);
					break;
				default:
					fprintf(fd_tb, "unrecv");
					putname(fd_tb, "(", now->lft, m, ", XX-1, ");
					fprintf(fd_tb, "%d, ", i);
					if (v->lft->sym
					&& !strcmp(v->lft->sym->name, "_"))
					{	fprintf(fd_tb, "trpt->bup.oval");
						if (multi_oval > 0)
							fprintf(fd_tb, "s[%d]", jj);
					} else
						putstmnt(fd_tb, v->lft, m);

					fprintf(fd_tb, ", %d);\n\t\t", (i==0)?1:0);
					if (multi_oval > 0)
						jj++;
					break;
			}	}
			jj = multi_oval - ii - 1;

			if (now->val == 1 && multi_oval > 0)
				jj++;	/* new 3.4.0 */

			for (v = now->rgt, i = 0; v; v = v->rgt, i++)
			{	switch(v->lft->ntyp) {
				case CONST:
				case EVAL:
					break;
				default:
					if (!v->lft->sym
					||  strcmp(v->lft->sym->name, "_") != 0)
					{	nocast=1; putstmnt(fd_tb, v->lft, m);
						nocast=0; fprintf(fd_tb, " = trpt->bup.oval");
						if (multi_oval > 0)
							fprintf(fd_tb, "s[%d]", jj);
						fprintf(fd_tb, ";\n\t\t");
					}
					if (multi_oval > 0)
						jj++;
					break;
			}	}
			multi_oval -= ii;
		}
		break;

	case '@':
		fprintf(fd_tb, "p_restor(II);\n\t\t");
		break;

	case SET_P:
		fprintf(fd_tb, "((P0 *)pptr((trpt->o_priority >> 8)))");
		fprintf(fd_tb, "->_priority = trpt->o_priority & 255");
		break;

	case ASGN:
		if (check_track(now) == STRUCT) { break; }

		nocast=1; putstmnt(fd_tb, now->lft, m);
		nocast=0; fprintf(fd_tb, " = trpt->bup.oval");
		if (multi_oval > 0)
		{	multi_oval--;
			fprintf(fd_tb, "s[%d]", multi_oval-1);
		}
		check_proc(now->rgt, m);
		break;

	case 'c':
		check_proc(now->lft, m);
		break;

	case '.':
	case GOTO:
	case ELSE:
	case BREAK:
		break;

	case C_CODE:
		fprintf(fd_tb, "sv_restor();\n");
		break;

	case ASSERT:
	case PRINT:
		check_proc(now, m);
		break;
	case PRINTM:
		break;

	case ',':
		if (now->lft)	/* eval usertype5 */
		{	undostmnt(now->lft, m);
			break;
		} /* else fall thru */
	default:
		printf("spin: bad node type %d (.b)\n", now->ntyp);
		alldone(1);
	}
}

int
any_undo(Lextok *now)
{	/* is there anything to undo on a return move? */
	if (!now) return 1;
	switch (now->ntyp) {
	case 'c':	return any_oper(now->lft, RUN);
	case ASSERT:
	case PRINT:	return any_oper(now, RUN);

	case PRINTM:
	case   '.':
	case  GOTO:
	case  ELSE:
	case BREAK:	return 0;
	default:	return 1;
	}
}

int
any_oper(Lextok *now, int oper)
{	/* check if an expression contains oper operator */
	if (!now) return 0;
	if (now->ntyp == oper)
		return 1;
	return (any_oper(now->lft, oper) || any_oper(now->rgt, oper));
}

static void
check_proc(Lextok *now, int m)
{
	if (!now)
		return;
	if (now->ntyp == '@' || now->ntyp == RUN)
	{	fprintf(fd_tb, ";\n\t\t");
		undostmnt(now, m);
	}
	check_proc(now->lft, m);
	check_proc(now->rgt, m);
}

void
genunio(void)
{	char buf1[256];
	Queue *q; int i;

	ntimes(fd_tc, 0, 1, R13_);
	for (q = qtab; q; q = q->nxt)
	{	fprintf(fd_tc, "\tcase %d:\n", q->qid);

		if (has_sorted)
		{	sprintf(buf1, "((Q%d *)z)->contents", q->qid);
			fprintf(fd_tc, "#ifdef HAS_SORTED\n");
			fprintf(fd_tc, "\t\tj = trpt->ipt;\n");	/* ipt was bup.oval */
			fprintf(fd_tc, "#endif\n");
			fprintf(fd_tc, "\t\tfor (k = j; k < ((Q%d *)z)->Qlen; k++)\n",
				q->qid);
			fprintf(fd_tc, "\t\t{\n");
			for (i = 0; i < q->nflds; i++)
			fprintf(fd_tc, "\t\t\t%s[k].fld%d = %s[k+1].fld%d;\n",
				buf1, i, buf1, i);
			fprintf(fd_tc, "\t\t}\n");
			fprintf(fd_tc, "\t\tj = ((Q0 *)z)->Qlen;\n");
		}

		sprintf(buf1, "((Q%d *)z)->contents[j].fld", q->qid);
		for (i = 0; i < q->nflds; i++)
			fprintf(fd_tc, "\t\t%s%d = 0;\n", buf1, i);
		if (q->nslots==0)
		{	/* check if rendezvous succeeded, 1 level down */
			fprintf(fd_tc, "\t\t_m = (trpt+1)->o_m;\n");
			fprintf(fd_tc, "\t\tif (_m) (trpt-1)->o_pm |= 1;\n");
			fprintf(fd_tc, "\t\tUnBlock;\n");
		} else
			fprintf(fd_tc, "\t\t_m = trpt->o_m;\n");

		fprintf(fd_tc, "\t\tbreak;\n");
	}
	ntimes(fd_tc, 0, 1, R14_);
	for (q = qtab; q; q = q->nxt)
	{	sprintf(buf1, "((Q%d *)z)->contents", q->qid);
		fprintf(fd_tc, "	case %d:\n", q->qid);
		if (q->nslots == 0)
			fprintf(fd_tc, "\t\tif (strt) boq = from+1;\n");
		else if (q->nslots > 1)	/* shift */
		{	fprintf(fd_tc, "\t\tif (strt && slot<%d)\n",
							q->nslots-1);
			fprintf(fd_tc, "\t\t{\tfor (j--; j>=slot; j--)\n");
			fprintf(fd_tc, "\t\t\t{");
			for (i = 0; i < q->nflds; i++)
			{	fprintf(fd_tc, "\t%s[j+1].fld%d =\n\t\t\t",
							buf1, i);
				fprintf(fd_tc, "\t%s[j].fld%d;\n\t\t\t",
							buf1, i);
			}
			fprintf(fd_tc, "}\n\t\t}\n");
		}
		strcat(buf1, "[slot].fld");
		fprintf(fd_tc, "\t\tif (strt) {\n");
		for (i = 0; i < q->nflds; i++)
			fprintf(fd_tc, "\t\t\t%s%d = 0;\n", buf1, i);
		fprintf(fd_tc, "\t\t}\n");
		if (q->nflds == 1)	/* set */
			fprintf(fd_tc, "\t\tif (fld == 0) %s0 = fldvar;\n",
							buf1);
		else
		{	fprintf(fd_tc, "\t\tswitch (fld) {\n");
			for (i = 0; i < q->nflds; i++)
			{	fprintf(fd_tc, "\t\tcase %d:\t%s", i, buf1);
				fprintf(fd_tc, "%d = fldvar; break;\n", i);
			}
			fprintf(fd_tc, "\t\t}\n");
		}
		fprintf(fd_tc, "\t\tbreak;\n");
	}
	ntimes(fd_tc, 0, 1, R15_);
}

extern void explain(int);

int
proper_enabler(Lextok *n)
{
	if (!n) return 1;
	switch (n->ntyp) {
	case NEMPTY:	case FULL:
	case NFULL:	case EMPTY:
	case LEN:	case 'R':
	case NAME:
		has_provided = 1;
		if (strcmp(n->sym->name, "_pid") == 0
		||  strcmp(n->sym->name, "_priority") == 0)
			return 1;
		return (!(n->sym->context));

	case C_EXPR:
	case CONST:
	case TIMEOUT:
		has_provided = 1;
		return 1;

	case ENABLED:	case PC_VAL:
	case GET_P:	/* not SET_P */
		return proper_enabler(n->lft);

	case '!': case UMIN: case '~':
		return proper_enabler(n->lft);

	case '/': case '*': case '-': case '+':
	case '%': case LT:  case GT: case '&': case '^':
	case '|': case LE:  case GE:  case NE: case '?':
	case EQ:  case OR:  case AND: case LSHIFT:
	case RSHIFT: case 'c': /* case ',': */
		return proper_enabler(n->lft) && proper_enabler(n->rgt);

	default:
		break;
	}
	printf("spin: saw ");
	explain(n->ntyp);
	printf("\n");
	return 0;
}
