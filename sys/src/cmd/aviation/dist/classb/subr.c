#include "tca.h"

double
anorm(double a)
{

	while(a < 0)
		a += 360;
	while(a >= 360)
		a -= 360;
	return a;
}

void
pdiag(Node *n1, Node *n2)
{
	double t, g;

	t = n1->vleft - n2->vleft;
	if(t < 0)
		t = -t;
	g = n1->vright - n2->vright;
	if(g < 0)
		g = -g;

	if(t > g) {
		if(n1->vleft > n2->vleft)
			fprint(2, "/[lr]~/s//n/\n");
		else
			fprint(2, "/[lr]~/s//s/\n");
	} else
		if(n1->vright < n2->vright)
			fprint(2, "/[lr]~/s//e/\n");
		else
			fprint(2, "/[lr]~/s//w/\n");
}

Node*
xcc1(Node *l, double rl, Node *r, double rr, double a)
{
	double ia, ba, bd, d;
	Node *t;
	int i;

	ia = 180./10.;
	ba = anorm(a + ia/2);

search:
	a = ba;
	bd = 1.e20;
	for(i=0; i<10; i++) {
		t = dme(l, a, rl);
		d = dist(t, r) - rr;
		if(d < 0)
			d = -d;
		if(d < bd) {
			bd = d;
			ba = a;
		}
		free(t);
		a = anorm(a + ia);
	}
	if(ia > 1.e-17) {
		ba = anorm(ba - ia/2);
		ia = ia/5;
		ba = anorm(ba + ia/2);
		goto search;
	}

	t = dme(l, ba, rl);
	return t;
}

Node*
xcc(Node *n)
{
	double rl, rr;
	Node *l, *r;
	Node *n1, *n2;
	double a;
	int dir;

	dir = n->ileft;
	n = n->nright;

	l = eval(n->nleft->nleft);
	r = eval(n->nright->nleft);
	if(l == 0 || r == 0)
		return 0;

	rl = n->nleft->vright;
	rr = n->nright->vright;

	a = angl(l, r);
	n1 = xcc1(l, rl, r, rr, a);
	n2 = xcc1(l, rl, r, rr, a+180);
	switch(dir) {
	case 'l':
		pdiag(n2, n1);
		return n2;
	case 'r':
		pdiag(n1, n2);
		return n1;
	case 'n':
		if(n1->vleft > n2->vleft)
			return n1;
		return n2;
	case 's':
		if(n2->vleft > n1->vleft)
			return n1;
		return n2;
	case 'e':
		if(n1->vright < n2->vright)
			return n1;
		return n2;
	case 'w':
		if(n2->vright < n1->vright)
			return n1;
		return n2;
	}
}

Node*
xcl1(Node *l, double rl, Node *r, double ar, double a)
{
	Node *t;
	double ba, ia, d, bd;
	int i;

	ia = 180./10.;
	a = anorm(a+ia/2);

search:
	bd = 1.e20;
	ba = 0;
	for(i=0; i<10; i++) {
		t = dme(l, a, rl);
		d = angl(r, t);
		d = adiff(d - ar);
		if(d < bd) {
			bd = d;
			ba = a;
		}
		free(t);
		a = anorm(a + ia);
	}
	if(ia > 1.e-17) {
		a = anorm(ba - ia/2);
		ia = ia/5;
		a = anorm(a + ia/2);
		goto search;
	}

	t = dme(l, ba, rl);
	return t;
}

Node*
xcl(Node *n)
{
	double rl, ar;
	Node *l, *r, *n1, *n2;
	int dir;

	dir = n->ileft;
	n = n->nright;

	l = eval(n->nleft->nleft);
	r = eval(n->nright->nleft);
	if(l == 0 || r == 0)
		return 0;

	rl = n->nleft->vright;
	ar = anorm(n->nright->vright);
	if(ar >= 180)
		ar -= 180;

	n1 = xcl1(l, rl, r, ar, ar+90);
	n2 = xcl1(l, rl, r, ar, ar+90+180);
	switch(dir) {
	case 'l':
		pdiag(n2, n1);
		return n2;
	case 'r':
		pdiag(n1, n2);
		return n1;
	case 'n':
		if(n1->vleft > n2->vleft)
			return n1;
		return n2;
	case 's':
		if(n2->vleft > n1->vleft)
			return n1;
		return n2;
	case 'e':
		if(n1->vright < n2->vright)
			return n1;
		return n2;
	case 'w':
		if(n2->vright < n1->vright)
			return n1;
		return n2;
	}
}

int
betwn(Node *n1, double a1, Node *n2, double a2, double d0, double d1)
{
	Node *t1, *t2;
	double ax, ay;

	t1 = dme(n1, a1, d0);
	t2 = dme(n1, a1, d1);
	ax = anorm(angl(n2, t1) - a2);
	ay = anorm(angl(n2, t2) - a2);
	free(t1);
	free(t2);
	if(ax >= 180) {
		if(ay < 180)
			return 1;
	} else
		if(ay >= 180)
			return 1;
	return 0;
}

Node*
xll(Node *n)
{
	double a1, a2;
	Node *l, *r, *t;
	double b0, id;

	l = eval(n->nleft->nleft);
	r = eval(n->nright->nleft);
	if(l == 0 || r == 0)
		return 0;

	a1 = anorm(n->nleft->vright);
	a2 = anorm(n->nright->vright);
	if(a1 >= 180)
		a1 -= 180;
	if(a2 >= 180)
		a2 -= 180;

	b0 = 0;
	id = 500;

	if(!betwn(l, a1, r, a2, b0, b0+id)) {
		a1 += 180;
		if(!betwn(l, a1, r, a2, b0, b0+id)) {
			fprint(2, "xll oops\n");
			return 0;
		}
	}
		
search:
	id = id/2;
	if(id > 1.e-17) {
		if(!betwn(l, a1, r, a2, b0, b0+id))
			b0 += id;
		goto search;
	}
	t = dme(l, a1, b0);
/*
	fprint(2, "x %.4f %.4f %.4f %.4f\n",
		t->vleft, t->vright,
		angl(l, t),
		angl(r, t));
/**/
	return t;
}

Node*
dme(Node *p, double aa, double ad)
{
	double u, t, lat, lng, a, d, v;
	Node *n;

	aa = anorm(aa);
	lat = p->vleft*PI/180;
	lng = p->vright*PI/180;
	d = ad/RADE;
	a = aa*PI/180;
	if(a >= PI)
		a = 2*PI - a;

	if(ad < 1.0e-3)
		goto out;
	t = fabs(a);
	if(t < 1.0e-10) {
		lat = lat + d;
		goto out;
	}
	t = fabs(PI - t);
	if(t < 1.0e-10) {
		lat = lat - d;
		goto out;
	}

	v = PIO2 - lat;
	if(v < 0)
		v += PI*2;
	t = sphere(UNKNOWN, d, v,
		a, MISSING, MISSING);
	u = PIO2 - t;
	t = sphere(t, d, v,
		a, UNKNOWN, MISSING);
	if(aa <= 180)
		lng = lng - t;
	else
		lng = lng + t;
	lat = u;

out:
	n = new(Opoint);
	n->vleft = lat*180/PI;
	n->vright = lng*180/PI;
	return n;
}

double
dist(Node *p1, Node *p2)
{
	double a, lat1, lat2, lng1, lng2;

	lat1 = p1->vleft*PI/180;
	lng1 = p1->vright*PI/180;
	lat2 = p2->vleft*PI/180;
	lng2 = p2->vright*PI/180;

	a = sin(lat1) * sin(lat2) +
		cos(lat1) * cos(lat2) *
		cos(lng1 - lng2);
	a = atan2(xsqrt(1 - a*a), a);
	if(a < 0)
		a = -a;
	return a * RADE;
}

double
angl(Node *p1, Node *p2)
{
	double a, lat1, lat2, lng1, lng2;

	lat1 = p1->vleft*PI/180;
	lng1 = p1->vright*PI/180;
	lat2 = p2->vleft*PI/180;
	lng2 = p2->vright*PI/180;

	a = sphere(PIO2-lat2, PIO2-lat1, MISSING,
		UNKNOWN, MISSING, fabs(lng1-lng2));
	if(lng1 < lng2)
		a = 2*PI - a;
	return anorm(a*180/PI);
}

double
adiff(double a)
{
	if(a < 0)
		a = -a;
	while(a >= 180)
		a -= 180;
	if(a >= 90)
		a = 180-a;
	return a;
}

double
xsqrt(double a)
{

	if(a < 0)
		return 0;
	return sqrt(a);
}

void
outstring(String *s)
{
	int i;

	for(i=0; i<s->len; i++)
		if(s->val[i] == '_')
			s->val[i] = ' ';
	gname = *s;
}

void
outarc(int dir, Node *p1, Node *p2, Node *p3)
{
	int n, i;
	long v, ov;
	double a, ia, da, r;
	Node *t;

	p1 = eval(p1);
	p2 = eval(p2);
	p3 = eval(p3);
	if(p1 == 0 || p2 == 0 || p3 == 0)
		return;

	print("1\n");
	print("c%s\n", header);
	if(gname.len)
		print("n%.*s\n", gname.len, gname.val);
	r = dist(p1, p2);
	if(r <= 0)
		return;
	ia = angl(p1, p2);
	da = angl(p1, p3);
	da = anorm(da-ia);
	if(da <= 0)
		return;
	if(dir == 'r') {
		n = r * da*PI/180 * grain;
		if(n <= 0)
			n = 1;
	} else {
		da = 360 - da;
		n = r * da*PI/180 * grain;
		if(n <= 0)
			n = 1;
		da = -da;
	}
	da = da/n;

	ov = 0;
	for(i=0,a=ia; i<=n; i++,a+=da) {
		t = dme(p1, a, r);
		v = t->vleft * 5e6;
		if(i == 0)
			print("u%ld\n", v);
		else
			print("d%ld\n", v-ov);
		free(t);
		ov = v;
	}

	for(i=0,a=ia; i<=n; i++,a+=da) {
		t = dme(p1, a, r);
		v = (360-t->vright) * 5e6;
		if(i == 0)
			print("h%ld\n", v);
		else
			print("d%ld\n", v-ov);
		free(t);
		ov = v;
	}
}

void
outcirc(Node *p, double r)
{
	int n, i;
	long v, ov;
	double a, da;
	Node *t;

	p = eval(p);
	if(p == 0)
		return;

	print("1\n");
	print("c%s\n", header);
	if(gname.len)
		print("n%.*s\n", gname.len, gname.val);

	da = 360;
	n = r * da*PI/180 * grain;
	da = da/n;

	ov = 0;
	for(i=0,a=1; i<=n; i++,a+=da) {
		t = dme(p, a, r);
		v = t->vleft * 5e6;
		if(i == 0)
			print("u%ld\n", v);
		else
			print("d%ld\n", v-ov);
		free(t);
		ov = v;
	}

	for(i=0,a=1; i<=n; i++,a+=da) {
		t = dme(p, a, r);
		v = (360-t->vright) * 5e6;
		if(i == 0)
			print("h%ld\n", v);
		else
			print("d%ld\n", v-ov);
		free(t);
		ov = v;
	}
}

void
outsegment(Node *p1, Node *p2)
{
	int n, i;
	long v, ov;
	double a, dd, d;
	Node *t;

	print("1\n");
	print("c%s\n", header);
	if(gname.len)
		print("n%.*s\n", gname.len, gname.val);

	p1 = eval(p1);
	p2 = eval(p2);
	if(p1 == 0 || p2 == 0)
		return;

	a = angl(p1, p2);
	dd = dist(p1, p2);
	if(dd <= 0)
		return;
	n = dd * grain;
	if(n <= 0)
		n = 1;
	dd = dd/n;

	ov = 0;
	for(i=0,d=0; i<=n; i++,d+=dd) {
		t = dme(p1, a, d);
		v = t->vleft * 5e6;
		if(i == 0)
			print("u%ld\n", v);
		else
			print("d%ld\n", v-ov);
		free(t);
		ov = v;
	}

	for(i=0,d=0; i<=n; i++,d+=dd) {
		t = dme(p1, a, d);
		v = (360-t->vright) * 5e6;
		if(i == 0)
			print("h%ld\n", v);
		else
			print("d%ld\n", v-ov);
		free(t);
		ov = v;
	}
}
