#include "acd.h"

Toc thetoc;

void
tocthread(void *v)
{
	Drive *d;

	threadsetname("tocthread");
	d = v;
	DPRINT(2, "recv ctocdisp?...");
	while(recv(d->ctocdisp, &thetoc) == 1) {
		DPRINT(2, "recv ctocdisp!...");
		drawtoc(d->w, &thetoc);
		DPRINT(2, "send dbreq...\n");
		send(d->ctocdbreq, &thetoc);
	}
}

void
freetoc(Toc *t)
{
	int i;

	free(t->title);
	for(i=0; i<t->ntrack; i++)
		free(t->track[i].title);
}

void
cddbthread(void *v)
{
	Drive *d;
	Toc t;

	threadsetname("cddbthread");
	d = v;
	while(recv(d->ctocdbreply, &t) == 1) {
		if(thetoc.nchange == t.nchange) {
			freetoc(&thetoc);
			thetoc = t;
			redrawtoc(d->w, &thetoc);
		}
	}
}

void
cdstatusthread(void *v)
{
	Drive *d;
	Cdstatus s;

	d = v;
	
	for(;;)
		recv(d->cstat, &s);

}
