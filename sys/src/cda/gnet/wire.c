#include "geom.h"
#include "thing.h"
#include "text.h"
#include "wire.h"
#include "box.h"

Wire::Wire(Rectangle r)
{
	register t;
	R = r;
	if (Y == V && U < X) {		/* canonicalize if possible */
		t = U;
		U = X;
		X = t;
	}
	else if (X == U && V < Y) {
		t = V;
		V = Y;
		Y = t;
	}
	net = 0;
}

int Wire::contains(Point p)
{
	if (X == U || Y == V)
		return p <= R;
	return p == P || p == Q;
/* this looks nicer but <= assumes R can be canonicalized
	return (p <= R) && (Y == V || X == U || p == P || p == Q);
*/
}

int Wire::prop(register Wire *w)
{
	if (net == 0)		/* unassigned wires do not propagate */
		fprintf(stderr,"un-netted wire asked to propagate\n");
	if (w->contains(P) || w->contains(Q) || contains(w->P) || contains(w->Q)) {
		if (w->net == 0) {
			w->net = net;
			return 1;	/* mission accomplished */
		}
		if (w->net != net)
			fprintf(stderr,"nets %s and %s shorted\n",net->s->s,w->net->s->s);
		return 0;
	}
	else
		return 0;
}

int Wire::namenet(register Text *t)
{
	if (contains(t->p)) {
		if (net != 0 && !net->s->eq(t->s))
			fprintf(stderr,"wire touches nets %s and %s\n",
				net->s->s,t->s->s);
		net = t;
		return 1;
	}
	return 0;
}

void Wire::namebox(register Box *b)
{
	Point p;
	if (P < b->R)
		p = P;
	else if (Q < b->R)
		p = Q;
	else
		return;
	net->p = p;		/* careful, we're messing with someone else */
	b->namepart(net);	/* so we get the box to think he owns it */
}

WireList::partition()
{
	register i=0,j=n-1;
	while (i < j) {
		while (i < j && wire(i)->net != 0)
			i++;
		while (wire(j)->net == 0)
			j--;
		if (i < j)
			exchg(i,j);
	}
	if (wire(i)->net == 0)
		return i;
	return i+1;
}

void WireList::prop()
{
	register i,j;
	int split;
	split = partition();
	for (i = 0; i < split && i < n; i++) {
		for (j = split; j < n; j++) {
			if (wire(i)->prop(wire(j))) {
				if (j > split)
					exchg(split,j);
				split++;
			}
		}
	}
}

void WireList::prop1()	/* make up net names this time */
{
	register i,j;
	int split,net=0;
	char madeup[20];
	String *lab;
	split = partition();
	for (i = split; i < n; i++) {
		if (i == split) {
			sprintf(madeup,"$N%04d",net++);
			lab = new String(madeup);
			wire(i)->net = new Text(Point(0,0),lab,lab,0);
			split++;
		}
		for (j = split; j < n; j++) {
			if (wire(i)->prop(wire(j))) {
				if (j > split)
					exchg(split,j);
				split++;
			}
		}
	}
}

int WireList::nets(Text *t)
{
	register i,j;
	for (i = 0, j = 0; i < n; i++)
		j |= wire(i)->namenet(t);
	return j;
}

void Wire::put(FILE *ouf)
{
	fprintf(ouf,"wire: %d %d %d %d %s\n",R.o.x,R.o.y,R.c.x,R.c.y,(net == 0) ? "" : net->s->s);
}
