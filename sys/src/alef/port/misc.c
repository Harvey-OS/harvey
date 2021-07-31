#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#define Extern
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
	new->type = type;
	new->next = next;
	new->size = bt->size;
	new->align = bt->align;
	new->member = 0;
	new->sym = 0;
	new->tag = 0;
	new->nbuf = 0;
	if(next)
		new->class = next->class;
	return new;
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
};

void
pargs(Node *n, char *p)
{
	char buf[64];

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

	case ODOT:
		sprint(p, "%T '%s'", n->t, n->sym->name);
		break;

	case OREGISTER:
		sprint(p, "<< R%d >>", n->reg);
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
			sprint(p, "'%s' %T", n->sym->name, n->t);
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
		print("    %sZ\n", i);
		return;
	}

	if(opt('t') == 1 && n->type == OLIST) {
		ptree(n->left, depth);
		ptree(n->right, depth);
		return;
	}

	depth++;
	print("%-3d %s%N\n", n->srcline, i, n);
	ptree(n->left, depth);
	ptree(n->right, depth);
}

void
pushjmp(Jmps **j)
{
	Jmps *n;

	n = malloc(sizeof(Jmps));
	n->i = ipc;
	n->next = 0;

	n->next = *j;
	*j = n;
}

void
popjmp(Jmps **j)
{
	if(*j)
		(*j) = (*j)->next;
	else
		fatal("popjmp");
}
