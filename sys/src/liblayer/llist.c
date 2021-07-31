#include <u.h>
#include <libg.h>
#include <layer.h>

void
_ldelete(Layer *l)
{
	Layer *tl;

	tl = l->cover->front;
	if(tl == l)
		l->cover->front = l->next;
	else{
		while(tl->next!=l)
			tl = tl->next;
		tl->next = l->next;
	}
}

void
_linsertfront(Layer *l)
{
	l->next = l->cover->front;
	l->cover->front = l;
}

void
_linsertback(Layer *l)
{
	Layer *tl;

	tl = l->cover->front;
	if(tl == 0)
		l->cover->front = l;
	else{
		while(tl->next)
			tl = tl->next;
		tl->next = l;
	}
	l->next = 0;
}
