/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include <memlayer.h>

Memimage *
memlalloc(Memscreen *s, Rectangle screenr, Refreshfn refreshfn, void *refreshptr,
	  uint32_t val)
{
	Memlayer *l;
	Memimage *n;
	static Memimage *paint;

	if(paint == nil) {
		paint = allocmemimage(Rect(0, 0, 1, 1), RGBA32);
		if(paint == nil)
			return nil;
		paint->flags |= Frepl;
		paint->clipr = Rect(-0x3FFFFFF, -0x3FFFFFF, 0x3FFFFFF, 0x3FFFFFF);
	}

	n = allocmemimaged(screenr, s->image->chan, s->image->data, s->image->X);
	if(n == nil)
		return nil;
	l = malloc(sizeof(Memlayer));
	if(l == nil) {
		free(n);
		return nil;
	}

	l->screen = s;
	if(refreshfn)
		l->save = nil;
	else {
		l->save = allocmemimage(screenr, s->image->chan);
		if(l->save == nil) {
			free(l);
			free(n);
			return nil;
		}
		/* allocmemimage doesn't initialize memory; this paints save area */
		if(val != DNofill)
			memfillcolor(l->save, val);
	}
	l->refreshfn = refreshfn;
	l->refreshptr = nil; /* don't set it until we're done */
	l->screenr = screenr;
	l->delta = Pt(0, 0);

	n->data->ref++;
	n->zero = s->image->zero;
	n->width = s->image->width;
	n->layer = l;

	/* start with new window behind all existing ones */
	l->front = s->rearmost;
	l->rear = nil;
	if(s->rearmost)
		s->rearmost->layer->rear = n;
	s->rearmost = n;
	if(s->frontmost == nil)
		s->frontmost = n;
	l->clear = 0;

	/* now pull new window to front */
	_memltofrontfill(n, val != DNofill);
	l->refreshptr = refreshptr;

	/*
	 * paint with requested color; previously exposed areas are already right
	 * if this window has backing store, but just painting the whole thing is simplest.
	 */
	if(val != DNofill) {
		memsetchan(paint, n->chan);
		memfillcolor(paint, val);
		memdraw(n, n->r, paint, n->r.min, nil, n->r.min, S);
	}
	return n;
}
