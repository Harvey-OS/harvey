#include <u.h>
#include <libc.h>
#include <bio.h>
#include "parl.h"
#include "y.tab.h"
#define Extern
#include "globl.h"
#include "tcom.h"

static Type **agtail;

void
applyarray(Node *n, Type *newt)
{
	int sz;
	char *s;
	Node *f;

	s = n->sym->name;

	for(f = n->left; f; f = f->right) {		/* OARRAY */
		sz = 0;
		if(f->left) {
			if(f->left->type != OCONST) {
				diag(n, "%s[] must have constant size", s);
				break;
			}

			sz = f->left->ival;
		}

		if(sz < 0 ||(sz == 0 && n->init == ZeroN && newt->class != External)) {
			diag(n, "%s[%d] dimension must be positive constant", s, sz);
			break;
		}
		newt = at(TARRAY, newt);
		newt->size = newt->next->size*sz;
	}
	n->t = newt;
}

/* 
 * recurse through the declaration list attaching types to symbols and tags
 */
void
applytype(Type *tbasic, Node *n)
{
	char class;
	Node *f, *proto, *p;

	if(n == ZeroN)
		return;

	proto = ZeroN;

	switch(n->type) {
	default:
		applytype(tbasic, n->left);
		applytype(tbasic, n->right);
		break;

	case OPROTO:
		diag(n, "illegal argument list");
		break;

	case OFUNC:
		n->type = ONAME;
		proto = n->left;
		n->left = ZeroN;

		/* Fall through */
	case ONAME:
		class = tbasic->class;

		for(f = n->right; f; f = f->left)		/* OIND */
			tbasic = at(TIND, tbasic);

		applyarray(n, tbasic);

		/* Rearrange for pointer to function declaration */
		if(proto) {
			n->t = at(TFUNC, n->t);
			p = an(ONAME, n->proto, ZeroN);
			p->sym = n->sym;
			p->t = n->t;
			n->proto = p;
			n->t->proto = p;
			for(f = proto->right; f; f = f->left)
				n->t = at(TIND, n->t);

			if(proto->left) {
				n->left = proto->left;
				applyarray(n, n->t);
			}
		}
		n->t->class = class;
		if(class == Private) {
			switch(n->t->type) {
			default:
				diag(n, "illegal storage class for type %T", n->t);
				break;
			case TINT:
			case TUINT:
			case TSINT:
			case TSUINT:
			case TCHAR:
			case TIND:
				break;
			}
		}
		break;
	}
}

Type*
tmcpy(Type *t)
{
	Type *new;

	new = abt(t->type);
	new->next = t->next;
	new->size = t->size;
	new->align = t->align;
	new->member = 0;
	new->class = t->class;
	new->sym = t->sym;
	new->tag = t->tag;
	new->nbuf = t->nbuf;

	return new;
}

void
agunshape(Node *n)
{
	Type *t;

	if(n == ZeroN)
		return;

	switch(n->type) {
	default:
		agunshape(n->left);
		agunshape(n->right);
		return;

	case OTYPE:
		t = tmcpy(n->t);
		t->tag = 0;			/* Unnamed so kill tag */
		break;

	case OAGDECL:
	case OUNDECL:
	case OADTDECL:
		t = tmcpy(n->t);
		if(t->tag == 0)
			t->tag = n->sym;
		break;

	case ONAME:
		t = tmcpy(n->t);
		t->tag = n->sym;
		break;
	}	
	*agtail = t;
	agtail = &t->member;
}

/*
 * Build a type structure for a complex
 */
void
buildtype(Node *n)
{
	Sym *sym, *tag;
	ulong offset;
	char *sn, *tn;
	Type *t, *m;
	Node *f;

	if(n->type ==  OSETDECL)
		return;

	f = n->left;
	sym = ZeroS;
	if(f->left)
		sym = f->left->sym;

	tag = ZeroS;
	if(f->right)
		tag = f->right->sym;

	switch(n->type) {
	default:
		fatal("buildtype %N", n);

	case OAGDECL:
		t = at(TAGGREGATE, 0);
		break;

	case OADTDECL:
		t = at(TADT, 0);
		break;

	case OUNDECL:
		t = at(TUNION, 0);
		break;
	}		

	/* Recurse through */
	agtail = &t->next;
	agunshape(n->right);
	n->t = t;


	/* Tag the parse tree with the name */
	if(sym) {
		if(sym->ltype)
		if(sym->ltype->type != TXXX)
			diag(n, "%s redclared type specifier", sym->name);

		sym->ltype = t;
	}

	/* pick up the tag if this is a sub of a complex type */
	if(tag)
		t->tag = tag;
	else
		t->tag = sym;

	/* propagate sizes and align structure members */
	offset = 0;
	for(m = t->next; m; m = m->member) {
		switch(t->type) {
		case TAGGREGATE:
			m->class = External;
			/* Fall through */

		case TADT:
			if(m->class == Adtdeflt) {
				m->class = Internal;
				if(m->type == TFUNC)
					m->class = External;
			}
			if(m->type == TFUNC)
				break;

			offset = align(offset, m);
			m->offset = offset;
			offset += m->size;
			break;

		case TUNION:
			m->class = External;
			if(m->size > offset)
				offset = m->size;
			m->offset = 0;
			break;
		}
	}
	/* Align the tail to integral machine type */
	t->size = align(offset, builtype[TINT]);

	if(opt('C')) {
		sn = "unnamed";
		tn = sn;
		if(sym)
			sn = sym->name;
		if(tag)
			tn = tag->name;
		print("COMPLEX %s.%s\n", sn, tn);
		prnagun(t, 0);
		print("*\n");
	}
}

/*
 * detect casts which generate no code
 */
int
convisnop(Type *t1, Type *t2)
{
	int i, b;

	i = 0;
	if(t2)
		i = t2->type;
	b = 1<<i;
	i = 0;
	if(t1)
		i = t1->type;
	if(b)
	if(b & nopconv[i])
		return 1;

	return 0;
}

/*
 * type arithmetic operators and do pointer arith scaling
 */
int
tmathop(Node *n, int logical, char tab[Ntype][Ntype])
{
	int v;
	Node *newn, *l, *r;

	l = n->left;
	r = n->right;
	if(opt('m'))
		print("tmathop: %N l %N r %N\n", n,  l, r);

	if(l->t->type == TIND && n->type == OSUB && r->t->type == TIND) {
		n->t = builtype[TINT];
		v = l->t->next->size;
		/* Make ODIV (OSUB ptr ptr) size */
		if(v <= 0) {
			diag(n, "pointer SUB '%T' illegal", l->t);
			return 1;
		}
		if(v == 1)
			return 0;

		newn = con(v);
		newn = an(ODIV, ZeroN, newn);
		newn->t = newn->right->t;
		newn->left = an(0, ZeroN, ZeroN);
		*newn->left = *n;
		*n = *newn;
		return 0;
	}
	v = tab[l->t->type][r->t->type];

	if(opt('m'))
		print("tmathop: %T %T %d\n", l->t, r->t, v);

	switch(v) {
	case TFLOAT:
	case TIND:
		if(logical == 0)
			break;

		/* Fall through */
	case 0:
		goto bad;
	}

	/* One side needs fixing */
	if(v == TIND && tab != logtype) {
		switch(n->type) {
		default:
			goto bad;
		case OADD:
		case OSUB:
		case OADDEQ:
		case OSUBEQ:
			break;
		}
		/* Insert OADD (OMUL size int) ptr */
		if(l->t->type == TIND) {
			n->t = l->t;
			v = l->t->next->size;
			if(v <= 0) {
				diag(n, "pointer ADD '%T' illegal", l->t);
				return 1;
			}
			if(v == 1)
				return 0;

			newn = an(OMUL, r, con(v));
			newn->t = builtype[TINT];
			n->right = newn;
			return 0;
		}

		n->t = r->t;
		v = r->t->next->size;
		if(v <= 0) {
			diag(n, "pointer ADD '%T' illegal", l->t);
			return 1;
		}
		if(v == 1)
			return 0;

		newn = an(OMUL, l, con(v));
		newn->t = builtype[TINT];
		n->left = newn;
		return 0;
	}

	/* Propogate results and insert converts to fix types */	
	if(r->t->type == v) {
		n->t = r->t;
		if(typecmp(n->t, l->t, 5) == 0) {
			newn = an(0, ZeroN, ZeroN);
			*newn = *l;
			l->type = OCONV;
			l->right = ZeroN;
			l->t = n->t;
			l->left = newn;
		}
	}
	else {
		n->t = l->t;
		if(typecmp(n->t, r->t, 5) == 0) {
			newn = an(0, ZeroN, ZeroN);
			*newn = *r;
			r->type = OCONV;
			r->right = ZeroN;
			r->t = n->t;
			r->left = newn;
		}
	}
	return 0;
bad:
	diag(n, "bad pointer math '%T' %s '%T'", l->t, treeop[n->type]+1, r->t);
	return 1;
}

/*
 * Build a call through an ADT
 */
int
tyadt(Node *n)
{
	Sym *s;
	Type *t;
	int junk;
	char buf[Strsize];
	Node *l, *adt, *nf;

	l = n->left;

	if(typechk(l->left, 0))
		return 1;

	adt = an(OTCHKED, l->left, ZeroN);
	adt->t = l->left->t;
	l->left = adt;

	t = walktype(adt->t, l, &junk);
	if(t == ZeroT) {
		diag(n, "%s not a member of %T", l->sym->name, adt->t);
		return 1;
	}
	if(t->type != TFUNC) {
		diag(n, "adt call must be to function");
		return 1;
	}

	adt = an(OADDR, l, ZeroN);

	/* make call to function */
	nf = an(ONAME, ZeroN, ZeroN);
	s = ltytosym(parent);
	if(s == ZeroS)
		fatal("tyadt");

	sprint(buf, "%s_%s", s->name, t->tag->name);
	s = lookup(buf);
	if(s == ZeroS)
		s = enter(buf, Tid);

	nf->t = t;
	nf->sym = s;
	nf->ti = malloc(sizeof(Tinfo));
	nf->ti->class = External;
	nf->ti->offset = 0;
	n->left = nf;

	l = t->proto;
	while(l->left)
		l = l->left;

	/* has *ADT as first arg */
	if(l->type == OADTARG) {
		if(n->right == ZeroN)
			n->right = adt;
		else
			addarg(n, adt);
	}

	return typechk(n, 0);
}

/*
 * typecheck a portion of the tree
 */
int
typechk(Node *n, int evalfor)
{
	Node *l, *r;
	Type *t;
	int v, size;

	if(n == ZeroN)
		return 0;

	l = n->left;
	r = n->right;

	switch(n->type) {
	default:
		return typechk(l, 0) | typechk(r, 0);

	case OTCHKED:
		*n = *l;
		return 0;

	case OALLOC:
		*n = *l;
		return tyalloc(n);

	case OUALLOC:
		*n = *l;
		return tyunalloc(n);

	case OCALL:
		if(l->type == ODOT)
			return tyadt(n);

		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(l->t->type == TIND)
		if(l->t->next->type == TFUNC) {
			l = an(OIND, l, ZeroN);
			l->t = l->left->t->next;
			n->left = l;
		}

		if(tycall(n))
			return 1;
		
		/* Pick up the return type from TFUNC->next */
		n->t = l->t->next;
		break;

	case OPROCESS:
		if(typechk(l, 0))
			return 1;

		if(l->type != OCALL) {
			diag(n, "proc must call a function");
			return 1;
		}

		typroc(l, procnode);
		*n = *l;
		return 0;

	case OTASK:
		if(typechk(l, 0))
			return 1;

		if(l->type != OCALL) {
			diag(n, "task must call a function");
			return 1;
		}

		typroc(l, tasknode);
		*n = *l;
		return 0;

	case OCHECK:
		return tyassert(n);

	case OILIST:
		if(n->t == ZeroT)
			return 0;
		if(((1<<n->t->type) & MCOMPLEX) == 0) {
			diag(n, "compound initialiser type must be complex '%T'", n->t);
			return 1;
		}
		return tasgninit(n->t, n, 0);

	case ONAME:
		if(n->t == 0) {
			diag(n, "undefined variable %s", n->sym->name);
			return 1;
		}
		if(n->ti->class == Private) {
			n->type = OREGISTER;
			n->reg = n->ti->offset;
		}
		n->islval = 1;
		break;

	case OAGDECL:
	case OUNDECL:
	case OSETDECL:
	case OADTDECL:
		fatal("typechk");

	case OPINC:
	case OPDEC:
	case OEINC:
	case OEDEC:
		if(typechk(l, 0))
			return 1;

		if(chklval(l))
			return 1;

		v = (1<<l->t->type) & MINCDEC;
		if(v == 0) {
			diag(n, "bad type for ++ or -- '%T'", l->t);
			return 1;
		}
		n->t = l->t;
		if(n->t->type == TIND)
		if(n->t->next->size < 1)
			diag(n, "illegal ++ or -- pointer math");
		break;

	case ONOT:
		if(typechk(l, 0))
			return 1;

		n->t = builtype[TINT];
		break;

	case OCAND:
	case OEQ:
	case OLEQ:
	case ONEQ:
	case OLT:
	case OGEQ:
	case OGT:
	case OCOR:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(tmathop(n, 0, logtype))
			return 1;

		if(((1<<l->t->type)&MUNSIGNED) || ((1<<r->t->type)&MUNSIGNED)) {
			switch(n->type) {
			case OLT:
				n->type = OLO;
				break;

			case OGT:
				n->type = OHI;
				break;

			case OGEQ:
				n->type = OHIS;
				break;

			case OLEQ:
				n->type = OLOS;
				break;
			}
		}
		n->t = builtype[TINT];
		break;

	case OSTORAGE:
		size = 0;
		if(n->t)
			size = n->t->size;
		else {
			if(typechk(l, NADDR))
				return 1;

			if(l->t)
				size = l->t->size;
		}
		if(size == 0)
			diag(n, "sizeof unsized type");

		n->type = OCONST;
		n->ival = size;
		n->t = builtype[TINT];
		break;

	case OADD:
		if(n->t)			/* Adt already typed */
			return 0;

	case ODIV:
	case OSUB:
	case OMUL:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		return tmathop(n, 0, moptype);

	case OMOD:
	case OXOR:
	case OLOR:
	case OLAND:
	case OLSH:
	case ORSH:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		return tmathop(n, 1, moptype);

	case OADDEQ:
	case OSUBEQ:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(chklval(l))
			return 1;

		if(typeval(typeaddasgn, l->t, r->t)) {
			diag(n, "incompatible types: '%T' %s '%T'",
						l->t, treeop[n->type]+1, r->t);
			return 1;
		}

		return tmathop(n, 0, moptype);
		break;

	case OMULEQ:
	case ODIVEQ:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(chklval(l))
			return 1;

		if(typeval(typeasgn, l->t, r->t)) {
			diag(n, "incompatible types: '%T' %s '%T'",
						l->t, treeop[n->type]+1, r->t);
			return 1;
		}

		return tmathop(n, 0, moptype);
		break;

	case ORSHEQ:
	case OLSHEQ:
	case OOREQ:
	case OANDEQ:
	case OXOREQ:
	case OMODEQ:
		if(typechk(l, 0) || typechk(r, 0))
			return 1;

		if(chklval(l))
			return 1;

		if(typeval(typeasand, l->t, r->t)) {
			diag(n, "incompatible types: '%T' %s '%T'",
						l->t, treeop[n->type]+1, r->t);
			return 1;
		}

		return tmathop(n, 1, moptype);
		break;
	
	case ORECV:
		if(typechk(l, 0))
			return 1;

		if(l->t->type != TCHANNEL) {
			diag(n, "must receive on a channel: '%T'", l->t);
			return 1;
		}

		n->t = l->t->next;
		switch(n->t->type) {
		case TADT:
		case TUNION:
		case TAGGREGATE:
			l = an(OLIST, l, con(n->t->size));
			break;
		}
		n->right = l;
		n->left = rfun[n->t->type];
		break;

	case OCSND:
	case OCRCV:
		if(typechk(l, 0))
			return 1;

		if(l->t->type != TCHANNEL) {
			diag(n, "must ? a channel: '%T'", l->t);
			return 1;
		}
		n->t = builtype[TINT];
		if(n->type == OCSND)
			n->left = csndnode;
		else
			n->left = crcvnode;
		n->type = OCALL;
		n->right = l;
		break;

	case OASGN:
		if(l->type == OSEND) 
			return tysend(n);

		if(typechk(l, 0) || chklval(l))
			return 1;

		if(typechk(r, 0))
			return 1;

		/* Check legality */
		if(typeval(typeasgn, l->t, r->t)) {
			if(promote(r, l)) {
				diag(n, "incompatible types: '%T' = '%T'", l->t, r->t);
				return 1;
			}
		}

		/* Insert the cast */
		if(typecmp(l->t, r->t, 5) == 0) {
			n->right = an(OCONV, r, ZeroN);
			n->right->t = l->t;
		}

		n->t = l->t;
		break;

	case OSEND:
		diag(n, "send requires assignment");
		return 1;

	case OIND:
		if(typechk(l, 0))
			return 1;

		if(l->t->type != TIND) {
			diag(n, "illegal indirection");
			return 1;
		}
		n->t = l->t->next;
		n->islval = 1;
		break;

	case OADDR:
		if(typechk(l, 0))
			return 1;

		if(chklval(l))
			return 1;

		if(l->type == OREGISTER)
			diag(n, "cannot take address of private");

		n->t = at(TIND, l->t);
		break;

	case ODOT:
		if(typechk(l, 0))
			return 1;

		if(tydot(n))
			return 1;
		break;

	case OCAST:
		if(typechk(l, 0))
			return 1;

		if(n->t->size != l->t->size) {
			diag(n, "cast must be same size: [%T]%T", n->t, l->t);
			return 1;
		}

		break;

	case OCONV:
		if(typechk(l, 0))
			return 1;

		v = convtab[n->t->type] & (1<<l->t->type);
		if(v == 0) {
			diag(n, "illegal convert (%T)%T", n->t, l->t);
			return 1;
		}
		if(convisnop(n->t, l->t)) {
			l->t = n->t;
			*n = *l;
		}
		break;

	case ORET:
		t = curfunc->t->next;	/* return type */
		if(l == ZeroN) {
			if(t->type != TVOID) {
				diag(n, "return should be %P", curfunc);
				return 1; 
			}
			n->t = builtype[TVOID];
			break;
		}

		if(t->type == TVOID) {
			diag(n, "return should be %P", curfunc);
			return 1;
		}

		if(typechk(l, 0))
			return 1;

		n->t = t;

		/* Check legality */
		if(typeval(typeasgn, n->t, l->t)) {
			diag(n, "incompatible types: %P returns '%T'", curfunc, l->t);
			return 1;
		}

		/* Insert the cast */
		if(typecmp(n->t, l->t, 5) == 0) {
			n->left = an(OCONV, l, ZeroN);
			n->left->t = n->t;
		}
		break;
	}
	typesu(n);

	if(evalfor == NADDR)
		return 0;
	else
		return tyaddr(n);
}

int
tyassert(Node *n)
{
	String *s;
	char mesg[128];
	Node *n1, *sn;

	if(chks == 0) {
		n->type = OLIST;
		n->left = ZeroN;
		n->right = ZeroN;
		return 0;
	}

	n->type = ONOT;
	n1 = an(0, ZeroN, ZeroN);
	*n1 = *n;
	
	if(typechk(n, 0))
		return 1;

	sprint(mesg, "%L runtime check fail\n", n->srcline);
	s = malloc(sizeof(String));
	s->string = strdup(mesg);
	s->len = strlen(mesg)+1;

	n->type = OIF;
	n->left = n1;
	sn = strnode(s);
	if(typechk(sn, 0))
		fatal("tyassert");
	n->right = an(OCALL, asfailnode, sn);
	n->right->t = builtype[TVOID];

	return 0;
}
