#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

/*
 * The hardware semaphores are strange.  Only 64 per page can be used,
 * 1024 pages of them.  Only the low bit is meaningful.
 * Reading an unset semaphore sets the semaphore and returns the old value.
 * Writing a semaphore sets the value, so writing 0 resets (clears) the semaphore.
 */

enum
{
	SEMPERPG	= 64,		/* hardware semaphores per page */
	NSEMPG		= 1024,
	ULOCKPG		= 512,
};

struct
{
	Lock	lock;			/* lock to allocate */
	uchar	bmap[NSEMPG];		/* allocation map */
	int	ulockpg;		/* count of user lock available */
}semalloc;

Page lkpgheader[NSEMPG];
#define lhash(laddr)	((int)laddr>>2)&(((NSEMPG-ULOCKPG)*(BY2PG>>2))-1)&~0x3c0

void
lockinit(void)
{
	int *sbsem, h, i;

	/*
	 * Initialise the system semaphore hardware
	 */
	for(i = 0; i < (NSEMPG-ULOCKPG)*BY2PG; i += 4) {
		h = lhash(i);
		sbsem = (int*)SBSEM+h;
		*sbsem = 0;
	}
	semalloc.ulockpg = ULOCKPG;
}

/* equivalent of newpage for pages of hardware locks */
Page*
lkpage(Segment *s, ulong va)
{
	uchar *p, *top;
	Page *pg;
	int i;

	USED(s);
	lock(&semalloc.lock);
	if(--semalloc.ulockpg < 0) {
		semalloc.ulockpg++;
		unlock(&semalloc.lock);
		return 0;
	}
	top = &semalloc.bmap[NSEMPG];
	for(p = semalloc.bmap; p < top && *p; p++)
		;
	if(p >= top)
		panic("lkpage");

	*p = 1;
	i = p-semalloc.bmap;
	pg = &lkpgheader[i];
	pg->pa = (ulong)((i*WD2PG) + SBSEM) & ~UNCACHED;
	pg->va = va;
	pg->ref = 1;

	unlock(&semalloc.lock);
	return pg;
}

void
lkpgfree(Page *pg)
{
	uchar *p;

	lock(&semalloc.lock);
	p = &semalloc.bmap[((pg->pa|UNCACHED)-(ulong)SBSEM)/BY2PG];
	if(!*p)
		panic("lkpgfree");
	*p = 0;
	
	semalloc.ulockpg++;
	unlock(&semalloc.lock);
}

void
lock(Lock *lk)
{
	int t, *hwsem, hash;

	hash = lhash(lk);
	hwsem = (int*)SBSEM+hash;

	t = 2000000;
	while(t--) {
		if(muxlock(hwsem, &lk->val))
			return;
		while(lk->val && t--)
			;
	}
	print("lock loop %lux pc %lux held by pc %lux\n",
					lk, getcallerpc(lk), lk->pc);
	if(u) {
		print("%d: %s\n", u->p->pid, u->p->text);
		dumpstack();
		u->p->state = Wakeme;
		sched();
	}
}

void
ilock(Lock *lk)
{
	int t, *hwsem, hash;
	int x;

	x = splhi();
	hash = lhash(lk);
	hwsem = (int*)SBSEM+hash;

	t = 2000000;
	while(t--) {
		if(muxlock(hwsem, &lk->val)){
			lk->sr = x;
			return;
		}
		while(lk->val && t--)
			;
	}
	splx(x);
	print("lock loop %lux pc %lux held by pc %lux\n",
					lk, getcallerpc(lk), lk->pc);
	if(u) {
		print("%d: %s\n", u->p->pid, u->p->text);
		dumpstack();
		u->p->state = Wakeme;
		sched();
	}

}	

int
canlock(Lock *lk)
{
	int hash;

	hash = lhash(lk);
	return muxlock((int*)SBSEM+hash, &lk->val);
}

void
unlock(Lock *l)
{
	l->pc = 0;
	l->val = 0;
}

void
iunlock(Lock *l)
{
	ulong sr;

	sr = l->sr;
	l->pc = 0;
	l->val = 0;
	splx(sr);
}
