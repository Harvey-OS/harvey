#include	"rework.h"

void
Net::order()
{
	char done[NPTS];
	Wire ww[NPTS];
	int didwork;
	register Wire *w, *head = &ww[NPTS/2], *tail = head;
	register i;

	*head = wires[0];
	memset(done, 0, nwires);
	for(didwork = 1; didwork;){
		didwork = 0;
		for(i = 1, w = wires+i; i < nwires; i++, w++){
			if(done[i]) continue;
			if((w->x1 == tail->x2) && (w->y1 == tail->y2))
				*++tail = *w, done[i] = 1;
			else if((w->x2 == tail->x2) && (w->y2 == tail->y2))
				w->reverse(), *++tail = *w, done[i] = 1;
			if((w->x2 == head->x1) && (w->y2 == head->y1))
				*--head = *w, done[i] = 1;
			else if((w->x1 == head->x1) && (w->y1 == head->y1))
				w->reverse(), *--head = *w, done[i] = 1;
			if(done[i])
				didwork = 1;
		}
	}
	if((tail-head) != nwires-1){
		fprintf(stderr, "discontinuous net %s\n", name);
		fprintf(stderr, "trying to add\n");
		for(i = 1; done[i]; i++)
			;
		wires[i].pr(stderr);
		fprintf(stderr, "to set starting at\n");
		head->pr(stderr);
		fprintf(stderr, "and ending with\n");
		tail->pr(stderr);
		fprintf(stderr, "whole net is:\n");
		pr(stderr);
		exit(1);
	}
	/*
		set start/stop bits
	*/
	for(w = head, i = 3-head->lev(); w <= tail; w++)
		w->f2 &= ~STARTSTOP;
	head->f2 |= START;
	tail->f2 |= STOP;
	memcpy((char *)wires, (char *)head, sizeof(Wire)*nwires);
	/*
		check levels (paranoid)
	*/
	for(w = head, i = 3-head->lev(); w <= tail; w++){
		if((3-i) != w->lev()){
			fprintf(stderr, "internal error: net %s\n", name);
			w->pr(stderr);
			fprintf(stderr, "whole net:\n");
			pr(stderr);
			exit(1);
		}
		i = 3-i;
	}
}
