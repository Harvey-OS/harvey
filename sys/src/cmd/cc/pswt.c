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
	C1 *q, *iq, *iqh, *iql;
	long def, nc, i, j, isv, nh;
	Prog *hsb;
	Node *vr[2];
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
	if(typev[n->type->etype])
		isv = 1;
	else if(isv){
		warn(n, "32-bit switch expression with 64-bit case constant");
		isv = 0;
	}

	iq = alloc(nc*sizeof(C1));
	q = iq;
	for(c = cases; c->link != C; c = c->link) {
		if(c->def)
			continue;
		if(c->isv && !isv)
			continue;	/* can never match */
		q->label = c->label;
		if(isv)
			q->val = c->val;
		else
			q->val = (long)c->val;	/* cast ensures correct value for 32-bit switch on 64-bit architecture */
		q++;
	}
	qsort(iq, nc, sizeof(C1), swcmp);
	if(debug['K'])
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
	if(!isv || ewidth[TIND] > ewidth[TLONG] || n->op == OREGISTER) {
		swit1(iq, nc, def, n);
		return;
	}

	/*
	 * 64-bit case on 32-bit machine:
	 * switch on high-order words, and
	 * in each of those, switch on low-order words
	 */
	if(n->op != OREGPAIR)
		fatal(n, "internal: expected register pair");
	if(thechar == '8'){	/* TO DO: need an enquiry function */
		vr[0] = n->left;	/* low */
		vr[1] = n->right;	/* high */
	}else{
		vr[0] = n->right;
		vr[1] = n->left;
	}
	vr[0]->type = types[TLONG];
	vr[1]->type = types[TLONG];
	gbranch(OGOTO);
	hsb = p;
	iqh = alloc(nc*sizeof(C1));
	iql = alloc(nc*sizeof(C1));
	nh = 0;
	for(i=0; i<nc;){
		iqh[nh].val = iq[i].val >> 32;
		q = iql;
		/* iq is sorted, so equal top halves are adjacent */
		for(j = i; j < nc; j++){
			if((iq[j].val>>32) != iqh[nh].val)
				break;
			q->val = (long)iq[j].val;
			q->label = iq[j].label;
			q++;
		}
		qsort(iql,  q-iql, sizeof(C1), swcmp);
if(0){for(int k=0; k<(q-iql); k++)print("nh=%ld k=%d h=%#llux l=%#llux lab=%ld\n", nh, k, (vlong)iqh[nh].val,  (vlong)iql[k].val, iql[k].label);}
		iqh[nh].label = pc;
		nh++;
		swit1(iql, q-iql, def, vr[0]);
		i = j;
	}
	patch(hsb, pc);
if(0){for(int k=0; k<nh; k++)print("k*=%d h=%#llux lab=%ld\n", k, (vlong)iqh[k].val,  iqh[k].label);}
	swit1(iqh, nh, def, vr[1]);
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
outlstring(TRune *s, long n)
{
	char buf[sizeof(TRune)];
	uint c;
	int i;
	long r;

	if(suppress)
		return nstring;
	while(nstring & (sizeof(TRune)-1))
		outstring("", 1);
	r = nstring;
	while(n > 0) {
		c = *s++;
		if(align(0, types[TCHAR], Aarg1)) {
			for(i = 0; i < sizeof(TRune); i++)
				buf[i] = c>>(8*(sizeof(TRune) - i - 1));
		} else {
			for(i = 0; i < sizeof(TRune); i++)
				buf[i] = c>>(8*i);
		}
		outstring(buf, sizeof(TRune));
		n -= sizeof(TRune);
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
