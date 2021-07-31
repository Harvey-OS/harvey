#include <u.h>
#include <libc.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

#define	HIOB		31	/* a prime */
#define	NIOBUF		80

static Iotrack	hiob[HIOB+1];		/* hash buckets + lru list */
static Iotrack	iobuf[NIOBUF];		/* the real ones */

#define	UNLINK(p, nx, pr)	((p)->pr->nx = (p)->nx, (p)->nx->pr = (p)->pr)

#define	LINK(h, p, nx, pr)	((p)->nx = (h)->nx, (p)->pr = (h), \
				 (h)->nx->pr = (p), (h)->nx = (p))

#define	HTOFRONT(h, p)	((h)->hnext != (p) ? (UNLINK(p,hnext,hprev), LINK(h,p,hnext,hprev)) : 0)

#define	TOFRONT(h, p)	((h)->next  != (p) ? (UNLINK(p, next, prev), LINK(h,p, next, prev)) : 0)

Iosect *
getsect(Xfs *xf, long addr)
{
	return getiosect(xf, addr, 1);
}

Iosect *
getosect(Xfs *xf, long addr)
{
	return getiosect(xf, addr, 0);
}

Iosect *
getiosect(Xfs *xf, long addr, int rflag)
{
	Iotrack *t;
	long taddr;
	int toff;
	Iosect *p;

	toff = addr % Sect2trk;
	taddr = addr - toff;
	t = getiotrack(xf, taddr);
	if(rflag && (t->flags&BSTALE)){
		if(tread(t) < 0){
			unlock(&t->lock);
			return 0;
		}
		t->flags &= ~BSTALE;
	}
	t->ref++;
	p = t->tp->p[toff];
	if(p == 0){
		p = newsect();
		t->tp->p[toff] = p;
		p->flags = t->flags&BSTALE;
		p->lock.key = 0;
		p->t = t;
		p->iobuf = t->tp->buf[toff];
	}
	unlock(&t->lock);
	lock(&p->lock);
	return p;
}

void
putsect(Iosect *p)
{
	Iotrack *t;

	if(canlock(&p->lock))
		panic("putsect");
	t = p->t;
	lock(&t->lock);
	t->flags |= p->flags;
	p->flags = 0;
	t->ref--;
	if(t->flags & BIMM){
		if(t->flags & BMOD)
			twrite(t);
		t->flags &= ~(BMOD|BIMM);
	}
	unlock(&t->lock);
	unlock(&p->lock);
}

Iotrack *
getiotrack(Xfs *xf, long addr)
{
	Iotrack *hp, *p;
	Iotrack *mp = &hiob[HIOB];
	long h;
/*
 *	chat("iotrack %d,%d...", dev, addr);
 */
	h = (xf->dev<<24) ^ addr;
	if(h < 0)
		h = ~h;
	h %= HIOB;
	hp = &hiob[h];

loop:

/*
 * look for it in the active list
 */
	lock(&hp->lock);
	for(p=hp->hnext; p != hp; p=p->hnext){
		if(p->addr != addr || p->xf != xf)
			continue;
		unlock(&hp->lock);
		lock(&p->lock);
		if(p->addr == addr && p->xf == xf)
			goto out;
		unlock(&p->lock);
		goto loop;
	}
	unlock(&hp->lock);
/*
 * not found
 * take oldest unref'd entry
 */
	lock(&mp->lock);
	for(p=mp->prev; p != mp; p=p->prev)
		if(p->ref == 0 && canlock(&p->lock)){
			if(p->ref == 0)
				break;
			unlock(&p->lock);
		}
	unlock(&mp->lock);
	if(p == mp){
		print("iotrack all ref'd\n");
		goto loop;
	}
	if(p->flags & BMOD){
		twrite(p);
		p->flags &= ~(BMOD|BIMM);
		unlock(&p->lock);
		goto loop;
	}
	purgetrack(p);
	p->addr = addr;
	p->xf = xf;
	p->flags = BSTALE;
out:
	lock(&hp->lock); HTOFRONT(hp, p); unlock(&hp->lock);
	lock(&mp->lock); TOFRONT(mp, p); unlock(&mp->lock);
	return p;
}

void
purgetrack(Iotrack *t)
{
	int i, ref = Sect2trk;
	Iosect *p;

	for(i=0; i<Sect2trk; i++){
		p = t->tp->p[i];
		if(p == 0){
			--ref;
			continue;
		}
		if(canlock(&p->lock)){
			freesect(p);
			--ref;
			t->tp->p[i] = 0;
		}
	}
	if(t->ref != ref)
		panic("purgetrack");
}

int
twrite(Iotrack *t)
{
	int i, ref;

	chat("[twrite %d...", t->addr);
	if(t->flags & BSTALE){
		for(ref=0,i=0; i<Sect2trk; i++)
			if(t->tp->p[i])
				++ref;
		if(ref < Sect2trk){
			if(tread(t) < 0){
				chat("error]");
				return -1;
			}
		}else
			t->flags &= ~BSTALE;
	}
	if(devwrite(t->xf, t->addr, t->tp->buf, Trksize) < 0){
		chat("error]");
		return -1;
	}
	chat(" done]");
	return 0;
}

int
tread(Iotrack *t)
{
	int i, ref = 0;
	uchar buf[Sect2trk][Sectorsize];

	for(i=0; i<Sect2trk; i++)
		if(t->tp->p[i])
			++ref;
	chat("[tread %d...", t->addr);
	if(ref == 0){
		if(devread(t->xf, t->addr, t->tp->buf, Trksize) < 0){
			chat("error]");
			return -1;
		}
		chat("done]");
		t->flags &= ~BSTALE;
		return 0;
	}
	if(devread(t->xf, t->addr, buf, Trksize) < 0){
		chat("error]");
		return -1;
	}
	for(i=0; i<Sect2trk; i++)
		if(t->tp->p[i] == 0){
			memmove(t->tp->buf[i], buf[i], Sectorsize);
			chat("%d ", i);
		}
	chat("done]");
	t->flags &= ~BSTALE;
	return 0;
}

void
purgebuf(Xfs *xf)
{
	Iotrack *p;

	for(p=&iobuf[0]; p<&iobuf[NIOBUF]; p++){
		if(p->xf != xf)
			continue;
		lock(&p->lock);
		if(p->xf == xf){
			if(p->flags & BMOD)
				twrite(p);
			p->flags = BSTALE;
			purgetrack(p);
		}
		unlock(&p->lock);
	}
}

void
sync(void)
{
	Iotrack *p;

	for(p=&iobuf[0]; p<&iobuf[NIOBUF]; p++){
		if(!(p->flags & BMOD))
			continue;
		lock(&p->lock);
		if(p->flags & BMOD){
			twrite(p);
			p->flags &= ~(BMOD|BIMM);
		}
		unlock(&p->lock);
	}
}

void
iotrack_init(void)
{
	Iotrack *mp, *p;

	for (mp=&hiob[0]; mp<&hiob[HIOB]; mp++)
		mp->hprev = mp->hnext = mp;
	mp->prev = mp->next = mp;

	for (p=&iobuf[0]; p<&iobuf[NIOBUF]; p++) {
		p->hprev = p->hnext = p;
		p->prev = p->next = p;
		TOFRONT(mp, p);
		p->tp = sbrk(sizeof(Track));
		memset(p->tp->p, 0, sizeof p->tp->p);
	}
}

static Lock	freelock;
static Iosect *	freelist;

Iosect *
newsect(void)
{
	Iosect *p;

	lock(&freelock);
	if(p = freelist)	/* assign = */
		freelist = p->next;
	else
		p = malloc(sizeof(Iosect));
	unlock(&freelock);
	p->next = 0;
	return p;
}

void
freesect(Iosect *p)
{
	lock(&freelock);
	p->next = freelist;
	freelist = p;
	unlock(&freelock);
}
