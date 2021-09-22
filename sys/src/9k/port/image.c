/*
 * manipulate Images (program text segments)
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#define NFREECHAN	64
#define IHASHSIZE	64
#define ihash(s)	imagealloc.hash[(s)%IHASHSIZE]

enum
{
	NIMAGE = 200,
};

static struct Imagealloc
{
	Lock;
	Image	*free;
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
	uvlong	ticks;			/* total time in the main loop */
	uvlong	maxt;			/* longest time in main loop */
} irstats;

void
initimage(void)
{
	Image *i, *ie;

	imagealloc.free = malloc(NIMAGE*sizeof(Image));
	if(imagealloc.free == nil)
		panic("imagealloc: no memory");
	ie = &imagealloc.free[NIMAGE-1];
	for(i = imagealloc.free; i < ie; i++)
		i->next = i+1;
	i->next = 0;
	imagealloc.freechan = malloc(NFREECHAN * sizeof(Chan*));
	imagealloc.szfreechan = NFREECHAN;
}

static void
imagereclaim(void)
{
	uvlong ticks;

	irstats.calls++;
	/* Somebody is already cleaning the page cache */
	if(!canqlock(&imagealloc.ireclaim))
		return;

	ticks = pagereclaim(1000);

	irstats.loops++;
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

/* returns a locked Image* */
Image*
attachimage(int type, Chan *c, uintptr base, uintptr top)
{
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
	while(!(i = imagealloc.free)) {
		unlock(&imagealloc);
		imagereclaim();
		sched();
		lock(&imagealloc);
	}

	imagealloc.free = i->next;

	lock(i);
	incref(c);
	i->c = c;
	i->dc = c->dev->dc;
//subtype
	i->qid = c->qid;
	i->mqid = c->mqid;
	i->mchan = c->mchan;
	l = &ihash(c->qid.path);
	i->hash = *l;
	*l = i;
found:
	unlock(&imagealloc);

	if(i->s == 0) {
		/* Disaster after commit in exec */
		if(waserror()) {
			unlock(i);
			pexit(Enovmem, 1);
		}
		i->s = newseg(type, base, top);
		i->s->image = i;
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
	if(--i->ref > 0) {
		unlock(i);
		return;
	}

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

	i->next = imagealloc.free;
	imagealloc.free = i;

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
	unlock(&imagealloc);
}
