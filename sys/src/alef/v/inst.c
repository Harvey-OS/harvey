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
	i->src1.type = A_NONE;
	i->dst.type = A_NONE;
	i->reg = Nreg;
	i->next = 0;
	i->lineno = iline;
	return i;
}

/* Emit a text instruction */
Inst*
instruction(int op, Node *s1, Node *s2, Node *dst)
{
	Inst *i;

	i = ai();
	i->op = op;
	i->pc = pc++;

	if(s1)
		mkaddr(s1, &i->src1, 1);

	if(s2 && s2 != dst) {
		switch(s2->type) {
		case OREGISTER:
			break;
		case OCONST:
			if(s2->ival == 0) {
				s2->reg = 0;
				break;
			}
			/* Fall */
		default:
			fatal("inst %s %N,%N,%N", itab[op], s1, s2, dst);
		}
		if(s2->t->type == TFLOAT)
			i->reg = s2->reg - Freg;
		else
			i->reg = s2->reg;
	}

	if(dst) {
		mkaddr(dst, &i->dst, 1);
		if(i->dst.type == A_REG)
		if(i->reg == i->dst.reg)
			i->reg = Nreg;
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

/* Back patch a branch */
void
label(Inst *i, ulong pc)
{
	Adres *a;

	switch(i->op) {
	case ABEQ:
	case ABFPF:
	case ABFPT:
	case ABGEZ:
	case ABGEZAL:
	case ABGTZ:
	case ABLEZ:
	case ABLTZ:
	case ABLTZAL:
	case ABNE:
	case AJAL:
	case AJMP:
		break;

	default:
		fatal("label not branch %i", i);
	}
	a = &i->dst;
	a->type = A_BRANCH;
	a->ival = pc;
}

/* Select code for arithmetic operations */
void
codmop(Node *o, Node *l, Node *r, Node *dst)
{
	Node n1;
	int op;

	op = AGOK;
	switch(o->type) {
	case OADD:
	case OADDEQ:
		switch(o->t->type) {
		default:
			op = AADDU;
			break;
		case TFLOAT:
			op = AADDD;
			break;
		}
		break;

	case OSUB:
	case OSUBEQ:
		switch(o->t->type) {
		default:
			op = ASUBU;
			break;
		case TFLOAT:
			op = ASUBD;
			break;
		}
		break;

	case OMUL:
	case OMULEQ:
		switch(o->t->type) {
		default:
			op = AMUL;
		lowreg:
			n1.type = OREGISTER;
			n1.t = builtype[TINT];
			n1.reg = Loreg;
			instruction(op, l, dst, ZeroN);
			instruction(AMOVW, &n1, ZeroN, dst);
			return;
		case TFLOAT:
			op = AMULD;
			break;
		}
		break;

	case ODIV:
	case ODIVEQ:	
		switch(o->t->type) {
		default:
			op = ADIV;
			goto lowreg;
		case TUINT:
		case TSUINT:
		case TCHAR:
			op = ADIVU;
			goto lowreg;
		case TFLOAT:
			op = ADIVD;
			break;
		}
		break;

	case OMOD:
	case OMODEQ:
		switch(o->t->type) {
		default:
			op = ADIV;
			break;
		case TUINT:
		case TSUINT:
		case TCHAR:
			op = ADIVU;
			break;
		}
		n1.type = OREGISTER;
		n1.t = builtype[TINT];
		n1.reg = Hireg;
		instruction(op, l, dst, ZeroN);
		instruction(AMOVW, &n1, ZeroN, dst);
		return;

	case OALSH:
	case OLSH:
	case OLSHEQ:
		op = ASLL;
		break;

	case ORSH:
	case ORSHEQ:
		op = ASRL;
		break;

	case OXOR:
	case OXOREQ:
		op = AXOR;
		break;

	case OLOR:
	case OOREQ:
		op = AOR;
		break;

	case OLAND:
	case OANDEQ:
		op = AAND;
		break;

	case OARSH:
		op = ASRA;
		break;
	}
	instruction(op, l, r, dst);
}

int
mulcon(Node *n, Node *nn)
{
	int o;
	long v;
	Multab *m;
	Node *l, *r, nod1, nod2, mop;
	char code[sizeof(m->code)+2], *p;

	if(n->t->type == TFLOAT)
		return 0;

	mop.t = n->t;
	l = n->left;
	r = n->right;
	if(l->type == OCONST) {
		l = r;
		r = n->left;
	}
	if(r->type != OCONST)
		return 0;

	v = r->ival;
	m = mulcon0(v);
	if(!m) {
		if(opt('M'))
			print("%L multiply table: %lld\n", r->ival);
		return 0;
	}
	if(opt('M'))
		print("%L multiply: %ld\n", v);

	memmove(code, m->code, sizeof(m->code));
	code[sizeof(m->code)] = 0;

	p = code;
	if(p[1] == 'i')
		p += 2;
	reg(&nod1, n->t, nn);
	genexp(l, &nod1);
	if(v < 0) {
		mop.type = OSUB;
		codmop(&mop, &nod1, con(0), &nod1);
	}
	reg(&nod2, n->t, ZeroN);

	for(;;) {
		switch(*p) {
		case '\0':
			regfree(&nod2);
			assign(&nod1, nn);
			regfree(&nod1);
			return 1;
		case '+':
			o = OADD;
			goto addsub;
		case '-':
			o = OSUB;
		addsub:	/* number is r,n,l */
			v = p[1] - '0';
			r = &nod1;
			if(v&4)
				r = &nod2;
			n = &nod1;
			if(v&2)
				n = &nod2;
			l = &nod1;
			if(v&1)
				l = &nod2;
			mop.type = o;
			codmop(&mop, l, n, r);
			break;
		default: /* op is shiftcount, number is r,l */
			v = p[1] - '0';
			r = &nod1;
			if(v&2)
				r = &nod2;
			l = &nod1;
			if(v&1)
				l = &nod2;
			v = *p - 'a';
			if(v < 0 || v >= 32) {
				diag(n, "mulcon unknown op: %c%c", p[0], p[1]);
				break;
			}
			mop.type = OALSH;
			codmop(&mop, con(v), l, r);
			break;
		}
		p += 2;
	}
	return 0;
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

/* Select code for conditional operators */
void
codcond(int comp, Node *src1, Node *src2, Node *dst)
{
	int a;
	Node *r;

	if(src1->type == OCONST)
	if(src1->ival == 0)
	if(src1->t->type != TFLOAT)
		src1 = regn(0);

	if(src2->type == OCONST)
	if(src2->ival == 0)
	if(src2->t->type != TFLOAT)
		src2 = regn(0);

	a = AGOK;
	switch(comp) {
	default:
		fatal("codcond: %s %N, %N, %N", treeop[comp]+1, src1, src2, dst);

	case OEQ:
		switch(src1->t->type) {
		case TFLOAT:
			break;
		default:
			instruction(ABEQ, src1, src2, dst);
			return;
		}

	case ONEQ:
		switch(src1->t->type) {
		case TFLOAT:
			break;
		default:
			instruction(ABNE, src1, src2, dst);
			return;
		}

	case OLT:
	case OGT:
	case OLEQ:
	case OGEQ:
		switch(src1->t->type) {
		case TFLOAT:
			switch(comp) {
			default:
				a = ACMPGTD;
				break;
			case ONEQ:
			case OEQ:
				a = ACMPEQD;
				break;
			case OLT:
			case OGEQ:
				a = ACMPGED;
				break;
			}
			instruction(a, src1, src2, ZeroN);
			if(comp == OEQ || comp == OGEQ || comp == OGT)
				instruction(ABFPT, ZeroN, ZeroN, ZeroN);
			else
				instruction(ABFPF, ZeroN, ZeroN, ZeroN);
			return;
		default:
			if(vconst(src1) == 0 || vconst(src2) == 0) {
				if(vconst(src1) == 0) {
					comp = inverted[comp];
					src1 = src2;
				}
				switch(comp) {
				case OGT:
					a = ABGTZ;
					break;
				case OLT:
					a = ABLTZ;
					break;
				case OLEQ:
					a = ABLEZ;
					break;
				case OGEQ:
					a = ABGEZ;
					break;
				}
				src2 = ZeroN;
				break;
			}
		}
		USED(a);
		/* Fall through */
	case OLO:
	case OHI:
	case OLOS:
	case OHIS:
		r = regtmp();
		switch(comp) {
		default:
			a = ASGT;
			break;
		case OLO:
		case OLOS:
		case OHIS:
		case OHI:
			a = ASGTU;
			break;
		}
		switch(comp) {
		default:
			instruction(a, src2, src1, r);
			break;
		case OLEQ:
		case OGT:
		case OLOS:
		case OHI:
			instruction(a, src1, src2, r);
			break;
		}
		switch(comp) {
		default:
			a = ABEQ;
			break;
		case OLT:
		case OGT:
		case OLO:
		case OHI:
			a = ABNE;
			break;
		}
		instruction(a, r, ZeroN, ZeroN);
		regfree(r);
		return;
	}
}

void
mkdata(Node *n, int off, int size, Inst *i)
{
	Adres *a;

	if(n->type != ONAME)
		fatal("mkdata %N\n", n);

	a = &i->src1;

	a->reg = Nreg;
	a->ival = off;
	a->sym = n->sym;
	a->type = A_INDREG;
	a->class = n->t->class;
	a->etype = n->t->type;

	i->reg = size;
}

void
mkaddr(Node *n, Adres *a, int ur0)
{
	long o;
	Tinfo *ti;

	a->sym = ZeroS;
	a->reg = Nreg;

	switch(n->type) {
	default:
		fatal("mkaddr: %N", n);

	case ONAME:
		ti = n->ti;

		a->type = A_INDREG;
		a->etype = ti->t->type;
		a->ival = ti->offset;
		a->sym = n->sym;
		if(ti->class == Argument) {
			a->reg = RegSP;
			a->class = A_NONE;
			break;
		}
		a->class = ti->class;
		break;

	case OCONST:
		switch(n->t->type) {
		default:
			if(ur0)
			if(n->ival == 0) {
				a->type = A_REG;
				a->reg = 0;
				break;
			}
			a->type = A_CONST;
			a->ival = n->ival;
			break;
		case TFLOAT:
			a->type = A_FCONST;
			a->fval = n->fval;
			break;
		}
		break;

	case OADD:
		if(n->left->type == OCONST) {
			mkaddr(n->left, a, 0);
			o = a->ival;
			mkaddr(n->right, a, 1);
		}
		else {
			mkaddr(n->right, a, 0);
			o = a->ival;
			mkaddr(n->left, a, 1);
		}
		a->ival += o;
		break;

	case OINDREG:
		a->type = A_INDREG;
		a->reg = n->reg;
		a->ival = n->ival;
		break;

	case OREGISTER:
		if(n->reg == Hireg)
			a->type = A_HI;
		else
		if(n->reg == Loreg)
			a->type = A_LO;
		else
			a->type = A_REG;

		switch(n->t->type) {
		default:
			a->reg = n->reg;
			break;
	
		case TFLOAT:
			a->type = A_FREG;
			if(n->reg == Retfreg)
				a->reg = Retfregno;
			else
				a->reg = n->reg - Freg;
			break;
		}
		break;

	case OIND:
		mkaddr(n->left, a, 1);
		if(a->type == A_REG || a->type == A_CONST)
			a->type = A_INDREG;
		break;

	case OADDR:
		mkaddr(n->left, a, 1);
		a->type = A_CONST;
		a->ival += n->ival;
		break;
	}
}

/* Generate function preamble and assign parameter offsets */
void
preamble(Node *name)
{
	Inst *i;
	Node *n;

	i = ai();
	i->op = ATEXT;
	i->pc = pc++;
	i->reg = txtprof;
	i->lineno = name->srcline;

	n = dupn(name);
	n->type = ONAME;
	mkaddr(n, &i->src1, 0);
	mkaddr(con(0), &i->dst, 0);
	ilink(i);
	funentry = i;		/* To back patch arg space */
	becomentry = i;		/* To back patch tail recursion */
}

/* Output accumulated string constants */
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
		mkaddr(con(s->len), &i->dst, 0);
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
			i->dst.type = A_STRING;
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
