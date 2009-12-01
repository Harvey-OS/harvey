#include "gc.h"

int
swcmp(const void *a1, const void *a2)
{
	C1 *p1, *p2;

	p1 = (C1*)a1;
	p2 = (C1*)a2;
	if(p1->val < p2->val)
		return -1;
	return p1->val > p2->val;
}

void
doswit(Node *n)
{
	Case *c;
	C1 *q, *iq;
	long def, nc, i, isv;
	int dup;

	def = 0;
	nc = 0;
	isv = 0;
	for(c = cases; c->link != C; c = c->link) {
		if(c->def) {
			if(def)
				diag(n, "more than one default in switch");
			def = c->label;
			continue;
		}
		isv |= c->isv;
		nc++;
	}
	if(isv && !typev[n->type->etype])
		warn(n, "32-bit switch expression with 64-bit case constant");

	iq = alloc(nc*sizeof(C1));
	q = iq;
	for(c = cases; c->link != C; c = c->link) {
		if(c->def)
			continue;
		q->label = c->label;
		if(isv)
			q->val = c->val;
		else
			q->val = (long)c->val;	/* cast ensures correct value for 32-bit switch on 64-bit architecture */
		q++;
	}
	qsort(iq, nc, sizeof(C1), swcmp);
	if(debug['W'])
	for(i=0; i<nc; i++)
		print("case %2ld: = %.8llux\n", i, (vlong)iq[i].val);
	dup = 0;
	for(i=0; i<nc-1; i++)
		if(iq[i].val == iq[i+1].val) {
			diag(n, "duplicate cases in switch %lld", (vlong)iq[i].val);
			dup = 1;
		}
	if(dup)
		return;
	if(def == 0) {
		def = breakpc;
		nbreak++;
	}
	swit1(iq, nc, def, n);
}

void
casf(void)
{
	Case *c;

	c = alloc(sizeof(*c));
	c->link = cases;
	cases = c;
}

long
outlstring(ushort *s, long n)
{
	char buf[2];
	int c;
	long r;

	if(suppress)
		return nstring;
	while(nstring & 1)
		outstring("", 1);
	r = nstring;
	while(n > 0) {
		c = *s++;
		if(align(0, types[TCHAR], Aarg1)) {
			buf[0] = c>>8;
			buf[1] = c;
		} else {
			buf[0] = c;
			buf[1] = c>>8;
		}
		outstring(buf, 2);
		n -= sizeof(ushort);
	}
	return r;
}

void
nullwarn(Node *l, Node *r)
{
	warn(Z, "result of operation not used");
	if(l != Z)
		cgen(l, Z);
	if(r != Z)
		cgen(r, Z);
}

void
ieeedtod(Ieee *ieee, double native)
{
	double fr, ho, f;
	int exp;

	if(native < 0) {
		ieeedtod(ieee, -native);
		ieee->h |= 0x80000000L;
		return;
	}
	if(native == 0) {
		ieee->l = 0;
		ieee->h = 0;
		return;
	}
	fr = frexp(native, &exp);
	f = 2097152L;		/* shouldnt use fp constants here */
	fr = modf(fr*f, &ho);
	ieee->h = ho;
	ieee->h &= 0xfffffL;
	ieee->h |= (exp+1022L) << 20;
	f = 65536L;
	fr = modf(fr*f, &ho);
	ieee->l = ho;
	ieee->l <<= 16;
	ieee->l |= (long)(fr*f);
}
