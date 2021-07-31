/***** spin: pangen4.c *****/

/* Copyright (c) 1991,1995 by AT&T Corporation.  All Rights Reserved.     */
/* This software is for educational purposes only.                        */
/* Permission is given to distribute this code provided that this intro-  */
/* ductory message is not removed and no monies are exchanged.            */
/* No guarantee is expressed or implied by the distribution of this code. */
/* Software written by Gerard J. Holzmann as part of the book:            */
/* `Design and Validation of Computer Protocols,' ISBN 0-13-539925-4,     */
/* Prentice Hall, Englewood Cliffs, NJ, 07632.                            */
/* Send bug-reports and/or questions to: gerard@research.att.com          */

#include "spin.h"
#include "y.tab.h"

extern FILE	*tc, *tb;
extern Queue	*qtab;
extern Symbol	*Fname;
extern int	lineno, nocast;
extern char	*R13[], *R14[], *R15[];

void
undostmnt(Lextok *now, int m)
{	Lextok *v;
	int i, j; extern int m_loss;

	if (!now)
	{	fprintf(tb, "0");
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
	case '?':	case PC_VAL:
			putstmnt(tb, now, m);
			break;
	case RUN:	fprintf(tb, "delproc(0, now._nr_pr-1)");
			break;
	case 's':	if (m_loss)
			{	fprintf(tb, "if (m == 2) m = unsend");
				putname(tb, "(", now->lft, m, ")");
			} else
			{	fprintf(tb, "m = unsend");
				putname(tb, "(", now->lft, m, ")");
			}
			break;
	case 'r':	for (v = now->rgt, i=j=0; v; v = v->rgt, i++)
				if (v->lft->ntyp != CONST)
					j++;
			fprintf(tb, ";\n");
			if (j == 0)
			{	fprintf(tb, "\t\tif (q_flds");
				putname(tb, "[((Q0 *)qptr(", now->lft, m, "-1))->_t]");
				fprintf(tb, " != %d)\n", i);
			} else
				fprintf(tb, "\t\tif (1)\n");
			fprintf(tb, "#if defined(FULLSTACK) && defined(NOCOMP)\n");
			fprintf(tb, "\t\t	sv_restor(!(t->atom&2));\n");
			fprintf(tb, "#else\n");
			fprintf(tb, "\t\t	sv_restor(0);\n");
			fprintf(tb, "#endif\n\t\t");
			if (j == 0)
			{	fprintf(tb, "else {\n\t\t");
				for (v = now->rgt, i = 0; v; v = v->rgt, i++)
				{	fprintf(tb, "\tunrecv");
					putname(tb, "(", now->lft, m, ", 0, ");
					fprintf(tb, "%d, ", i);
					undostmnt(v->lft, m);
					fprintf(tb, ", %d);\n\t\t", (i==0)?1:0);
				}
				fprintf(tb, "}\n\t\t");
			}
			break;
	case '@':	fprintf(tb, "p_restor(II)");
			break;
	case ASGN:	nocast=1; putstmnt(tb,now->lft,m);
			nocast=0; fprintf(tb, " = trpt->oval");
			check_proc(now->rgt, m);
			break;
	case 'c':	check_proc(now->lft, m);
			break;
	case '.':
	case GOTO:
	case ELSE:
	case BREAK:	break;
	case ASSERT:
	case PRINT:	check_proc(now, m);
			break;
	default:	printf("spin: bad node type %d (.b)\n",
							now->ntyp);
			exit(1);
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

void
check_proc(Lextok *now, int m)
{
	if (!now)
		return;
	if (now->ntyp == '@' || now->ntyp == RUN)
	{	fprintf(tb, ";\n\t\t");
		undostmnt(now, m);
	}
	check_proc(now->lft, m);
	check_proc(now->rgt, m);
}

void
genunio(void)
{	char *buf1;
	Queue *q; int i; extern int has_sorted;

	buf1 = (char *) emalloc(128);
	ntimes(tc, 0, 1, R13);
	for (q = qtab; q; q = q->nxt)
	{	fprintf(tc, "\tcase %d:\n", q->qid);

	if (has_sorted)
	{	sprintf(buf1, "((Q%d *)z)->contents", q->qid);
		fprintf(tc, "\t\tj = trpt->oval;\n");
		fprintf(tc, "\t\tfor (k = j; k < ((Q%d *)z)->Qlen; k++)\n", q->qid);
		fprintf(tc, "\t\t{\n");
		for (i = 0; i < q->nflds; i++)
		fprintf(tc, "\t\t\t%s[k].fld%d = %s[k+1].fld%d;\n",
			buf1, i, buf1, i);
		fprintf(tc, "\t\t}\n");
		fprintf(tc, "\t\tj = ((Q0 *)z)->Qlen;\n");
	}

		sprintf(buf1, "((Q%d *)z)->contents[j].fld", q->qid);
		for (i = 0; i < q->nflds; i++)
			fprintf(tc, "\t\t%s%d = 0;\n", buf1, i);
		if (q->nslots==0)
		{	/* check if rendezvous succeeded, 1 level down */
			fprintf(tc, "\t\tm = (trpt+1)->o_m;\n");
			fprintf(tc, "\t\tif (m) trpt->o_pm |= 1;\n");
			fprintf(tc, "\t\tUnBlock;\n");
		} else
			fprintf(tc, "\t\tm = trpt->o_m;\n");
		fprintf(tc, "\t\tbreak;\n");
	}
	ntimes(tc, 0, 1, R14);
	for (q = qtab; q; q = q->nxt)
	{	sprintf(buf1, "((Q%d *)z)->contents", q->qid);
		fprintf(tc, "	case %d:\n", q->qid);
		if (q->nslots == 0)
			fprintf(tc, "\t\tif (strt) boq = from+1;\n");
		else if (q->nslots > 1)	/* shift */
		{	fprintf(tc, "\t\tif (strt && slot<%d)\n",
							q->nslots-1);
			fprintf(tc, "\t\t{\tfor (j--; j>=slot; j--)\n");
			fprintf(tc, "\t\t\t{");
			for (i = 0; i < q->nflds; i++)
			{	fprintf(tc, "\t%s[j+1].fld%d =\n\t\t\t",
							buf1, i);
				fprintf(tc, "\t%s[j].fld%d;\n\t\t\t",
							buf1, i);
			}
			fprintf(tc, "}\n\t\t}\n");
		}
		strcat(buf1, "[slot].fld");
		fprintf(tc, "\t\tif (strt) {\n");
		for (i = 0; i < q->nflds; i++)
			fprintf(tc, "\t\t\t%s%d = 0;\n", buf1, i);
		fprintf(tc, "\t\t}\n");
		if (q->nflds == 1)	/* set */
			fprintf(tc, "\t\tif (fld == 0) %s0 = fldvar;\n",
							buf1);
		else
		{	fprintf(tc, "\t\tswitch (fld) {\n");
			for (i = 0; i < q->nflds; i++)
			{	fprintf(tc, "\t\tcase %d:\t%s", i, buf1);
				fprintf(tc, "%d = fldvar; break;\n", i);
			}
			fprintf(tc, "\t\t}\n");
		}
		fprintf(tc, "\t\tbreak;\n");
	}
	ntimes(tc, 0, 1, R15);
}
