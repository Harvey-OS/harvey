#include "cc.h"

void
evconst(Node *n)
{
	Node *l, *r;
	int et, isi, isf;
	long v;

	if(n == Z || n->type == T)
		return;
	et = n->type->etype;
	isi = typechlp[et];
	isf = FPCHIP && typefdv[et];
	l = n->left;
	r = n->right;
	switch(n->op) {
	default:
		return;

	case OCAST:
		if(isi) {
			et = l->type->etype;
			if(typefdv[et]) {
				if(!FPCHIP)
					return;
				if(l->ud > 0x7fffffff || l->ud < -0x7fffffff) {
					warn(n, "overflow in fp->int conversion");
					return;		/* let runtime get the fault */
				}
				v = l->ud;
			} else
			if(typechlp[et])
				v = l->offset;
			else
				return;
			n->offset = castto(v, n->type->etype);
			break;
		}
		if(FPCHIP && isf) {
			et = l->type->etype;
			if(typechlp[et]) {
				if(typeu[et] && l->offset < 0)
					return;		/* let runtime do it */
				n->ud = l->offset;
				break;
			}
			if(typefdv[et]) {
				n->ud = l->ud;
				break;
			}
			return;
		}
		return;

	case OCONST:
		break;

	case OADD:
		if(isi) {
			n->offset = l->offset + r->offset;
			break;
		}
		if(isf) {
			n->ud = l->ud + r->ud;
			break;
		}
		return;

	case OSUB:
		if(isi) {
			n->offset = l->offset - r->offset;
			break;
		}
		if(isf) {
			n->ud = l->ud - r->ud;
			break;
		}
		return;

	case OMUL:
		if(isi) {
			n->offset = l->offset * r->offset;
			break;
		}
		if(isf) {
			n->ud = l->ud * r->ud;
			break;
		}
		return;
	case OLMUL:
		if(isi) {
			n->offset = (ulong)l->offset * (ulong)r->offset;
			break;
		}
		return;


	case ODIV:
		if(isi) {
			n->offset = l->offset / r->offset;
			break;
		}
		if(isf) {
			n->ud = l->ud / r->ud;
			break;
		}
		return;

	case OLDIV:
		if(isi) {
			n->offset = (ulong)l->offset / (ulong)r->offset;
			break;
		}
		return;

	case OMOD:
		if(isi) {
			n->offset = l->offset % r->offset;
			break;
		}
		return;

	case OLMOD:
		if(isi) {
			n->offset = (ulong)l->offset % (ulong)r->offset;
			break;
		}
		return;

	case OAND:
		if(isi) {
			n->offset = l->offset & r->offset;
			break;
		}
		return;

	case OOR:
		if(isi) {
			n->offset = l->offset | r->offset;
			break;
		}
		return;

	case OXOR:
		if(isi) {
			n->offset = l->offset ^ r->offset;
			break;
		}
		return;

	case OLSHR:
		if(isi) {
			n->offset = (ulong)l->offset >> (ulong)r->offset;
			break;
		}
		return;

	case OASHR:
		if(isi) {
			n->offset = l->offset >> r->offset;
			break;
		}
		return;

	case OASHL:
		if(isi) {
			n->offset = l->offset << r->offset;
			break;
		}
		return;

	case OLO:
		if(isi) {
			n->offset = (ulong)l->offset < (ulong)r->offset;
			break;
		}
		return;

	case OLT:
		if(isi) {
			n->offset = l->offset < r->offset;
			break;
		}
		if(isf) {
			n->offset = l->ud < r->ud;
			break;
		}
		return;

	case OHI:
		if(isi) {
			n->offset = (ulong)l->offset > (ulong)r->offset;
			break;
		}
		return;

	case OGT:
		if(isi) {
			n->offset = l->offset > r->offset;
			break;
		}
		if(isf) {
			n->offset = l->ud > r->ud;
			break;
		}
		return;

	case OLS:
		if(isi) {
			n->offset = (ulong)l->offset <= (ulong)r->offset;
			break;
		}
		return;

	case OLE:
		if(isi) {
			n->offset = l->offset <= r->offset;
			break;
		}
		if(isf) {
			n->offset = l->ud <= r->ud;
			break;
		}
		return;

	case OHS:
		if(isi) {
			n->offset = (ulong)l->offset >= (ulong)r->offset;
			break;
		}
		return;

	case OGE:
		if(isi) {
			n->offset = l->offset >= r->offset;
			break;
		}
		if(isf) {
			n->offset = l->ud >= r->ud;
			break;
		}
		return;

	case OEQ:
		if(isi) {
			n->offset = l->offset == r->offset;
			break;
		}
		if(isf) {
			n->offset = l->ud == r->ud;
			break;
		}
		return;

	case ONE:
		if(isi) {
			n->offset = l->offset != r->offset;
			break;
		}
		if(isf) {
			n->offset = l->ud != r->ud;
			break;
		}
		return;

	case ONOT:
		if(isi) {
			n->offset = !l->offset;
			break;
		}
		return;

	case OANDAND:
		if(isi) {
			n->offset = l->offset && r->offset;
			break;
		}
		return;

	case OOROR:
		if(isi) {
			n->offset = l->offset || r->offset;
			break;
		}
		return;
	}
	n->op = OCONST;
	v = castto(n->offset, n->type->etype);
	if(v != n->offset) {
		warn(n, "truncation in a constant expression");
		n->offset = v;
	}
}

#define	NTERM	10
struct	term
{
	long	mult;
	Node	*node;
} term[NTERM];
int	nterm;

void
acom(Node *n)
{
	Type *t;
	Node *l, *r;
	int i;

	switch(n->op)
	{

	case ONAME:
	case OCONST:
	case OSTRING:
	case OINDREG:
	case OREGISTER:
		return;

	case OADD:
	case OSUB:
	case OMUL:
		l = n->left;
		r = n->right;
		if(addo(n)) {
			if(addo(r))
				break;
			if(addo(l))
				break;
		}
		acom(l);
		acom(r);
		return;

	default:
		l = n->left;
		r = n->right;
		if(l != Z)
			acom(l);
		if(r != Z)
			acom(r);
		return;
	}

	/* bust terms out */
	t = n->type;
	term[0].mult = 0;
	term[0].node = Z;
	nterm = 1;
	acom1(1L, n);
	if(debug['a'])
	for(i=0; i<nterm; i++) {
		print("%d %3ld ", i, term[i].mult);
		prtree1(term[i].node, 1, 0);
	}
	if(nterm < NTERM)
		acom2(n, t);
	n->type = t;
}

acomcmp1(void *a1, void *a2)
{
	long c1, c2;
	struct term *t1, *t2;

	t1 = a1;
	t2 = a2;
	c1 = t1->mult;
	if(c1 < 0)
		c1 = -c1;
	c2 = t2->mult;
	if(c2 < 0)
		c2 = -c2;
	if(c1 > c2)
		return 1;
	if(c1 < c2)
		return -1;
	c1 = 1;
	if(t1->mult < 0)
		c1 = 0;
	c2 = 1;
	if(t2->mult < 0)
		c2 = 0;
	if(c2 -= c1)
		return c2;
	if(t2 > t1)
		return 1;
	return -1;
}

acomcmp2(void *a1, void *a2)
{
	long c1, c2;
	struct term *t1, *t2;

	t1 = a1;
	t2 = a2;
	c1 = t1->mult;
	c2 = t2->mult;
	if(c1 > c2)
		return 1;
	if(c1 < c2)
		return -1;
	if(t2 > t1)
		return 1;
	return -1;
}

void
acom2(Node *n, Type *t)
{
	Node *l, *r;
	struct term trm[NTERM];
	int k, nt, i, j;
	long c1, c2;

	/*
	 * copy into automatic
	 */
	c2 = 0;
	nt = nterm;
	for(i=0; i<nt; i++)
		trm[i] = term[i];
	/*
	 * recur on subtrees
	 */
	k = 0;
	for(i=1; i<nt; i++) {
		c1 = trm[i].mult;
		if(c1 == 0)
			continue;
		l = trm[i].node;
		if(l != Z) {
			k = 1;
			acom(l);
		}
	}
	c1 = trm[0].mult;
	if(k == 0) {
		n->op = OCONST;
		n->offset = c1;
		return;
	}
	k = ewidth[t->etype];
	/*
	 * prepare constant term,
	 * combine it with an addressing term
	 */
	if(c1 != 0) {
		l = new1(OCONST, Z, Z);
		l->type = t;
		l->offset = c1;
		trm[0].mult = 1;
		for(i=1; i<nt; i++) {
			if(trm[i].mult != 1)
				continue;
			r = trm[i].node;
			if(r->op != OADDR)
				continue;
			r->type = t;
			l = new1(OADD, r, l);
			l->type = t;
			trm[i].mult = 0;
			break;
		}
		trm[0].node = l;
	}
	/*
	 * look for factorable terms
	 * c1*i + c1*c2*j -> c1*(i + c2*j)
	 */
	qsort(trm+1, nt-1, sizeof(trm[0]), acomcmp1);
	for(i=nt-1; i>=0; i--) {
		c1 = trm[i].mult;
		if(c1 < 0)
			c1 = -c1;
		if(c1 <= 1)
			continue;
		for(j=i+1; j<nt; j++) {
			c2 = trm[j].mult;
			if(c2 < 0)
				c2 = -c2;
			if(c2 <= 1)
				continue;
			if(c2 % c1)
				continue;
			r = trm[j].node;
			if(ewidth[r->type->etype] != k) {
				r = new1(OCAST, r, Z);
				r->type = t;
			}
			c2 = trm[j].mult/trm[i].mult;
			if(c2 != 1 && c2 != -1) {
				r = new1(OMUL, r, new(OCONST, Z, Z));
				r->type = t;
				r->right->type = t;
				r->right->offset = c2;
			}
			l = trm[i].node;
			if(ewidth[l->type->etype] != k) {
				l = new1(OCAST, l, Z);
				l->type = t;
			}
			r = new1(OADD, l, r);
			r->type = t;
			if(c2 == -1)
				r->op = OSUB;
			trm[i].node = r;
			trm[j].mult = 0;
		}
	}
	if(debug['a']) {
		print("\n");
		for(i=0; i<nt; i++) {
			print("%d %3ld ", i, trm[i].mult);
			prtree1(trm[i].node, 1, 0);
		}
	}
	/*
	 * put it all back together
	 */
	qsort(trm+1, nt-1, sizeof(trm[0]), acomcmp2);
	l = Z;
	for(i=nt-1; i>=0; i--) {
		c1 = trm[i].mult;
		if(c1 == 0)
			continue;
		r = trm[i].node;
		if(ewidth[r->type->etype] != k || r->op == OBIT) {
			r = new1(OCAST, r, Z);
			r->type = t;
		}
		if(c1 != 1 && c1 != -1) {
			r = new1(OMUL, r, new(OCONST, Z, Z));
			r->type = t;
			r->right->type = t;
			if(c1 < 0) {
				r->right->offset = -c1;
				c1 = -1;
			} else {
				r->right->offset = c1;
				c1 = 1;
			}
		}
		if(l == Z) {
			l = r;
			c2 = c1;
			continue;
		}
		if(c1 < 0)
			if(c2 < 0)
				l = new1(OADD, l, r);
			else
				l = new1(OSUB, l, r);
		else
			if(c2 < 0) {
				l = new1(OSUB, r, l);
				c2 = 1;
			} else
				l = new1(OADD, l, r);
		l->type = t;
	}
	if(c2 < 0) {
		r = new1(OCONST, 0, 0);
		r->offset = 0;
		r->type = t;
		l = new1(OSUB, r, l);
		l->type = t;
	}
	*n = *l;
}

void
acom1(long v, Node *n)
{
	Node *l, *r;

	if(v == 0 || nterm >= NTERM)
		return;
	if(!addo(n)) {
		if(n->op == OCONST)
		if(typechlp[n->type->etype]) {
			term[0].mult += v*n->offset;
			return;
		}
		term[nterm].mult = v;
		term[nterm].node = n;
		nterm++;
		return;
	}
	switch(n->op) {

	case OCAST:
		acom1(v, n->left);
		break;

	case OADD:
		acom1(v, n->left);
		acom1(v, n->right);
		break;

	case OSUB:
		acom1(v, n->left);
		acom1(-v, n->right);
		break;

	case OMUL:
		l = n->left;
		r = n->right;
		if(l->op == OCONST)
		if(typechlp[n->type->etype]) {
			acom1(v*l->offset, r);
			break;
		}
		if(r->op == OCONST)
		if(typechlp[n->type->etype]) {
			acom1(v*r->offset, l);
			break;
		}

	default:
		diag(n, "not addo");
	}
}

addo(Node *n)
{

	if(n != Z)
	if(typechlp[n->type->etype])
	switch(n->op) {

	case OCAST:
		if(nilcast(n->left->type, n->type))
			return 1;
		break;

	case OADD:
	case OSUB:
		return 1;

	case OMUL:
		if(n->left->op == OCONST)
			return 1;
		if(n->right->op == OCONST)
			return 1;
	}
	return 0;
}

void
refinv(Sym *s)
{
	Ref *r, *r1;

	r = s->ref;
	s->ref = 0;

	while(r) {
		r1 = r;
		r = r->link;

		r1->link = s->ref;
		s->ref = r1;
	}
}

void
reftrace(void)
{
	char str[100], *p;
	Sym *s;
	Ref *r, *hr, *rr;
	int h, c;
	long l;

	for(h=0; h<NHASH; h++)
	for(s = hash[h]; s != S; s = s->link) {
		if(s->ref == 0)
			continue;
		refinv(s);

		/*
		 * look for HELP -> hr
		 */
		hr = 0;
		for(r = s->ref; r; r = r->link)
			if(r->class == CHELP)
				hr = r;
		if(hr == 0)
			continue;

		/*
		 * if no line/file args, print all
		 */
		if(hr->lineno == 0 && hr->sym == 0) {
			for(r = s->ref; r; r = r->link) {
				c = r->class;
				if(c == CHELP)
					continue;
				if(c >= CLAST) {
					print("*");
					c = c%CLAST;
				}
				print("%s <%uL>", cnames[c], r->lineno);
				if(r->dlineno)
					print(" <%uL>", r->dlineno);
				print("\n");
			}
			continue;
		}

		/*
		 * look for lineno that matches hr
		 */
		rr = 0;
		for(r = s->ref; r; r = r->link) {
			if(r->class == CHELP)
				continue;
			sprint(str, "%uL", r->lineno);
			p = utfrune(str, ':');
			if(p == 0)
				continue;
			*p++ = 0;
			l = atol(p);
			if(l != hr->lineno)
				continue;
			if(hr->sym) {
				p = utfrrune(str, '/');
				if(p == 0)
					p = str;
				if(strcmp(p, hr->sym->name) != 0)
					continue;
			}
			rr = r;
		}
		if(rr == 0)
			continue;

		/*
		 * look for definition
		 */
		l = rr->dlineno;
		c = rr->class % CLAST;
		hr = 0;
		for(r = s->ref; r; r = r->link) {
			if(r->class != c+CLAST)
				continue;
			if(l == 0) {
				hr = r;
				break;
			}
			if(r == rr || r->lineno == l) {
				hr = r;
				break;
			}
		}

		/*
		 * print definition
		 */
		if(hr != 0) {
			l = hr->lineno;
			print("%uL\n", hr->lineno);
		}

		/*
		 * print all refs
		 */
		for(r = s->ref; r; r = r->link) {
			if(r->class != c)
				continue;
			if(r->dlineno == 0 || r->dlineno == l)
				print("%uL\n", r->lineno);
		}
	}
}
