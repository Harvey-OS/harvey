/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Generic traversal routines.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static uint
etype(Entry *e)
{
	uint t;

	if(e->flags&VtEntryDir)
		t = BtDir;
	else
		t = BtData;
	return t+e->depth;
}

void
initWalk(WalkPtr *w, Block *b, uint size)
{
	memset(w, 0, sizeof *w);
	switch(b->l.type){
	case BtData:
		return;

	case BtDir:
		w->data = b->data;
		w->m = size / VtEntrySize;
		w->isEntry = 1;
		return;

	default:
		w->data = b->data;
		w->m = size / VtScoreSize;
		w->type = b->l.type;
		w->tag = b->l.tag;
		return;
	}
}

int
nextWalk(WalkPtr *w, uchar score[VtScoreSize], uchar *type, u32int *tag, Entry **e)
{
	if(w->n >= w->m)
		return 0;

	if(w->isEntry){
		*e = &w->e;
		entryUnpack(&w->e, w->data, w->n);
		memmove(score, w->e.score, VtScoreSize);
		*type = etype(&w->e);
		*tag = w->e.tag;
	}else{
		*e = nil;
		memmove(score, w->data+w->n*VtScoreSize, VtScoreSize);
		*type = w->type-1;
		*tag = w->tag;
	}
	w->n++;
	return 1;
}

