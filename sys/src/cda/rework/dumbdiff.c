#include	"rework.h"

static dbg;

void
Net::dumbdiff(Net *old)
{
	register Wire *n, *o;
	int statusquo;
	int first;

	dbg = DEBUG(name) || DEBUG(old->name);
	/* so dumb we don't have to order! */
	if(dbg){
		fprintf(stdout, "dumbdiff:\nold=%s\n", old->name);
		old->pr(stdout);
		fprintf(stdout, "new=%s\n", name);
		pr(stdout);
	}
	if(dbg)
		fprintf(stdout, "delete unnecessary old wires\n");
	OVER(o, old)
		if(havewire(o) == 0)
			old->delwire(o);
	/*
		flip levels??
	*/
	statusquo = 0;
	OVER(n, this){
		if(o = old->havewire(n))
			if(o->lev() == n->lev())
				statusquo++;
			else
				statusquo--;
	}
	if(statusquo < 0){
		OVER(n, this)
			n->flip();
	}
	if(dbg)
		fprintf(stdout, "deleting old wires at wrong level\n");
	OVER(o, old){
		n = havewire(o);
		if(n->lev() != o->lev())
			old->delwire(o);
	}
	if(dbg)
		fprintf(stdout, "adding missing new wires\n");
	OVER(n, this)
		if(old->havewire(n) == 0)
			old->addwire(n);
	old->order();
	/*
		check for new level 1's underneath old level 2
	*/
	if(dbg)
		fprintf(stdout, "add/remove old level2s over new level1s\n");
	first = 1;
	OVER(o, old){
		if(first){
			first = 0;
			continue;
		}
		if(o->newish && !o[-1].newish && (o->lev() == 1) && (o[-1].lev() == 2)){
			o[-1].unpr();
			o[-1].pr(RE);
			if(dbg) o[-1].pr(stdout);
			continue;
		}

		if(!o->newish && o[-1].newish && (o->lev() == 2) && (o[-1].lev() == 1)){
			o->unpr();
			o->pr(RE);
			if(dbg) o->pr(stdout);
			continue;
		}
	}
	if(dbg){
		fprintf(stdout, "output:\n");
		old->pr(stdout, name);
	}
	old->pr(NEW, name);
	done = 1;
	old->done = 1;
}

void
Net::addwire(Wire *w)
{
	Wire *ww;

	if(nwires == nalloc){
		nalloc += 10;
		ww = new Wire[nalloc];
		memcpy((char *)ww, (char *)wires, sizeof(Wire)*nwires);
		delete wires;
		wires = ww;
	}
	w->newish = 1;
	wires[nwires++] = *w;
	if(dbg)
		w->pr(stdout);
	w->pr(RE);
}

void
Net::delwire(Wire *w)
{
	Wire *b, *e;

	b = e = w;
	if(w->lev() == 1){	/* check for level twos on either side */
		if((w < &wires[nwires-1])&&(w->x2 == w[1].x1)&&(w->y2 == w[1].y1))
			e++;
		if((w > wires)&&(w->x1 == w[-1].x2)&&(w->y1 == w[-1].y2))
			b--;
	}
	if(dbg){
		fprintf(stdout, "vvvvv\n");
		for(w = b; w <= e; w++)
			w->pr(stdout);
		fprintf(stdout, "^^^^^\n");
	}
	/* unwraps have to be flipped so as to get the wirewrap machine roght */
	for(w = b; w <= e; w++)
		w->unpr();
	if(step > b)
		step = b;
	if(e != &wires[nwires])
		memcpy((char *)b, (char *)(e+1), sizeof(Wire)*(&wires[nwires-1]-e));
	nwires -= 1+(e-b);
}

Wire *
Net::havewire(register Wire *ww)
{
	register i;
	register Wire *w;

	for(w = wires, i = 0; i < nwires; i++, w++)
		if(	((w->x1 == ww->x1)&&(w->y1 == ww->y1)&&(w->x2 == ww->x2)&&(w->y2 == ww->y2))
		||	((w->x1 == ww->x2)&&(w->y1 == ww->y2)&&(w->x2 == ww->x1)&&(w->y2 == ww->y1))
		) return(w);
	return((Wire *)0);
}
