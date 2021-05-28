/***** spin: reprosrc.c *****/

/*
 * This file is part of the public release of Spin. It is subject to the
 * terms in the LICENSE file that is included in this source directory.
 * Tool documentation is available at http://spinroot.com
 */

#include <stdio.h>
#include <assert.h>
#include "spin.h"
#include "y.tab.h"

static int indent = 1;

extern YYSTYPE yylval;
extern ProcList	*ready;
static void repro_seq(Sequence *);

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

static void
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
					plunk_inline(stdout, e->n->sym->name, 1, 1);
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
	repro_proc(ready);
}

static int in_decl;
static int in_c_decl;
static int in_c_code;

void
blip(int n, char *b)
{	char mtxt[1024];

	strcpy(mtxt, "");

	switch (n) {
	default:	if (n > 0 && n < 256)
				sprintf(mtxt, "%c", n);
			else
				sprintf(mtxt, "<%d?>", n);

			break;
	case '(':	strcpy(mtxt, "("); in_decl++; break;
	case ')':	strcpy(mtxt, ")"); in_decl--; break;
	case '{':	strcpy(mtxt, "{"); break;
	case '}':	strcpy(mtxt, "}"); break;
	case '\t':	sprintf(mtxt, "\\t"); break;
	case '\f':	sprintf(mtxt, "\\f"); break;
	case '\n':	sprintf(mtxt, "\\n"); break;
	case '\r':	sprintf(mtxt, "\\r"); break;
	case 'c':	sprintf(mtxt, "condition"); break;
	case 's':	sprintf(mtxt, "send"); break;
	case 'r':	sprintf(mtxt, "recv"); break;
	case 'R':	sprintf(mtxt, "recv poll"); break;
	case '@':	sprintf(mtxt, "@"); break;
	case '?':	sprintf(mtxt, "(x->y:z)"); break;
	case NEXT:	sprintf(mtxt, "X"); break;
	case ALWAYS:	sprintf(mtxt, "[]"); break;
	case EVENTUALLY: sprintf(mtxt, "<>"); break;
	case IMPLIES:	sprintf(mtxt, "->"); break;
	case EQUIV:	sprintf(mtxt, "<->"); break;
	case UNTIL:	sprintf(mtxt, "U"); break;
	case WEAK_UNTIL: sprintf(mtxt, "W"); break;
	case IN: sprintf(mtxt, "in"); break;
	case ACTIVE:	sprintf(mtxt, "active"); break;
	case AND:	sprintf(mtxt, "&&"); break;
	case ARROW:	sprintf(mtxt, "->"); break;
	case ASGN:	sprintf(mtxt, "="); break;
	case ASSERT:	sprintf(mtxt, "assert"); break;
	case ATOMIC:	sprintf(mtxt, "atomic"); break;
	case BREAK:	sprintf(mtxt, "break"); break;
	case C_CODE:	sprintf(mtxt, "c_code"); in_c_code++; break;
	case C_DECL:	sprintf(mtxt, "c_decl"); in_c_decl++; break;
	case C_EXPR:	sprintf(mtxt, "c_expr"); break;
	case C_STATE:	sprintf(mtxt, "c_state"); break;
	case C_TRACK:	sprintf(mtxt, "c_track"); break;
	case CLAIM:	sprintf(mtxt, "never"); break;
	case CONST:	sprintf(mtxt, "%d", yylval->val); break;
	case DECR:	sprintf(mtxt, "--"); break;
	case D_STEP:	sprintf(mtxt, "d_step"); break;
	case D_PROCTYPE: sprintf(mtxt, "d_proctype"); break;
	case DO:	sprintf(mtxt, "do"); break;
	case DOT:	sprintf(mtxt, "."); break;
	case ELSE:	sprintf(mtxt, "else"); break;
	case EMPTY:	sprintf(mtxt, "empty"); break;
	case ENABLED:	sprintf(mtxt, "enabled"); break;
	case EQ:	sprintf(mtxt, "=="); break;
	case EVAL:	sprintf(mtxt, "eval"); break;
	case FI:	sprintf(mtxt, "fi"); break;
	case FOR:	sprintf(mtxt, "for"); break;
	case FULL:	sprintf(mtxt, "full"); break;
	case GE:	sprintf(mtxt, ">="); break;
	case GET_P:	sprintf(mtxt, "get_priority"); break;
	case GOTO:	sprintf(mtxt, "goto"); break;
	case GT:	sprintf(mtxt, ">"); break;
	case HIDDEN:	sprintf(mtxt, "hidden"); break;
	case IF:	sprintf(mtxt, "if"); break;
	case INCR:	sprintf(mtxt, "++"); break;

	case INLINE:	sprintf(mtxt, "inline"); break;
	case INIT:	sprintf(mtxt, "init"); break;
	case ISLOCAL:	sprintf(mtxt, "local"); break;

	case LABEL:	sprintf(mtxt, "<label-name>"); break;

	case LE:	sprintf(mtxt, "<="); break;
	case LEN:	sprintf(mtxt, "len"); break;
	case LSHIFT:	sprintf(mtxt, "<<"); break;
	case LT:	sprintf(mtxt, "<"); break;
	case LTL:	sprintf(mtxt, "ltl"); break;

	case NAME:	sprintf(mtxt, "%s", yylval->sym->name); break;

	case XU:	switch (yylval->val) {
			case XR:	sprintf(mtxt, "xr"); break;
			case XS:	sprintf(mtxt, "xs"); break;
			default:	sprintf(mtxt, "<?>"); break;
			}
			break;

	case TYPE:	switch (yylval->val) {
			case BIT:	sprintf(mtxt, "bit"); break;
			case BYTE:	sprintf(mtxt, "byte"); break;
			case CHAN:	sprintf(mtxt, "chan"); in_decl++; break;
			case INT:	sprintf(mtxt, "int"); break;
			case MTYPE:	sprintf(mtxt, "mtype"); break;
			case SHORT:	sprintf(mtxt, "short"); break;
			case UNSIGNED:	sprintf(mtxt, "unsigned"); break;
			default:	sprintf(mtxt, "<unknown type>"); break;
			}
			break;

	case NE:	sprintf(mtxt, "!="); break;
	case NEG:	sprintf(mtxt, "!"); break;
	case NEMPTY:	sprintf(mtxt, "nempty"); break;
	case NFULL:	sprintf(mtxt, "nfull"); break;

	case NON_ATOMIC: sprintf(mtxt, "<sub-sequence>"); break;

	case NONPROGRESS: sprintf(mtxt, "np_"); break;
	case OD:	sprintf(mtxt, "od"); break;
	case OF:	sprintf(mtxt, "of"); break;
	case OR:	sprintf(mtxt, "||"); break;
	case O_SND:	sprintf(mtxt, "!!"); break;
	case PC_VAL:	sprintf(mtxt, "pc_value"); break;
	case PRINT:	sprintf(mtxt, "printf"); break;
	case PRINTM:	sprintf(mtxt, "printm"); break;
	case PRIORITY:	sprintf(mtxt, "priority"); break;
	case PROCTYPE:	sprintf(mtxt, "proctype"); break;
	case PROVIDED:	sprintf(mtxt, "provided"); break;
	case RETURN:	sprintf(mtxt, "return"); break;
	case RCV:	sprintf(mtxt, "?"); break;
	case R_RCV:	sprintf(mtxt, "??"); break;
	case RSHIFT:	sprintf(mtxt, ">>"); break;
	case RUN:	sprintf(mtxt, "run"); break;
	case SEP:	sprintf(mtxt, "::"); break;
	case SEMI:	sprintf(mtxt, ";"); break;
	case SET_P:	sprintf(mtxt, "set_priority"); break;
	case SHOW:	sprintf(mtxt, "show"); break;
	case SND:	sprintf(mtxt, "!"); break;

	case INAME:
	case UNAME:
	case PNAME:
	case STRING:	sprintf(mtxt, "%s", yylval->sym->name); break;

	case TRACE:	sprintf(mtxt, "trace"); break;
	case TIMEOUT:	sprintf(mtxt, "(timeout)"); break;
	case TYPEDEF:	sprintf(mtxt, "typedef"); break;
	case UMIN:	sprintf(mtxt, "-"); break;
	case UNLESS:	sprintf(mtxt, "unless"); break;
	}
	strcat(b, mtxt);
}

void
purge(char *b)
{
	if (strlen(b) == 0) return;

	if (b[strlen(b)-1] != ':')	/* label? */
	{	if (b[0] == ':' && b[1] == ':')
		{	indent--;
			doindent();
			indent++;
		} else
		{	doindent();
		}
	}
	printf("%s\n", b);
	strcpy(b, "");

	in_decl = 0;
	in_c_code = 0;
	in_c_decl = 0;
}

int pp_mode;
extern int lex(void);

void
pretty_print(void)
{	int c, lastc = 0;
	char buf[1024];

	pp_mode = 1;
	indent = 0;
	strcpy(buf, "");
	while ((c = lex()) != EOF)
	{
		if ((lastc == IF || lastc == DO) && c != SEP)
		{	indent--;	/* c_code if */
		}
		if (c == C_DECL  || c == C_STATE
		||  c == C_TRACK || c == SEP
		||  c == DO      || c == IF
		|| (c == TYPE && !in_decl))
		{	purge(buf); /* start on new line */
		}

		if (c == '{'
		&& lastc != OF && lastc != IN
		&& lastc != ATOMIC && lastc != D_STEP
		&& lastc != C_CODE && lastc != C_DECL && lastc != C_EXPR)
		{	purge(buf);
		}

		if (c == PREPROC)
		{	int oi = indent;
			purge(buf);
			assert(strlen(yylval->sym->name) < sizeof(buf));
			strcpy(buf, yylval->sym->name);
			indent = 0;
			purge(buf);
			indent = oi;
			continue;
		}

		if (c != ':' && c != SEMI
		&&  c != ',' && c != '('
		&&  c != '#' && lastc != '#'
		&&  c != ARROW && lastc != ARROW
		&&  c != '.' && lastc != '.'
		&&  c != '!' && lastc != '!'
		&&  c != SND && lastc != SND
		&&  c != RCV && lastc != RCV
		&&  c != O_SND && lastc != O_SND
		&&  c != R_RCV && lastc != R_RCV
		&&  (c != ']' || lastc != '[')
		&&  (c != '>' || lastc != '<')
		&&  (c != GT || lastc != LT)
		&&  c != '@' && lastc != '@'
		&& (lastc != '(' || c != ')')
		&& (lastc != '/' || c != '/')
		&&  c != DO && c != OD && c != IF && c != FI
		&&  c != SEP && strlen(buf) > 0)
			strcat(buf, " ");

		if (c == '}' || c == OD || c == FI)
		{	purge(buf);
			indent--;
		}
		blip(c, buf);

		if (c == '{' || c == DO || c == IF)
		{	purge(buf);
			indent++;
		}

		if (c == '}' || c == BREAK || c == SEMI || c == ELSE
		|| (c == ':' && lastc == NAME))
		{	purge(buf);
		}
		lastc = c;
	}
	purge(buf);
}
