#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"

static char *binop[] =
{
	[OMUL]	"*",
	[ODIV]	"/",
	[OMOD]	"%",
	[OADD]	"+",
	[OSUB]	"-",
	[ORSH]	">>",
	[OLSH]	"<<",
	[OLT]	"<",
	[OGT]	">",
	[OLEQ]	"<=",
	[OGEQ]	">=",
	[OEQ]	"==",
	[ONEQ]	"!=",
	[OLAND]	"&",
	[OXOR]	"^",
	[OLOR]	"|",
	[OCAND]	"&&",
	[OCOR]	"||",
	[OASGN]	" = "
};

static char *tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
static char *type[] =
{
	[TINT]		"integer",
	[TFLOAT]	"float",
	[TSTRING]	"string",
	[TLIST]		"list",
};

void
whatis(Lsym *l)
{
	int t;
	int def;
	Type *ti;

	def = 0;
	if(l->v->set) {
		t = l->v->type;
		Bprint(bout, "%s variable", type[t]);
		if(t == TINT || t == TFLOAT)
			Bprint(bout, " format %c", l->v->fmt);
		Bputc(bout, '\n');
		def = 1;
	}
	if(l->lt) {
		Bprint(bout, "complex {\n");
		for(ti = l->lt; ti; ti = ti->next)
			Bprint(bout, "\t%s %s\n", ti->type, ti->name);
		Bprint(bout, "}\n");
		def = 1;
	}
	if(l->proc) {
		Bprint(bout, "defn %s(", l->name);
		pexpr(l->proc->left);
		Bprint(bout, ")\n{\n");
		pcode(l->proc->right, 1);
		Bprint(bout, "}\n");
		def = 1;
	}
	if(l->builtin) {
		Bprint(bout, "builtin function\n");
		def = 1;
	}
	if(def == 0)
		Bprint(bout, "%s is undefined\n", l->name);
}

void
slist(Node *n, int d)
{
	if(n == 0)
		return;
	if(n->op == OLIST)
		Bprint(bout, "%.*s{\n", d-1, tabs);
	pcode(n, d);
	if(n->op == OLIST)
		Bprint(bout, "%.*s}\n", d-1, tabs);
}

void
pcode(Node *n, int d)
{
	Node *r, *l;

	if(n == 0)
		return;

	r = n->right;
	l = n->left;

	switch(n->op) {
	default:
		Bprint(bout, "%.*s", d, tabs);
		pexpr(n);
		Bprint(bout, ";\n");
		break;
	case OLIST:
		pcode(n->left, d);
		pcode(n->right, d);
		break;
	case OLOCAL:
		Bprint(bout, "%.*slocal %s;\n", d, tabs, n->sym->name);
		break;
	case OCOMPLEX:
		Bprint(bout, "%.*scomplex %s %s;\n", d, tabs, n->sym->name, l->sym->name);
		break;
	case OIF:
		Bprint(bout, "%.*sif ", d, tabs);
		pexpr(l);
		d++;
		Bprint(bout, " then\n", d, tabs);
		if(r && r->op == OELSE) {
			slist(r->left, d);
			Bprint(bout, "%.*selse\n", d-1, tabs, d, tabs);
			slist(r->right, d);
		}
		else
			slist(r, d);
		break;
	case OWHILE:
		Bprint(bout, "%.*swhile ", d, tabs);
		pexpr(l);
		d++;
		Bprint(bout, " do\n", d, tabs);
		slist(r, d);
		break;
	case ORET:
		Bprint(bout, "%.*sreturn ", d, tabs);
		pexpr(l);
		Bprint(bout, ";\n");
		break;
	case ODO:
		Bprint(bout, "%.*sloop ", d, tabs);
		pexpr(l->left);
		Bprint(bout, ", ");
		pexpr(l->right);
		Bprint(bout, " do\n");
		slist(r, d+1);
	}
}

void
pexpr(Node *n)
{
	Node *r, *l;

	if(n == 0)
		return;

	r = n->right;
	l = n->left;

	switch(n->op) {
	case ONAME:
		Bprint(bout, "%s", n->sym->name);
		break;
	case OCONST:
		switch(n->type) {
		case TINT:
			Bprint(bout, "%d", n->ival);
			break;
		case TFLOAT:
			Bprint(bout, "%g", n->fval);
			break;
		case TSTRING:
			pstr(n->string);
			break;
		case TLIST:
			break;
		}
		break;
	case OMUL:
	case ODIV:
	case OMOD:
	case OADD:
	case OSUB:
	case ORSH:
	case OLSH:
	case OLT:
	case OGT:
	case OLEQ:
	case OGEQ:
	case OEQ:
	case ONEQ:
	case OLAND:
	case OXOR:
	case OLOR:
	case OCAND:
	case OCOR:
	case OASGN:
		pexpr(l);
		Bprint(bout, binop[n->op]);
		pexpr(r);
		break;
	case OINDM:
		Bprint(bout, "*");
		pexpr(l);
		break;
	case OEDEC:
		Bprint(bout, "--");
		pexpr(l);
		break;
	case OEINC:
		Bprint(bout, "++");
		pexpr(l);
		break;
	case OPINC:
		pexpr(l);
		Bprint(bout, "++");
		break;
	case OPDEC:
		pexpr(l);
		Bprint(bout, "--");
		break;
	case ONOT:
		Bprint(bout, "!");
		pexpr(l);
		break;
	case OLIST:
		pexpr(l);
		if(r) {
			Bprint(bout, ",");
			pexpr(r);
		}
		break;
	case OCALL:
		pexpr(l);
		Bprint(bout, "(");
		pexpr(r);
		Bprint(bout, ")");
		break;
	case OCTRUCT:
		Bprint(bout, "{");
		pexpr(l);
		Bprint(bout, "}");
		break;
	case OHEAD:
		Bprint(bout, "head ");
		pexpr(l);
		break;
	case OTAIL:
		Bprint(bout, "tail ");
		pexpr(l);
		break;
	case OAPPEND:
		Bprint(bout, "append ");
		pexpr(l);
		Bprint(bout, ",");
		pexpr(r);
		break;
	case ORET:
		Bprint(bout, "return ");
		pexpr(l);
		break;
	case OINDEX:
		pexpr(l);
		Bprint(bout, "[");
		pexpr(r);
		Bprint(bout, "]");
		break;
	case OINDC:
		Bprint(bout, " @");
		pexpr(l);
		break;
	case ODOT:
		fatal("dot");
	case OFRAME:
		Bprint(bout, "%s:%s", n->sym->name, r->sym->name);
		break;
	}
}

void
pstr(String *s)
{
	int i, c;

	Bputc(bout, '"');
	for(i = 0; i < s->len; i++) {
		c = s->string[i];
		switch(c) {
		case '\0':
			c = '0';
			break;
		case '\n':
			c = 'n';
			break;
		case '\r':
			c = 'r';
			break;
		case '\t':
			c = 't';
			break;
		case '\b':
			c = 'b';
			break;
		case '\f':
			c = 'f';
			break;
		case '\a':
			c = 'a';
			break;
		case '\v':
			c = 'v';
			break;
		case '\\':
			c = '\\';
			break;
		case '"':
			c = '"';
			break;
		default:
			Bputc(bout, c);
			continue;
		}
		Bputc(bout, '\\');
		Bputc(bout, c);
	}
	Bputc(bout, '"');
}
