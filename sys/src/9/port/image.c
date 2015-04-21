/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#define NFREECHAN	64
#define IHASHSIZE	64
#define ihash(s)	imagealloc.hash[s%IHASHSIZE]

static struct Imagealloc
{
	Lock;
	Image	*mru;			/* head of LRU list */
	Image	*lru;			/* tail of LRU list */
	Image	*hash[IHASHSIZE];
	QLock	ireclaim;		/* mutex on reclaiming free images */

	Chan	**freechan;		/* free image channels */
	int	nfreechan;		/* number of free channels */
	int	szfreechan;		/* size of freechan array */
	QLock	fcreclaim;		/* mutex on reclaiming free channels */
} imagealloc;

static struct {
	int	calls;			/* times imagereclaim was called */
	int	loops;			/* times the main loop was run */
	uint64_t	ticks;			/* total time in the main loop */
	uint64_t	maxt;			/* longest time in main loop */
	int	noluck;			/* # of times we couldn't get one */
	int	nolock;			/* # of times we couldn't get the lock */
} irstats;

#if 0
static void
dumplru(void)
{
	Image *i;

	print("lru:");
	for(i = imagealloc.mru; i != nil; i = i->next)
		print(" %p(c%p,r%d)", i, i->c, i->ref);
	print("\n");
}
#endif

/*
 * imagealloc and i must be locked.
 */
static void
imageunused(Image *i)
{
	if(i->prev != nil)
		i->prev->next = i->next;
	else
		imagealloc.mru = i->next;
	if(i->next != nil)
		i->next->prev = i->prev;
	else
		imagealloc.lru = i->prev;
	i->next = i->prev = nil;
}

/*
 * imagealloc and i must be locked.
 */
static void
imageused(Image *i)
{
	imageunused(i);
	i->next = imagealloc.mru;
	i->next->prev = i;
	imagealloc.mru = i;
	if(imagealloc.lru == nil)
		imagealloc.lru = i;
}

/*
 * imagealloc must be locked.
 */
static Image*
lruimage(void)
{
	Image *i;

	for(i = imagealloc.lru; i != nil; i = i->prev)
		if(i->c == nil){
			/*
			 * i->c will be set before releasing the
			 * lock on imagealloc, which means it's in use.
			 */
			return i;
		}
	return nil;
}

/*
 * On clu, set conf.nimages = 10 to exercise reclaiming.
 * It won't be able to get through all of cpurc, but will reclaim.
 */
void
initimage(void)
{
	Image *i, *ie;

	DBG("initimage: %uld images\n", conf.nimage);
	imagealloc.mru = malloc(conf.nimage*sizeof(Image));
	if(imagealloc.mru == nil)
		panic("imagealloc: no memory");
	ie = &imagealloc.mru[conf.nimage];
	for(i = imagealloc.mru; i < ie; i++){
		i->c = nil;
		i->ref = 0;
		i->prev = i-1;
		i->next = i+1;
	}
	imagealloc.mru[0].prev = nil;
	imagealloc.mru[conf.nimage-1].next = nil;
	imagealloc.lru = &imagealloc.mru[conf.nimage-1];
	imagealloc.freechan = malloc(NFREECHAN * sizeof(Chan*));
	imagealloc.szfreechan = NFREECHAN;

}

static void
imagereclaim(void)
{
	Image *i;
	uint64_t ticks0, ticks;

	irstats.calls++;
	/* Somebody is already cleaning the page cache */
	if(!canqlock(&imagealloc.ireclaim))
		return;
	DBG("imagereclaim maxt %ulld noluck %d nolock %d\n",
		irstats.maxt, irstats.noluck, irstats.nolock);
	ticks0 = fastticks(nil);
	if(!canlock(&imagealloc)){
		/* never happen in the experiments I made */
		qunlock(&imagealloc.ireclaim);
		return;
	}

	for(i = imagealloc.lru; i != nil; i = i->prev){
		if(canlock(i)){
			i->ref++;	/* make sure it does not go away */
			unlock(i);
			pagereclaim(i);
			lock(i);
			DBG("imagereclaim: image %p(c%p, r%d)\n", i, i->c, i->ref);
			if(i->ref == 1){	/* no pages referring to it, it's ours */
				unlock(i);
				unlock(&imagealloc);
				putimage(i);
				break;
			}else
				--i->ref;
			unlock(i);
		}
	}

	if(i == nil){
		irstats.noluck++;
		unlock(&imagealloc);
	}
	irstats.loops++;
	ticks = fastticks(nil) - ticks0;
	irstats.ticks += ticks;
	if(ticks > irstats.maxt)
		irstats.maxt = ticks;
	//print("T%llud+", ticks);
	qunlock(&imagealloc.ireclaim);
}

/*
 *  since close can block, this has to be called outside of
 *  spin locks.
 */
static void
imagechanreclaim(void)
{
	Chan *c;

	/* Somebody is already cleaning the image chans */
	if(!canqlock(&imagealloc.fcreclaim))
		return;

	/*
	 * We don't have to recheck that nfreechan > 0 after we
	 * acquire the lock, because we're the only ones who decrement
	 * it (the other lock contender increments it), and there's only
	 * one of us thanks to the qlock above.
	 */
	while(imagealloc.nfreechan > 0){
		lock(&imagealloc);
		imagealloc.nfreechan--;
		c = imagealloc.freechan[imagealloc.nfreechan];
		unlock(&imagealloc);
		cclose(c);
	}

	qunlock(&imagealloc.fcreclaim);
}

Image*
attachimage(int type, Chan *c, int color, uintptr_t base, usize len)
{
	Mach *m = machp();
	Image *i, **l;

	/* reclaim any free channels from reclaimed segments */
	if(imagealloc.nfreechan)
		imagechanreclaim();

	lock(&imagealloc);

	/*
	 * Search the image cache for remains of the text from a previous
	 * or currently running incarnation
	 */
	for(i = ihash(c->qid.path); i; i = i->hash) {
		if(c->qid.path == i->qid.path) {
			lock(i);
			if(eqqid(c->qid, i->qid) &&
			   eqqid(c->mqid, i->mqid) &&
			   c->mchan == i->mchan &&
			   c->dev->dc == i->dc) {
//subtype
				goto found;
			}
			unlock(i);
		}
	}

	/*
	 * imagereclaim dumps pages from the free list which are cached by image
	 * structures. This should free some image structures.
	 */
	while(!(i = lruimage())) {
		unlock(&imagealloc);
		imagereclaim();
		sched();
		lock(&imagealloc);
	}

	lock(i);
	incref(c);
	i->dc = c->dev->dc;
//subtype
	i->qid = c->qid;
	i->mqid = c->mqid;
	i->mchan = c->mchan;
	i->color = color;
	l = &ihash(c->qid.path);
	i->hash = *l;
	*l = i;
found:
	imageused(i);
	unlock(&imagealloc);

	if(i->s == 0) {
		/* Disaster after commit in exec */
		if(waserror()) {
			unlock(i);
			pexit(Enovmem, 1);
		}
		i->s = newseg(type, base, len);
		i->s->image = i;
		i->s->color = color;
		i->ref++;
		poperror();
	}
	else
		incref(i->s);

	return i;
}

void
putimage(Image *i)
{
	Chan *c, **cp;
	Image *f, **l;

	if(i->notext)
		return;

	lock(i);
	if(--i->ref == 0) {
		l = &ihash(i->qid.path);
		mkqid(&i->qid, ~0, ~0, QTFILE);
		unlock(i);
		c = i->c;

		lock(&imagealloc);
		for(f = *l; f; f = f->hash) {
			if(f == i) {
				*l = i->hash;
				break;
			}
			l = &f->hash;
		}

		/* defer freeing channel till we're out of spin lock's */
		if(imagealloc.nfreechan == imagealloc.szfreechan){
			imagealloc.szfreechan += NFREECHAN;
			cp = malloc(imagealloc.szfreechan*sizeof(Chan*));
			if(cp == nil)
				panic("putimage");
			memmove(cp, imagealloc.freechan, imagealloc.nfreechan*sizeof(Chan*));
			free(imagealloc.freechan);
			imagealloc.freechan = cp;
		}
		imagealloc.freechan[imagealloc.nfreechan++] = c;
		i->c = nil;		/* flag as unused in lru list */
		unlock(&imagealloc);

		return;
	}
	unlock(i);
}
