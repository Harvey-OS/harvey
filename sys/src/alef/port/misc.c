#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#define Extern extern
#include "globl.h"

void
listcount(Node *n, Node **vec)
{
	if(n == ZeroN)
		return;

	switch(n->type) {
	case OLIST:
		listcount(n->left, vec);
		listcount(n->right, vec);
		break;

	default:
		if(vec)
			vec[veccnt] = n;
		veccnt++;	
		break;
	}
}

int
notunion(Type *t)
{
	for(t = t->next; t; t = t->member) {
		switch(t->type) {
		case TARRAY:
		case TUNION:
			return 0;
		case TADT:
		case TAGGREGATE:
			if(!notunion(t->next))
				return 0;
			break;
		}
	}
	return 1;
}

Node*
dupn(Node *n)
{
	Node *n1;

	n1 = malloc(sizeof(Node));
	*n1 = *n;
	return n1;
}

Node*
an(int type, Node *left, Node *right)
{
	Node *new;

	stats.nodes++;
	new = malloc(sizeof(Node));
	memset(new, 0, sizeof(Node));

	new->left = left;
	new->right = right;
	new->type = type;
	new->srcline = line;
	newflag = 1;
	return new;
}

Type*
abt(int type)
{
	Type *new;

	new = malloc(sizeof(Type));
	memset(new, 0, sizeof(Type));
	new->type = type;
	new->nbuf = 0;

	return new;
}

Type*
at(int type, Type *next)
{
	Type *new, *bt;

	stats.types++;

	bt = builtype[type];

	new = malloc(sizeof(Type));
	memset(new, 0, sizeof(Type));
	new->type = type;
	new->next = next;
	new->size = bt->size;
	new->align = bt->align;
	if(next)
		new->class = next->class;
	return new;
}

Tinfo*
ati(Type *t, char class)
{
	Tinfo *ti;

	ti = malloc(sizeof(Tinfo));
	memset(ti, 0, sizeof(Tinfo));
	ti->t = t;
	ti->class = class;
	ti->offset = 0;

	return ti;
}

void
addarg(Node *n, Node *a)
{
	Node *nf;

	n = n->right;
	while(n->type == OLIST)
		n = n->left;

	nf = an(0, ZeroN, ZeroN);
	*nf = *n;
	n->type = OLIST;
	n->left = a;
	n->right = nf;
	n->t = ZeroT;
}

char not[Nrops] =	/* comrel */
{
	[OEQ]	ONEQ,
	[ONEQ]	OEQ,
	[OLEQ]	OGT,
	[OLOS]	OHI,
	[OLT]	OGEQ,
	[OLO]	OHIS,
	[OGEQ]	OLT,
	[OHIS]	OLO,
	[OGT]	OLEQ,
	[OHI]	OLOS,
};

char inverted[Nrops] =	/* invrel */
{
	[OEQ]	OEQ,
	[ONEQ]	ONEQ,
	[OLEQ]	OGEQ,		
	[OLOS]	OHIS,
	[OLT]	OGT,
	[OLO]	OHI,
	[OGEQ]	OLEQ,
	[OHIS]	OLOS,
	[OGT]	OLT,
	[OHI]	OLO,

};

char *treeop[] =
{
	"OADD",
	"OSUB",
	"OMUL",
	"OMOD",
	"ODIV",
	"OIND",
	"OCAST",
	"OADDR",
	"OASGN",
	"OARRAY",
	"OCALL",
	"OCAND",
	"OCASE",
	"OCONST",
	"OCONV",
	"OCOR",
	"ODOT",
	"OELSE",
	"OEQ",
	"OFOR",
	"OFUNC",
	"OGEQ",
	"OGT",
	"OIF",
	"OLAND",
	"OLEQ",
	"OLIST",
	"OLOR",
	"OLSH",
	"OLT",
	"OAGDECL",
	"ONAME",
	"ONEQ",
	"ONOT",
	"OPAR",
	"ORECV",
	"ORET",
	"ORSH",
	"OSEND",
	"OSTORAGE",
	"OSWITCH",
	"OUNDECL",
	"OWHILE",
	"OXOR",
	"OPROTO",
	"OVARARG",
	"ODEFAULT",
	"OCONT",
	"OBREAK",
	"OREGISTER",
	"OINDREG",
	"OARSH",
	"OALSH",
	"OJMP",
	"OLO",
	"OHI",
	"OLOS",
	"OHIS",
	"ODWHILE",
	"OPROCESS",
	"OTASK",
	"OSETDECL",
	"OADDEQ",
	"OSUBEQ",
	"OMULEQ",
	"ODIVEQ",
	"OMODEQ",
	"ORSHEQ",
	"OLSHEQ",
	"OANDEQ",
	"OOREQ",
	"OXOREQ",
	"OTYPE",
	"OGOTO",
	"OLABEL",
	"OPINC",
	"OPDEC",
	"OEINC",
	"OEDEC",
	"OADTDECL",
	"OADTARG",
	"OSELECT",
	"OALLOC",
	"OCSND",
	"OCRCV",
	"OILIST",
	"OINDEX",
	"OUALLOC",
	"OCHECK",
	"ORAISE",
	"ORESCUE",
	"OTCHKED",
	"OBLOCK",
	"OLBLOCK",
	"OBECOME",
	"OITER",
	"OXEROX",
	"OVASGN",
};

void
pargs(Node *n, char *p)
{
	char buf[512];

	if(n == ZeroN)
		return;

	switch(n->type) {
	case OLIST:
		pargs(n->left, p);
		pargs(n->right, p);
		return;

	case OVARARG:
		sprint(buf, "..., ");
		break;

	case ONAME:
	case OPROTO:
	case OADTARG:
		sprint(buf, "%T, ", n->t);
		break;
	}
	strcat(p, buf);
}

int
protoconv(void *t, Fconv *f)
{
	Node *n;
	char *p, buf[512];
	int i;

	n = *((Node**)t);
	i = sprint(buf, "%T %s(", n->t->next, n->sym->name);
	if(n->left == 0)
		strcat(buf, "void, ");
	else
		pargs(n->left, buf+i);

	p = strrchr(buf, ',');
	if(p != 0) {
		p[0] = ')';
		p[1] = '\0';
	}

	strconv(buf, f);
	return(sizeof(Node*));
}

int
nodeconv(void *t, Fconv *f)
{
	Node *n;
	char *p, buf[256];
	int i;

	n = *((Node**)t);

	if(n == 0) {
		strcpy(buf, "ZeroN");
		strconv(buf, f);
		return(sizeof(Node*));
	}

	i = sprint(buf, "%s [%d,%d] ", treeop[n->type], n->sun, n->islval);
	p = buf+i;

	switch(n->type) {
	default:
		sprint(p, "%T", n->t);
		break;

	case OLABEL:
	case OGOTO:
	case ODOT:
		sprint(p, "%T '%s'", n->t, n->sym->name);
		break;

	case OREGISTER:
		sprint(p, "<%R>", n->reg);
		break;

	case OINDREG:
		sprint(p, "%d<%R>", n->ival, n->reg);
		break;

	case OCONST:
		switch(n->t->type) {
		default:
			sprint(p, "%T", n->t);
			break;

		case TFLOAT:
			sprint(p, "%T '%g'", n->t, n->fval);
			break;

		case TINT:
		case TUINT:
		case TSINT:
		case TSUINT:
		case TCHAR:
		case TIND:
			sprint(p, "%T '%d'", n->t, n->ival);
			break;
		}
		break;

	case OFUNC:
	case ONAME:
		if(n->sym == 0)
			sprint(p, ".frame %T", n->t);
		else
			sprint(p, "%T '%s'", n->t, n->sym->name);
		break;
	}

	strconv(buf, f);
	return(sizeof(Node*));
}

char indent[] = "...........................";

void
ptree(Node *n, int depth)
{
	char *i;

	i = indent+sizeof(indent)-1-depth;
	if(n == ZeroN) {
		print("     %sZ\n", i);
		return;
	}

	if(opt('t') == 1 && n->type == OLIST) {
		ptree(n->left, depth);
		ptree(n->right, depth);
		return;
	}

	depth++;
	print("%-4d %s%N\n", n->srcline, i, n);
	ptree(n->left, depth);
	ptree(n->right, depth);
}

void
pushlab(Jmps **j, Inst *i)
{
	Jmps *n;

	n = malloc(sizeof(Jmps));
	n->i = i;
	n->next = 0;
	n->par = inpar;
	n->crit = incrit;
	n->next = *j;
	*j = n;
}

void
pushjmp(Jmps **j)
{
	pushlab(j, ipc);
}

void
popjmp(Jmps **j)
{
	if(*j)
		(*j) = (*j)->next;
	else
		fatal("popjmp");
}

static int	dtest, doff;
static Node	*droot, *dlst, *dmark;

void
substitute(Node *n, Node *s, Node *t)
{
	if(n == ZeroN)
		return;

	if(n == dmark) {
		dtest = 1;
		return;
	}

	switch(n->type) {
	default:
		substitute(n->left, s, t);
		substitute(n->right, s, t);
		break;

	case ONAME:
		if(dtest == 0)
			break;
		if(n->sym == s->sym && n->ti->class == s->ti->class)
			*n = *t;
		break;
	}
}

Node*
depend(Node *n)
{
	Node *s;

	if(n == ZeroN)
		return 0;

	if(opt('b'))
		print(":%d %N\n", dtest, n);

	if(n == dmark) {
		dtest = 1;
		return 0;
	}

	switch(n->type) {
	default:
		s = depend(n->left);
		if(s != 0)
			return s;
		s = depend(n->right);
		if(s != 0)
			return s;
		break;

	case OIND:
		return n;

	case ONAME:
		if(dtest == 0)
			break;
		if(n->ti->class != Parameter)
			break;
		if(opt('b'))
			print("Check %N off %d\n", n, n->ti->offset);
		if(n->ti->offset >= doff+dmark->t->size)
			break;
		return n;
	}
	return 0;
}

void
argtree(Node *n)
{
	Node *s, *t, *l;

	if(n == ZeroN)
		return;

	switch(n->type) {
	default:
		argtree(n->left);
		argtree(n->right);
		break;
	case ONAME:
		for(;;) {
			dtest = 0;
			s = depend(droot);
			if(s == 0)
				break;

			if(opt('b'))
				print("%N %d writes %N\n", dmark, doff, s);

			l = an(OLIST, ZeroN, ZeroN);
			*l = *s;

			t = stknode(s->t);
			dtest = 0;
			substitute(droot, s, t);
		
			t = an(OASGN, t, l);
			t->t = s->t;
			dlst = an(OLIST, t, dlst);
		}
		break;
	}
}

void
argexp(Node *n)
{
	if(n == ZeroN)
		return;

	switch(n->type) {
	case OLIST:
		argexp(n->left);
		argexp(n->right);
		break;

	default:
		doff = align(doff, builtype[TINT]);
		dmark = n;
		if(opt('b'))
			print("set dmark %N off %d\n", dmark, doff);
		argtree(n);
		doff += n->t->size;
	}
}

Node*
paramdep(Node *n)
{
	if(nerr != 0)
		return nil;

	doff = Parambase;
	droot = n;
	dlst = ZeroN;

	argexp(n);
	if(opt('b')) {
		print("**\n");
		ptree(n, 0);
		print("Saves:\n");
		ptree(dlst, 0);
	}

	return dlst;
}

static
char *chanops[] =
{
	"connect",
	"announce",
	0
};

int
chkchan(Node *n, char *op)
{
	int i;

	for(i = 0; chanops[i] != 0; i++)
		if(strcmp(op, chanops[i]) == 0)
			return 0;

	diag(n, "illegal channel operation: %s", op);
	return 1;
}

int
chklval(Node *n)
{
	if(n->islval)
		return 0;

	diag(n, "not an l-value");
	return 1;
}
