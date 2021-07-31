#include	"rework.h"

static dbg;

struct Pt
{
	int x, y;
	char *s;
	char done;
};

class Ptset
{
public:
	Ptset() { npt = 0; }
	int npt;
	Pt p[NPTS];
	long lastbest;
	void add(int, int, char *);
	int best(int);
};

void
Net::quickdiff(Net *old)
{
	register Wire *n;
	int i, lev, j, last;
	Ptset p;
	char *len;

	dbg = DEBUG(name) || DEBUG(old->name);
	/*
		first see if we can do a quickdiff!
	*/
	dsame(nwires+1, pts, old->nwires+1, old->pts);
	for(i = old->nwires; i >= 0; i--)
		if(old->pts[i] >= 0){	/* would have to delete */
			dumbdiff(old);
			return;
		}
	if(dbg){
		fprintf(stdout, "quickdiff:\nold=%s\n", old->name);
		old->pr(stdout);
		fprintf(stdout, "new=%s\n", name);
		pr(stdout);
	}
	/*
		build point list
	*/
	for(i = 0; i < nwires+1; i++)
		if(pts[i] >= 0){
			OVER(n, this)
				if(PT(n->x1, n->y1) == pts[i]){
					p.add(n->x1, n->y1, n->f5);
					break;
				} else if(PT(n->x2, n->y2) == pts[i]){
					p.add(n->x2, n->y2, n->f6);
					break;
				}
		}
	if(dbg){
		fprintf(stdout, "adding these points\n");
		for(i = 0; i < p.npt; i++)
			fprintf(stdout, "\t%d/%d %s\n", p.p[i].x, p.p[i].y, p.p[i].s);
	}
	/*
		find starting point, put it at p.npt
	*/
	old->order();
	if(old->wires[0].lev() == old->wires[old->nwires-1].lev()){
		long ll;

		/* find closest */
		n = &old->wires[0];
		p.add(n->x1, n->y1, n->f5);
		p.best(p.npt-1);
		ll = p.lastbest;
		n = &old->wires[old->nwires-1];
		p.add(n->x2, n->y2, n->f6);
		p.best(p.npt-1);
		if(p.lastbest < ll)
			p.p[p.npt-2] = p.p[p.npt-1];
		else
			n = &old->wires[0];
		p.npt--;
		n->unpr();
		n->pr(RE);
	} else if(old->wires[0].lev() == 1){
		n = &old->wires[0];
		p.add(n->x1, n->y1, n->f5);
	} else {
		n = &old->wires[old->nwires-1];
		p.add(n->x2, n->y2, n->f6);
	}
	p.npt--;
	if(dbg)
		fprintf(stdout, "adding at %d/%d\n", p.p[p.npt].x, p.p[p.npt].y);
	lev = 3-n->lev();
	for(i = 0, last = p.npt, p.p[last].done = 1; i < p.npt; i++){
		j = p.best(last);
		len = new char[10];
		sprintf(len, "%d", abs(p.p[last].x-p.p[j].x)+abs(p.p[last].y-p.p[j].y));
		n = new Wire;
		n->set(old->wires[0].f1, lev, old->wires[0].f3, len, p.p[last].x, p.p[last].y,
			p.p[last].s, p.p[j].x, p.p[j].y, p.p[j].s);
		old->addwire(n);
		p.p[j].done = 1;
		last = j;
		lev = 3-lev;
	}
	old->order();
	if(dbg){
		fprintf(stdout, "output:\n");
		old->pr(stdout, name);
	}
	old->pr(NEW, name);
	done = 1;
	old->done = 1;
}

void
Ptset::add(int a, int b, char *c)
{
	p[npt].x = a;
	p[npt].y = b;
	p[npt].s = c;
	p[npt].done = 0;
	npt++;
}

Ptset::best(int o)
{
	register Pt *z, *op = p+o, *bestz = 0;
	register long d;

	lastbest = 999999999L;
	for(z = &p[npt]; z >= p; z--){
		if(z == op) continue;
		if(z->done) continue;
		d = abs(z->x-op->x)+abs(z->y-op->y);
		if(d < lastbest)
			lastbest = d, bestz = z;
	}
	if(bestz == 0){
		fprintf(stderr, "ALERT!! best screwup %d\n", o);
		exit(1);
	}
	return(bestz-p);
}
