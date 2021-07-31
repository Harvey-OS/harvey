#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "symbols.h"
#include "y.tab.h"

#define NSYM	5000	/* per file */
#define NDEP	200	/* per line */
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
#define HEAVY	2
Tree *bucket[BUCK];
#define max(a,b)	((a > b) ? a : b)

void
dofanout(Tree *t, int i)
{
	t->fanout += i;
	switch (t->op) {
	case C0:
	case C1:
	case clkbuf:
		t->fanout = 0;
		break;
	case ID:
	case CK:
	case inbuf:
		return;
	case dff:
		dofanout(t->pre, HEAVY);	/* an approximation, they all differ */
	case lat:
		dofanout(t->clr, 1);
		dofanout(t->cke, 1);
		dofanout(t->right, 1);
		dofanout(t->left, 1);
		return;
	case mux:
		if (t->fanout > i)
			return;
		dofanout(t->cke, 1);
	case xor:
	case or:
	case and:
	case tribuf:
	case bibuf:
		if (t->fanout > i)
			return;
		dofanout(t->right, 1);
	case buffer:
	case outbuf:
		if (t->fanout > i)
			return;
		dofanout(t->left, 1);
		return;
	case not:
		dofanout(t->left, i);
		return;
	default:
		yyerror("dofanout: unexpected op %d",t->op);
		return;
	}
}

Tree *
idnode(Sym *s, int bar)
{
	Tree *t;
	int hash;
	/* subtle thing here, would like negative true flip flops if inv */
	if (bar & 1)
		return notnode(idnode(s, 1^bar));
	hash = (ID*(ulong)s)%BUCK;
	for (t = bucket[hash]; t; t = t->next)
		if ((t->op == ID || t->op == CK) && t->id == s)
			return t;
	t = (Tree *) calloc(1, sizeof(Tree));
	t->op = ID;
	t->id = s;
	if (bar&2) {
		s->clock = 1;
		t->op = CK;
	}
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
	int hash;
	hash=0;
	if (uflag) {
		hash = ((op*(ulong)l)^((ulong)r>>5))%BUCK;
		for (t = bucket[hash]; t; t = t->next)
			if (t->op == op && t->left == l && t->right == r)
				return t;
	}
	t = (Tree *) calloc(1,sizeof(Tree));
	t->op = op;
	t->left = l;
	t->right = r;
	if (!uflag || op == buffer)
		return t;
	t->next = bucket[hash];
	bucket[hash] = t;
	return t;
}

Tree *
muxnode(Tree *s, Tree *l, Tree *r)
{
	Tree *t;
	int hash;
	hash=0;
	if (uflag) {
		hash = ((mux*(ulong)s)^((ulong)l>>5))%BUCK;
		for (t = bucket[hash]; t; t = t->next)
			if (t->op == mux && t->left == s && t->right == l && t->cke == r)
				return t;
	}
	t = (Tree *) calloc(1,sizeof(Tree));
	t->op = mux;
	t->left = s;
	t->right = l;
	t->cke = r;
	if (!uflag)
		return t;
	t->next = bucket[hash];
	bucket[hash] = t;
	return t;
}

Tree *
padnode(int op, Sym *s, Tree *l, Tree *r)
{
	Tree *t;
	t = (Tree *) calloc(1,sizeof(Tree));
	t->id = s;
	t->op = op;
	t->left = l;
	t->right = r;
	return t;
}

Tree *
dffnode(Sym *s, Tree *d, Tree *clk, Tree *cke, Tree *clr, Tree *pre)
{
	Tree *t;
	t = (Tree *) calloc(1,sizeof(Tree));
	t->op = dff;
	t->id = s;
	t->left = d;
	t->right = clk;
	t->cke = cke;
	t->clr = clr;
	t->pre = pre;
	return t;
}

Tree *
latnode(Sym *s, Tree *d, Tree *clk, Tree *cke, Tree *clr)
{
	Tree *t;
	t = (Tree *) calloc(1,sizeof(Tree));
	t->op = lat;
	t->id = s;
	t->left = d;
	t->right = clk;
	t->cke = cke;
	t->clr = clr;
	return t;
}

char *opname[] = {
	[C0]		"C0",
	[C1]		"C1",
	[ID]		"ID",
	[CK]		"CK",
	[not]		"not",
	[and]		"and",
	[or]		"or",
	[xor]		"xor",
	[dff]		"dff",
	[lat]		"lat",
	[inbuf]		"inbuf",
	[outbuf]	"outbuf",
	[bibuf]		"bibuf",
	[tribuf]	"tribuf",
	[buffer]	"buffer",
	[mux]		"mux",
	[clkbuf]	"clkbuf",
};

void
prtree(Tree *t)
{
	int op,i;
	if (t == 0)
		return;
	if (uflag && t->fanout)
		fprintf(stdout,"%d#%u", t->fanout+1, t);
	switch (t->op) {
	case not:
		fprintf(stdout,vflag ? "not(" : "!");
		prtree(t->left);
		fprintf(stdout,vflag ? ")" : "");
		return;
	case buffer:
	case inbuf:
	case outbuf:
	case bibuf:
	case tribuf:
	case clkbuf:
		fprintf(stdout,"%s(",opname[t->op]);
		prtree(t->left);
		if (t->op == bibuf || t->op == tribuf) {
			fprintf(stdout,",");
			prtree(t->right);
		}
		fprintf(stdout,")");
		return;
	case ID:
	case CK:
		fprintf(stdout,t->id->name);
		return;
	case C0:
	case C1:
		fprintf(stdout,"%d",t->val);
		return;
	case dff:
		fprintf(stdout,"dff(");
		for (i = 1; i < 6; i++) {
			prtree(mtGetNodes(t,i));
			fprintf(stdout,(i < 5) ? "," : ")");
		}
		return;
	case lat:
		fprintf(stdout,"lat(");
		for (i = 1; i < 5; i++) {
			prtree(mtGetNodes(t,i));
			fprintf(stdout,(i < 4) ? "," : ")");
		}
		return;
	case mux:
		fprintf(stdout,"mux(");
		for (i = 1; i < 4; i++) {
			prtree(mtGetNodes(t,i));
			fprintf(stdout,(i < 3) ? "," : ")");
		}
		return;
	default:
		fprintf(stdout,"%s(",(t->op == and) ? "and" : (t->op == or) ? "or" : "xor");
		if (vflag) {
			prtree(t->left);
			fprintf(stdout,",");
			prtree(t->right);
		}
		else {
			for (op = t->op; t && t->op == op; t = t->right) {
				prtree(t->left);
				fprintf(stdout,",");
			}
			prtree(t);
		}
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
	s->name = (char *) malloc(strlen(name)+2);	/* "-" suffix -> "_0" */
	strcpy(s->name,name);
	return s;
}

eq(NODEPTR a,NODEPTR b)
{
	return a->op == ID && b->op == ID && a->id == b->id;
}

mtValue(NODEPTR root)
{
	if(root==NULL)
		return(0);
	return(root->op);
}

NODEPTR
mtGetNodes(NODEPTR r,int n)
{
	if (r == NULL)
		if (n == 1)
			return(Root);
		else
			return(NULL);
	if (n == 1)
		return r->left;
	else if (n == 2)
		return r->right;
	else if (n == 3)
		return r->cke;
	else if (n == 4)
		return r->clr;
	else if (n == 5)
		return r->pre;
	else return(NULL);
}

void
mtSetNodes(NODEPTR r,int n,NODEPTR c)
{
	if (r == NULL && n == 1) {
		Root = c;
		return;
	}
	if (n == 1)
		r->left = c;
	else if (n == 2)
		r->right = c;
	else if (n == 3)
		r->cke = c;
	else if (n == 4)
		r->clr = c;
	else if (n == 5)
		r->pre = c;
}
