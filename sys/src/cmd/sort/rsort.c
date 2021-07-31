/* Copyright 1990, AT&T Bell Labs */
#include "fsort.h"

/* Radix sort by bytes.  Records are chained in a list.
   There are up to 257 buckets at each recursion level.
   Bucket 0, the "outlier", is for strings with no byte
   to index on, buckets 1-256 for byte values 0-255.*/

int	uflag = 0;
int	rflag = 0;

/* Sort the list s by distributing according to
   digit d into an array of buckets, sorting the buckets
   recursively, and re-collecting them back into the list.
   If uflag is nonzero return only 1 representative of
   each equivalence class.  The squeeze step noted
   below determines what buckets are in use, and squeezes
   those into a stack frame, which is contiguous to s.
   The "outlier" is handled separately to cut the span
   of the squeeze loop.
   Precondition: length(s) > 1 */

#define tiebreak(t)				\
	if(uflag) {				\
		t->tail = t->head;		\
		t->head->next = 0;		\
	} else					\
	if(keyed && t->head->next) {		\
		rflag = fields[0].rflag;	\
		keyed = 0;			\
		sort(t, 0);			\
		keyed = 1;			\
		rflag = 0;			\
	} else

void
sort(List *s, int d)
{
	List *t;
	Rec *r, *q;
	List *frame = s + 1;		/* stack frame */
	static List buck[257];		/* buckets */
	int nbuck;			/* number of filled buckets */
	List *low;			/* lowest unsorted bucket */

loop:
	r = s->head;
	low = buck + 257;
	nbuck = 0;
	while(r) {
#		define bucket(fieldlen, field)			\
			if(r->fieldlen > d) {			\
				t = buck + 1 + field[d];	\
				if(low > t)			\
					low = t;		\
			} else					\
				t = buck
		if(keyed)
			bucket(klen, key(r));
		else
			bucket(dlen, data(r));
		if(t->head == 0) {
			t->head = r;
			nbuck++;
		} else
			t->tail->next = r;	/* stable sort */
		t->tail = r;
		r = r->next;
	}
	t = frame;			/* t is stack top */
	if(t+nbuck > stackmax)
		fatal("stack overflow", "", 0);
#	define copy(b)				\
		if(b->head) {			\
			*t = *b;		\
			t->tail->next = 0;	\
			t++;			\
			b->head = 0;		\
			nbuck--;		\
		} else
	copy(buck);			/* outlier */
	for(; nbuck>0; low++)		/* squeeze */
		copy(low);
	if(t == frame+1) {		/* tail recursion */
/*		debug(frame, t, d);*/
		r = frame->head;
		if(r->len[keyed] <= d) {
			tiebreak(frame);
			*s = *frame;
			return;		/* string ran out */
		}
		d++;
		goto loop;
	}
/*	debug(frame, t, d);*/
	t--;
	if(t->head->next)		/* skip 1-item list */
		sort(t, d+1);
	if(!rflag) {			/* forward order */
		r = t->tail;
		while(t > frame) {	
			q = t->head;
			if((--t>frame || t->head->len[keyed]>d)
			   && t->head->next)
				sort(t, d+1);
			else
			if(t==frame)
				tiebreak(t);
			t->tail->next = q;	/* concatenate */
		}
		s->head = frame->head;
		s->tail = r;
	} else {			/* reverse order */
		r = t->head;
		while(t > frame) {	
			q = t->tail;
			if((--t>frame || t->head->len[keyed]>d)
			   && t->head->next)
				sort(t, d+1);
			else
			if(t==frame)
				tiebreak(t);
			q->next = t->head;	/* concatenate */
		}
		s->head = r;
		s->tail = frame->tail;
	}
/*	printf("return\n"); debug(s,s+1,d);*/
}

/*
void
debug(List *stack, List *top, int d)
{
	printf("----------------------level %d\n", d);
	while(stack<top) {
		printout(stack->head, stdout, "stdout");
		printf("----------------------\n");
		stack++;
	}
}
*/
