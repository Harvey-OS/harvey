#include	"rework.h"

void
Net::eqelim()
{
	register Net *o;
	register Wire *w = wires;
	register i;

	o = pins.look(PT(w->x1, w->y1));
	if(o == 0)
		return;
	if(nwires != o->nwires)
		return;
	for(i = 0; i < nwires+1; i++)
		if(!o->in(pts[i]))
			return;
	done = 1;
	o->done = 1;
	if(!verbose){
		if(oflag)
			o->order();
		o->pr(NEW, name);
	}
}
