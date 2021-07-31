#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"

/* These are machine specific convs which generate no code */
ulong
nopconv[Ntype] =
{
	0,			/* TXXX */
	MINT|MUINT|MIND,	/* TINT */
	MINT|MUINT|MIND,	/* TUINT */
	MSINT|MSUINT,		/* TSINT */
	MSINT|MSUINT,		/* TSUINT */
	MCHAR,			/* TCHAR */
	MFLOAT,			/* TFLOAT */
	MINT|MUINT|MIND,	/* TIND */
	0,			/* TCHANNEL */
	0,			/* TARRAY */
	MAGGREGATE,		/* TAGGREGATE */
	MUNION,			/* TUNION */
	0,			/* TFUNC */
	0,			/* TVOID */
	MADT,			/* TADT */
};

Inst*
ai(void)
{
	Inst *i;

	i = malloc(sizeof(Inst));
	i->src1.type = D_NONE;
	i->src1.index = D_NONE;
	i->dst.type = D_NONE;
	i->dst.index = D_NONE;
	i->next = 0;
	i->lineno = iline;
	return i;
}

/*
 * Emit an assembler instruction
 */
Inst*
instruction(int op, Node *s1, Node *dst)
{
	Inst *i;

	i = ai();
	i->op = op;
	i->pc = pc++;

	if(s1)
		mkaddr(s1, &i->src1, 0);
	else {
		i->src1.type = D_NONE;
		i->src1.index = D_NONE;
	}
	if(dst)
		mkaddr(dst, &i->dst, 0);
	else {
		i->dst.type = D_NONE;
		i->dst.index = D_NONE;
	}

	ilink(i);
	return ipc;
}

void
ilink(Inst *i)
{
	if(opt('c'))
		print("(%d) %4d %i\n", i->lineno, i->pc, i);

	if(proghead)
		ipc->next = i;
	else
		proghead = i;

	ipc = i;
}

/*
 * Back patch a branch
 */
void
label(Inst *i, ulong pc)
{
	Adres *a;

	switch(i->op) {
	case AJCC:
	case AJCS:
	case AJCXZ:
	case AJEQ:
	case AJGE:
	case AJGT:
	case AJHI:
	case AJLE:
	case AJLS:
	case AJLT:
	case AJMI:
	case AJMP:
	case AJNE:
	case AJOC:
	case AJOS:
	case AJPC:
	case AJPL:
	case AJPS:
		break;

	default:
		fatal("label not branch");
	}
	a = &i->dst;
	a->type = D_BRANCH;
	a->ival = pc;
}

/*
 * Select code for arithmetic operations
 */
void
codmop(Node *o, Node *l, Node *r, Node *dst)
{
	int op;

	if(r && r != dst)
		fatal("codmop: not three address!");

	op = AGOK;
	switch(o->type) {
	case OADD:
	case OADDEQ:
		if(l->type == OCONST) {
			if(l->ival == 1) {
				switch(o->t->type) {
				default:
					op = AINCL;
					break;
				case TCHAR:
					op = AINCB;
					break;
				case TSINT:
				case TSUINT:
					op = AINCW;
					break;
				}
				instruction(op, ZeroN, dst);
				return;
			}
			if(l->ival == -1) {
				switch(o->t->type) {
				default:
					op = ADECL;
					break;
				case TCHAR:
					op = ADECB;
					break;
				case TSINT:
				case TSUINT:
					op = ADECW;
					break;
				}
				instruction(op, ZeroN, dst);
				return;
			}
		}
		switch(o->t->type) {
		default:
			op = AADDL;
			break;
		case TCHAR:
			op = AADDB;
			break;
		case TSINT:
		case TSUINT:
			op = AADDW;
			break;
		}
		break;

	case OSUB:
	case OSUBEQ:
		switch(o->t->type) {
		default:
			op = ASUBL;
			break;
		case TCHAR:
			op = ASUBB;
			break;
		case TSINT:
		case TSUINT:
			op = ASUBW;
			break;
		}
		break;

	case OMUL:
	case OMULEQ:
		switch(o->t->type) {
		default:
			op = AIMULL;
			break;
		case TCHAR:		/* Unsigned */
		case TUINT:
		case TSUINT:
			op = AMULL;
			break;
		}
		instruction(op, l, ZeroN);
		return;

	case ODIV:
	case ODIVEQ:	
	case OMOD:
	case OMODEQ:
		switch(o->t->type) {
		default:
			op = AIDIVL;
			instruction(ACDQ, ZeroN, ZeroN);
			break;
		case TCHAR:		/* Unsigned */
		case TUINT:
		case TSUINT:
			op = ADIVL;
			instruction(AMOVL, con(0), regn(D_DX));
			break;
		}
		instruction(op, l, ZeroN);
		return;

	case OALSH:
	case OLSH:
	case OLSHEQ:
		switch(o->t->type) {
		case TINT:
			op = ASALL;
			break;
		case TUINT:
			op = ASHLL;
			break;
		case TCHAR:
			op = ASHLB;
			break;
		case TSINT:
			op = ASALW;
			break;
		case TSUINT:
			op = ASHLW;
			break;
		}
		break;

	case ORSH:
	case ORSHEQ:
		switch(o->t->type) {
		case TINT:
			op = ASARL;
			break;
		case TUINT:
			op = ASHRL;
			break;
		case TCHAR:
			op = ASHRB;
			break;
		case TSINT:
			op = ASARW;
			break;
		case TSUINT:
			op = ASHRW;
			break;
		}
		break;

	case OXOR:
	case OXOREQ:
		switch(o->t->type) {
		default:
			op = AXORL;
			break;
		case TCHAR:
			op = AXORB;
			break;
		case TSINT:
		case TSUINT:
			op = AXORW;
			break;
		}
		break;

	case OLOR:
	case OOREQ:
		switch(o->t->type) {
		default:
			op = AORL;
			break;
		case TCHAR:
			op = AORB;
			break;
		case TSINT:
		case TSUINT:
			op = AORW;
			break;
		}
		break;

	case OLAND:
	case OANDEQ:
		switch(o->t->type) {
		default:
			op = AANDL;
			break;
		case TCHAR:
			op = AANDB;
			break;
		case TSINT:
		case TSUINT:
			op = AANDW;
			break;
		}
		break;

	case OARSH:
		switch(o->t->type) {
		default:
			op = ASARL;
			break;
		case TCHAR:
			op = ASARB;
			break;
		case TSINT:
		case TSUINT:
			op = ASARW;
			break;
		}
		break;
	}
	instruction(op, l, dst);
}

void
codfop(Node *o, Node *l, Node *r, int flag)
{
	int op;

	SET(op);

	switch(o->type) {
	case OADD:
	case OADDEQ:
		if(flag&FPOP)
			op = AFADDDP;
		else
			op = AFADDD;
		break;
	case OSUB:
	case OSUBEQ:
		switch(flag) {
		case FNONE:
			op = AFSUBD;
			break;
		case FPOP:
			op = AFSUBDP;
			break;
		case FREV:
			op = AFSUBRD;
			break;
		case FPOP|FREV:
			op = AFSUBRDP;
			break;
		}
		break;
	case ODIV:
	case ODIVEQ:
		switch(flag) {
		case FNONE:
			op = AFDIVD;
			break;
		case FPOP:
			op = AFDIVDP;
			break;
		case FREV:
			op = AFDIVRD;
			break;
		case FPOP|FREV:
			op = AFDIVRDP;
			break;
		}
		break;
	case OMUL:
	case OMULEQ:
		op = AFMULD;
		if(flag&FPOP)
			op = AFMULDP;
		break;
	}
	instruction(op, l, r);
}

int
vconst(Node *n)
{
	int i;

	if(n == ZeroN || n->type != OCONST || n->t == ZeroT)
		return -159;

	switch(n->t->type) {
	case TFLOAT:
		i = 100;
		if(n->fval > i || n->fval < -i)
			return -159;
		i = n->fval;
		if(i != n->fval)
			return -159;
		return i;

	case TCHAR:
	case TSINT:
	case TSUINT:
	case TINT:
	case TUINT:
	case TIND:
		i = n->ival;
		if(i != n->ival)
			return -159;
		return i;
	}
}

/*
 * Select code for conditional operators
 */

static int condtab[] =
{
	[OEQ]	AJEQ,
	[ONEQ]	AJNE,
	[OLT]	AJLT,
	[OGT]	AJGT,
	[OLEQ]	AJLE,
	[OGEQ]	AJGE,
	[OLO]	AJCS,
	[OHI]	AJHI,
	[OLOS]	AJLS,
	[OHIS]	AJCC,
};

void
codcond(int comp, Node *src1, Node *src2)
{
	int a, t;

	t = src1->t->type;
	a = condtab[comp];
	if(a == 0)
		fatal("codcond: %N %s %N", src1, treeop[comp]+1, src2);

	switch(t) {
	default:
		instruction(ACMPL, src1, src2);
		break;
	case TSINT:
	case TSUINT:
		instruction(ACMPW, src1, src2);
		break;
	case TCHAR:
		instruction(ACMPB, src1, src2);
		break;
	}		
	instruction(a, ZeroN, ZeroN);
}

static int fcondtab[] =
{
	[OEQ]	AJEQ,
	[ONEQ]	AJNE,
	[OLT]	AJCS,
	[OGT]	AJHI,
	[OLEQ]	AJLS,
	[OGEQ]	AJCC,
};

void
condfop(int op, Node *l, Node *r, int flag)
{
	int a;
	Node n, *rn;

	switch(flag) {
	default:			/* Used as rval side */
		a = AFCOMF;
		break;
	case FPOP:			/* Used for lval */
	case FREV:
		a = AFCOMFP;
		break;
	case FREV|FPOP:			/* Result discarded */
		a = AFCOMDPP;
		break;
	}

	instruction(a, l, r);
	reg(&n, builtype[TINT], ZeroN);
	if(n.reg != D_AX) {
		regfree(&n);
		n.reg = D_AX;
		rn = rpush(D_AX);
		instruction(AWAIT, ZeroN, ZeroN);
		instruction(AFSTSW, ZeroN, &n);
		instruction(ASAHF, ZeroN, ZeroN);
		rpop(&n, rn);
	}
	else {
		instruction(AWAIT, ZeroN, ZeroN);
		instruction(AFSTSW, ZeroN, &n);
		instruction(ASAHF, ZeroN, ZeroN);
		regfree(&n);
	}

	a = fcondtab[op];
	if(a == 0)
		fatal("condfop: %N %s %N", l, treeop[op]+1, r);
	instruction(a, ZeroN, ZeroN);
}

void
mkdata(Node *n, int off, int size, Inst *i)
{
	Adres *a;

	if(n->type != ONAME)
		fatal("mkdata %N\n", n);

	a = &i->src1;

	a->index = D_NONE;
	a->scale = size;
	a->ival = off;
	a->sym = n->sym;
	switch(n->t->class) {
	default:
		fatal("mkdata: class");
	case Internal:
		a->type = D_STATIC;
		break;
	case External:
	case Global:
		a->type = D_EXTERN;
		break;
	}
	a->etype = n->t->type;
}

void
mkaddr(Node *n, Adres *a, int ur0)
{
	ulong l;
	Node *p;
	Tinfo *ti;

	USED(ur0);
	a->sym = ZeroS;
	a->index = D_NONE;

	switch(n->type) {
	default:
		fatal("mkaddr: %N\n", n);

	case ONAME:
		ti = n->ti;
		l = ti->offset;
		switch(ti->class) {
		default:
			a->type = D_NONE;
			break;
		case Internal:
			a->type = D_STATIC;
			a->sym = n->sym;
			break;
		case External:
		case Global:
			a->type = D_EXTERN;
			a->sym = n->sym;
			break;
		case Parameter:
			a->type = D_PARAM;
			a->sym = n->sym;
			break;
		case Automatic:
			a->type = D_AUTO;
			a->sym = n->sym;
			break;
		case Argument:
			a->type = D_INDIR+D_SP;
			break;
		}
		a->etype = ti->t->type;
		a->ival = l;
		break;

	case OCONST:
		switch(n->t->type) {
		default:
			a->type = D_CONST;
			a->ival = n->ival;
			break;
		case TFLOAT:
			a->type = D_FCONST;
			a->fval = n->fval;
			break;
		}
		break;

	case OINDREG:
		a->type = n->reg + D_INDIR;
		a->ival = n->ival;
		break;

	case OREGISTER:
		a->type = n->reg;
		break;

	case OADD:
		l = n->right->ival;
		p = n->left;
		if(n->left->type == OCONST) {
			l = n->left->ival;
			p = n->right;
		}
		mkaddr(p, a, 0);
		a->ival += l;
		break;

	case OIND:
		mkaddr(n->left, a, 0);
		switch(a->type) {
		case D_AX:
		case D_CX:
		case D_DX:
		case D_BX:
		case D_SP:
		case D_BP:
		case D_SI:
		case D_DI:
			a->type += D_INDIR;
			break;
		case D_CONST:
			a->type = D_NONE+D_INDIR;
			break;
		case D_ADDR:
			a->type = a->index;
			a->index = D_NONE;
			break;
		default:
			fatal("mkaddr: bad OIND %a", a);		
		}
		break;

	case OADDR:
		mkaddr(n->left, a, 0);
		if(a->type >= D_INDIR)
			a->type -= D_INDIR;
		else
		switch(a->type) {
		default:
			fatal("mkaddr: bad OADDR");
		case D_EXTERN:
		case D_STATIC:
		case D_AUTO:
		case D_PARAM:
			if(a->index == D_NONE) {
				a->index = a->type;
				a->type = D_ADDR;
			}
		}
		break;
	}
}

/*
 * Generate function preamble and assign parameter offsets
 */
void
preamble(Node *name)
{
	Inst *i;
	Node *n;

	i = ai();
	i->op = ATEXT;
	i->pc = pc++;
	i->lineno = name->srcline;

	n = dupn(name);
	n->type = ONAME;
	mkaddr(n, &i->src1, 0);
	mkaddr(con(0), &i->dst, 0);
	ilink(i);
	funentry = i;		/* To back patch arg space */
	becomentry = i;		/* To back patch tail recursion */
}

/*
 * Output accumulated string constants
 */
void
strop(void)
{
	String *s;
	int c, l;
	Inst *i;
	char *p;

	for(s = strdat; s; s = s->next) {
		i = ai();
		i->op = AGLOBL;
		mkaddr(&s->n, &i->src1, 0);
		mkaddr(con(s->len), &i->dst,  0);
		ilink(i);

		c = 0;
		p = s->string;
		while(s->len) {
			l = s->len;
			if(l > 8)
				l = 8;

			iline = s->n.srcline;
			i = ai();
			i->op = ADATA;
			i->dst.type = D_SCONST;
			i->dst.index = D_NONE;
			memset(i->dst.str, 0, sizeof(i->dst.str));
			memmove(i->dst.str, p, l);
			s->len -= l;
			mkdata(&s->n, c, l, i);
			p += l;
			c += l;
			ilink(i);
		}
	}
}
