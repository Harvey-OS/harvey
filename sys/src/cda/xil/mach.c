#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>
#include "symbols.h"
#include "dat.h"

void
apply(void (*f)(Sym *))
{
	Sym *s;
	for (s = symtab; s->name; s++)
		(*f)(s);
}

void
merge(Sym *s)
{
	Tree *t;
	if (s->internal && !s->clk && !s->inst && (t = s->exp))
		if (t->id) {
			s->lval = s->rval = 0;
			s->name = t->id->name;
		}
		else
			t->id = s;
	if ((t = s->clk) && t->op == ID)
		t->id->clock = 1;
}

Tree *
flatten(Tree *t)
{
	static gensym;
	Sym *s;
	Tree *v;
	char buf[10];
	v = 0;
	if (t == 0)
		return t;
	if (t->vis) {
		sprint(buf, "G%3.3d", gensym++);
		s = lookup(buf);
		s->exp = t;
		s->internal = 1;
		s->lval = 1;
		s->rval = 1;
		v = idnode(s, 0);
	}
	switch (t->op) {
	case C0:
	case C1:
	case ID:
		return t;
	case xor:
	case and:
	case or:
		t->right = flatten(t->right);
	case not:
		t->left = flatten(t->left);
		return t->vis ? v : t;
	default:
		yyerror("flatten: unknown op %d", t->op);
	}
	return t;
}

Sym *wall;

void
dovis(Sym *s)
{
	Tree **tp;
	for (tp = &s->exp; tp <= &s->pre; tp++)
		if (*tp)
			markvis(*tp);
	wall = s;
}

int gsym;

char *xname[] = {
	"0",
	"1",
	"ID",
	"inv",
	"and",
	"or",
	"xor",
};

int
pinconv(void *o, Fconv *fp)
{
	Sym *s;
	char buf[20];
	char *p, *q;
	int c;
	s = *((Sym **) o);
	if (s->hard) {
		for (p = s->pin, q = buf; (c = *p) && !isdigit(c); p++)
			*q++ = c;
		if (isdigit(c))
			sprint(q, "<%s>", p);
		else
			*q = 0;
	}
	else
		sprint(buf, "%s", s->pin);
	strconv(buf, fp);
	return sizeof(Sym *);
}

int
treeconv(void *o, Fconv *fp)
{
	Sym *s;
	Tree *t;
	char buf[1000], *bp=buf;
	char prefix[10];
	t = *((Tree **) o);
	if (t->op < not) {
		s = t->id;
		bp += sprint(bp, "%s", s->name);
		if (s->pin)
			sprint(bp, "_%P", s);
		strconv(buf, fp);
		return sizeof(Tree *);
	}
	else if (t->op == not && t->left->op == ID) {
		s = t->left->id;
		bp += sprint(bp, "%s,, INV", s->name);
		if (s->pin)
			sprint(bp, "_%P", s);
		strconv(buf, fp);
		return sizeof(Tree *);
	}
	if (!t->id) {
		sprint(buf, "T%3.3d", gsym++);
		t->id = lookup(buf);
		t->id->internal = 1;
	}
	sprint(prefix, t->id->internal ? "" : "x_");
	bp += sprint(bp, "sym, c_%s, %s\n", t->id->name, xname[t->op]);
	bp += sprint(bp, "pin, o, o, %s%s\n", prefix, t->id->name);
	switch (t->op) {
	case and:
	case or:
	case xor:
		bp += sprint(bp, "pin, i0, i, %T\n", t->right);
		bp += sprint(bp, "pin, i1, i, %T\n", t->left);
		break;
	case not:
		bp += sprint(bp, "pin, i, i, %T\n", t->left);
		break;
	default:
		bp += sprint(bp, "pin, op=%s\n", xname[t->op]);
		break;
	}
	sprint(bp, "end\n");
	print("%s", buf);
	if (t->id->name) {
		sprint(buf, "%s%s", prefix, t->id->name);
		strconv(buf, fp);
	}
	return sizeof(Tree *);
}

enum Iot {
	NONE,
	IN,
	OUT,
	BI,
	TRI
};

void
outsym(Sym *s)
{
	int i, n;
	char buf[4096], *bp=buf;
	int iot=NONE;
	if (n = s->inst) {
		if (n < 1)
			return;
		bp += sprint(bp, "sym, c_%s, %s%s\n", s[1].name, s[0].name, s[1].hard ? ", def=hm" : "");
		for (s++, i = 0; i < n; i++, s++) {
			bp += sprint(bp, "pin, %P, ", s);
			if (s->lval)
				bp += sprint(bp, "i, %T\n", s->exp);
			else
				bp += sprint(bp, "o, %s_%P\n", s->name, s);
		}
		sprint(bp, "end\n");
		print(buf);
		return;
	}
	if (!s->lval && !s->rval)
		return;
	if (!s->internal) {		/* goes to the outside, classify */
		if (s->lval)
			iot = s->rval ? BI : s->ena ? TRI : OUT;
		else
			iot = IN;
		bp += sprint(bp, "ext, p_%s, %c\n", s->name, "xiobt"[iot]);
	}
	if (iot == IN || iot == BI) {
		bp += sprint(bp, "sym, i_%s, %s\n", s->name, s->clock ? "bufgp" : "ibuf");
		bp += sprint(bp, "pin, o, o, %s\n", s->name);
		bp += sprint(bp, "pin, i, i, p_%s\n", s->name);
		bp += sprint(bp, "end\n");
	}
	if (s->clk) {
		bp  += sprint(bp, "sym, c_%s, %s%s\n", s->name, s->internal ? "dff" : "outff", s->ena ? "t" : "");
		if (s->internal)
			bp += sprint(bp, "pin, q, o, %s\n", s->name);
		else if (s->ena) {
			bp += sprint(bp, "pin, o, o, %s_%s\n", s->internal ? "" : "p", s->name);
			bp += sprint(bp, "pin, t, i, %T\n", s->ena);
		}
		else
			bp += sprint(bp, "pin, q, o, %s_%s\n", s->internal ? "" : "p", s->name);
		if (s->exp)
			bp += sprint(bp, "pin, d, i, %T\n", s->exp);
		bp += sprint(bp, "pin, c, i, %T\n", s->clk);
		if (s->cke && s->internal)	/* bug - shouldn't be silent */
			bp += sprint(bp, "pin, ce, i, %T\n", s->cke);
		if (s->rd && s->internal)
			bp += sprint(bp, "pin, rd, i, %T\n", s->rd);
		if (s->pre && s->internal)
			bp += sprint(bp, "pin, sd, i, %T\n", s->pre);
		bp += sprint(bp, "end\n");
	}
	else if (s->exp) {
		if (s->exp->id == 0)
			s->exp->id = s;
		if (!s->internal) {
			bp += sprint(bp, "sym, o_%s, obuf%s\n", s->name, (s->ena) ? "t" : "");
			bp += sprint(bp, "pin, o, o, p_%s\n", s->name);
			bp += sprint(bp, "pin, i, i, %T\n", s->exp);
			if (s->ena)
				bp += sprint(bp, "pin, t, i, %T\n", s->ena);
			bp += sprint(bp, "end\n");
		}
		else
			bp += sprint(bp, "%T", s->exp);
	}
	if (bp-buf > 20)
		print(buf);
}

void
prsym(Sym *s)
{
	int i, n;
	if (s < wall && fflag)
		flatten(s->exp);
	if (n = s->inst) {
		if (n < 1)
			return;
		for (s++, i = 0; i < n; i++, s++)
			if (s->lval) {
				fprintf(stdout, "%s.%s = ", s->name, s->pin);
				prtree(s->exp);
				fprintf(stdout, "\n");
			}
	}
	if (!s->lval)
		return;
	fprintf(stdout,"%s = ",s->name);
	prtree(s->exp);
	fprintf(stdout,"\n");
}
