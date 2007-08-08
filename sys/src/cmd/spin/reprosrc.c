/***** spin: reprosrc.c *****/

/* Copyright (c) 2002-2003 by Lucent Technologies, Bell Laboratories.     */
/* All Rights Reserved.  This software is for educational purposes only.  */
/* No guarantee whatsoever is expressed or implied by the distribution of */
/* this code.  Permission is given to distribute this code provided that  */
/* this introductory message is not removed and no monies are exchanged.  */
/* Software written by Gerard J. Holzmann.  For tool documentation see:   */
/*             http://spinroot.com/                                       */
/* Send all bug-reports and/or questions to: bugs@spinroot.com            */

#include <stdio.h>
#include "spin.h"
#include "y.tab.h"

static int indent = 1;

extern ProcList	*rdy;
void repro_seq(Sequence *);

void
doindent(void)
{	int i;
	for (i = 0; i < indent; i++)
		printf("   ");
}

void
repro_sub(Element *e)
{
	doindent();
	switch (e->n->ntyp) {
	case D_STEP:
		printf("d_step {\n");
		break;
	case ATOMIC:
		printf("atomic {\n");
		break;
	case NON_ATOMIC:
		printf(" {\n");
		break;
	}
	indent++;
	repro_seq(e->n->sl->this);
	indent--;

	doindent();
	printf(" };\n");
}

void
repro_seq(Sequence *s)
{	Element *e;
	Symbol *v;
	SeqList *h;

	for (e = s->frst; e; e = e->nxt)
	{
		v = has_lab(e, 0);
		if (v) printf("%s:\n", v->name);

		if (e->n->ntyp == UNLESS)
		{	printf("/* normal */ {\n");
			repro_seq(e->n->sl->this);
			doindent();
			printf("} unless {\n");
			repro_seq(e->n->sl->nxt->this);
			doindent();
			printf("}; /* end unless */\n");
		} else if (e->sub)
		{
			switch (e->n->ntyp) {
			case DO: doindent(); printf("do\n"); indent++; break;
			case IF: doindent(); printf("if\n"); indent++; break;
			}

			for (h = e->sub; h; h = h->nxt)
			{	indent--; doindent(); indent++; printf("::\n");
				repro_seq(h->this);
				printf("\n");
			}

			switch (e->n->ntyp) {
			case DO: indent--; doindent(); printf("od;\n"); break;
			case IF: indent--; doindent(); printf("fi;\n"); break;
			}
		} else
		{	if (e->n->ntyp == ATOMIC
			||  e->n->ntyp == D_STEP
			||  e->n->ntyp == NON_ATOMIC)
				repro_sub(e);
			else if (e->n->ntyp != '.'
			     &&  e->n->ntyp != '@'
			     &&  e->n->ntyp != BREAK)
			{
				doindent();
				if (e->n->ntyp == C_CODE)
				{	printf("c_code ");
					plunk_inline(stdout, e->n->sym->name, 1);
				} else if (e->n->ntyp == 'c'
				       &&  e->n->lft->ntyp == C_EXPR)
				{	printf("c_expr { ");
					plunk_expr(stdout, e->n->lft->sym->name);
					printf("} ->\n");
				} else
				{	comment(stdout, e->n, 0);
					printf(";\n");
			}	}
		}
		if (e == s->last)
			break;
	}
}

void
repro_proc(ProcList *p)
{
	if (!p) return;
	if (p->nxt) repro_proc(p->nxt);

	if (p->det) printf("D");	/* deterministic */
	printf("proctype %s()", p->n->name);
	if (p->prov)
	{	printf(" provided ");
		comment(stdout, p->prov, 0);
	}
	printf("\n{\n");
	repro_seq(p->s);
	printf("}\n");
}

void
repro_src(void)
{
	repro_proc(rdy);
}
