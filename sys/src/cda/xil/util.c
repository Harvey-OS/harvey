#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "symbols.h"
#include "y.tab.h"

#define NSYM	10000	/* per file */
#define NDEP	50	/* per line */
Sym symtab[NSYM];
Sym *dep[NDEP];
Tree *Root;
int ndep;

Tree *
constnode(int i)
{
	Tree *t;
	t = (Tree *) calloc(1,sizeof(Tree));
	t->op = i ? C1 : C0;
	t->val = i;
	t->id = lookup(i ? "VCC" : "GND");
	return t;
}

#define BUCK	137
Tree *bucket[BUCK];
#define max(a,b)	((a > b) ? a : b)

Tree *
uniq(Tree *t)
{
	int hash,op;
	Tree *s,*l,*r=0;
	switch (op = t->op) {
	case C0:
	case C1:
	case ID:
		return t;
	case xor:
	case or:
	case and:
		t->right = uniq(t->right);
		r = t->right;
	case not:
		t->left = uniq(t->left);
		l = t->left;
		hash = ((op*(ulong)l)^((ulong)r>>5))%BUCK;
		for (s = bucket[hash]; s; s = s->next)
			if (s->op == op && s->left == l && s->right == r)
				return s;
		goto cleanup;
	default:
		yyerror("uniq: unexpected op %d",t->op);
		return t;
	}
cleanup:
	t->next = bucket[hash];
	bucket[hash] = t;
	return t;
}

Tree *
idnode(Sym *s, int bar)
{
	Tree *t;
	int hash;
	if (bar)
		return notnode(idnode(s, 0));
	hash = (ID*(ulong)s)%BUCK;
	for (t = bucket[hash]; t; t = t->next)
		if (t->op == ID && t->id == s)
			return t;
	t = (Tree *) calloc(1,sizeof(Tree));
	t->op = ID;
	t->id = s;
	t->next = bucket[hash];
	bucket[hash] = t;
	return t;
}

Tree *
nameit(Sym *s, Tree *t)
{
	t->id = s;
	return t;
}

Tree *
opnode(int op, Tree *l, Tree *r)
{
	Tree *t;
	t = (Tree *) calloc(1,sizeof(Tree));
	t->op = op;
	t->mask = l->mask;
	if (r)
		t->mask |= r->mask;
	t->vis = 1;			/* deprecate self */
	if (nbits(t->mask) <= K) {	/* promote kids */
		l->vis = 0;
		if (r)
			r->vis = 0;
	}
	t->left = l;
	t->right = r;
	return t;
}

Tree *
muxnode(Tree *s, Tree *l, Tree *r)
{
	return ornode(andnode(notnode(s),l),andnode(s,r));
}

char *opname[] = {
	[C0]		"C0",
	[C1]		"C1",
	[ID]		"ID",
	[not]		"not",
	[and]		"and",
	[or]		"or",
	[xor]		"xor",
};

void
outbin(ulong m)
{
	char *p, buf[40];
	for (p = buf; m; m >>= 1)
		*p++ = (m&1) + '0';
	*p = 0;
	fprintf(stdout,buf);
}

void
prtree(Tree *t)
{
	if (t == 0)
		return;
	if (vflag)
		outbin(t->mask);
#define BRACKET
#ifdef BRACKET
	if (t->vis) {
		fprintf(stdout,"[");
		t->vis = 0;
		prtree(t);
		t->vis = 1;
		fprintf(stdout,"]");
		return;
	}
#else
	if (t->invis)
		fprintf(stdout,".");
#endif
	switch (t->op) {
	case not:
		fprintf(stdout,"!");
		prtree(t->left);
		return;
	case ID:
		fprintf(stdout,t->id->name);
		if (t->id->pin)
			fprintf(stdout, ".%s", t->id->pin);
		return;
	case C0:
	case C1:
		fprintf(stdout,"%d",t->val);
		return;
	default:
		fprintf(stdout,"%s(", opname[t->op]);
		prtree(t->left);
		fprintf(stdout,",");
		prtree(t->right);
		fprintf(stdout,")");
		return;
	}
}

Sym *
lookup(char *name)
{
	Sym *s;
	for (s = symtab; s < &symtab[NSYM] && s->name; s++)
		if (strcmp(name,s->name) == 0)
			return s;
	if (s == &symtab[NSYM])
		yyerror("author error, insufficient symtab space");
	s->name = (char *) malloc(strlen(name)+1);
	strcpy(s->name,name);
	return s;
}

void
definst(Sym *m, int n, Sym *i)
{
	Sym *s;
	int hard = 'h' == m->name[strlen(m->name)-1];
	for (s = i; s < i+n; s++) {	/* reserve n slots */
		s->name = i->name;
		s->inst = -1;
		s->hard = hard;
	}
	m->inst = n;
}

Sym *
looktup(char *name, char *pin)
{
	Sym *s;
	for (s = symtab; s < &symtab[NSYM] && s->name && strcmp(name, s->name); s++)
		;
	if (s == &symtab[NSYM] || !s->name)
		yyerror("bug, insufficient space or missing master");
	for (; s < &symtab[NSYM] && s->pin; s++)
		if (strcmp(pin, s->pin) == 0)
			return s;
	if (!s->name || strcmp(s->name, name))
		yyerror("bug, ran out of inst pins");
	s->pin = (char *) malloc(strlen(pin)+1);
	strcpy(s->pin,pin);
	return s;
}
