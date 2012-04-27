#include <u.h>
#include <libc.h>
#include "cformat.h"
#include "lru.h"
#include "bcache.h"
#include "disk.h"

int	icformat(Disk*, ulong);

/*
 *  read in the disk structures,  return -1 if the format
 *  is inconsistent.
 */
int
dinit(Disk *d, int f, int psize, char *expname)
{
	ulong	i;
	uvlong	length;
	char	buf[1024];
	Bbuf	*b;
	Dalloc	*ba;
	Dir	*dir;

	/*
	 *  get disk size
	 */
	dir = dirfstat(f);
	if(dir == nil){
		perror("dinit: stat");
		return -1;
	}
	length = dir->length;
	free(dir);

	/*
	 *  read first physical block to get logical block size, # of inodes,
	 *  and # of allocation blocks
	 */
	if(seek(f, 0, 0) < 0){
		perror("dinit: seek");
		return -1;
	}
	if(read(f, buf, sizeof(buf)) != sizeof(buf)){
		perror("dinit: read");
		return -1;
	}
	ba = (Dalloc*)buf;
	if(ba->bsize <= 0){
		fprint(2, "dinit: bsize 0x%lux<= 0\n", ba->bsize);
		return -1;
	}
	if((ba->bsize % psize) != 0){
		fprint(2, "dinit: logical bsize (%lud) not multiple of physical (%ud)\n",
			ba->bsize, psize);
		return -1;
	}
	d->bsize = ba->bsize;
	d->nb = length/d->bsize;
	d->b2b = (d->bsize - sizeof(Dahdr))*8;
	d->nab = (d->nb+d->b2b-1)/d->b2b;
	d->p2b = d->bsize/sizeof(Dptr);
	strncpy(d->name, ba->name, sizeof d->name);

	if (expname != nil && strncmp(d->name, expname, sizeof d->name) != 0) {
		/* Mismatch with recorded name; fail here to force a format */
		fprint(2, "cfs: name mismatch\n");
		return -1;
	}

	/*
	 *  check allocation blocks for consistency
	 */
	if(bcinit(d, f, d->bsize) < 0){
		fprint(2, "dinit: couldn't init block cache\n");
		return -1;
	}
	for(i = 0; i < d->nab; i++){
		b = bcread(d, i);
		if(b == 0){
			perror("dinit: read");
			return -1;
		}
		ba = (Dalloc*)b->data;
		if(ba->magic != Amagic){
			fprint(2, "dinit: bad magic in alloc block %uld\n", i);
			return -1;
		}
		if(d->bsize != ba->bsize){
			fprint(2, "dinit: bad bsize in alloc block %uld\n", i);
			return -1;
		}
		if(d->nab != ba->nab){
			fprint(2, "dinit: bad nab in alloc block %uld\n", i);
			return -1;
		}
		if(strncmp(d->name, ba->name, sizeof(d->name))){
			fprint(2, "dinit: bad name in alloc block %uld\n", i);
			return -1;
		}
	}
	return 0;
}

/*
 *  format the disk as a cache
 */
int
dformat(Disk *d, int f, char *name, ulong bsize, ulong psize)
{
	int	i;
	uvlong	length;
	Bbuf	*b;
	Dalloc	*ba;
	Dir	*dir;
	Dptr	dptr;

	fprint(2, "formatting disk\n");

	/*
	 *  calculate basic numbers
	 */
	dir = dirfstat(f);
	if(dir == nil)
		return -1;
	length = dir->length;
	d->bsize = bsize;
	if((d->bsize % psize) != 0){
		fprint(2, "cfs: logical bsize not multiple of physical\n");
		return -1;
	}
	d->nb = length/d->bsize;
	d->b2b = (d->bsize - sizeof(Dahdr))*8;
	d->nab = (d->nb+d->b2b-1)/d->b2b;
	d->p2b = d->bsize/sizeof(Dptr);

	/*
	 *  init allocation blocks
	 */
	if(bcinit(d, f, d->bsize) < 0)
		return -1;
	for(i = 0; i < d->nab; i++){
		b = bcalloc(d, i);
		if(b == 0){
			perror("cfs: bcalloc");
			return -1;
		}
		memset(b->data, 0, d->bsize);
		ba = (Dalloc*)b->data;
		ba->magic = Amagic;
		ba->bsize = d->bsize;
		ba->nab = d->nab;
		strncpy(ba->name, name, sizeof(ba->name));
		bcmark(d, b);
	}

	/*
	 *  allocate allocation blocks
	 */
	for(i = 0; i < d->nab; i++)
		if(dalloc(d, &dptr) == Notabno){
			fprint(2, "can't allocate allocation blocks\n");
			return -1;
		}

	return bcsync(d);
}

/*
 *  allocate a block from a bit vector page
 *
 *  a return value of Notabno means no blocks left
 */
static ulong
_balloc(Dalloc *ba, ulong max)
{
	int len;	/* number of valid words */
	ulong i;	/* bit position in long */
	ulong m;	/* 1<<i */
	ulong v;	/* old value of long */
	ulong *p, *e;

	/*
	 *  find a word with a 0 bit
	 */
	len = (max+BtoUL-1)/BtoUL;
	for(p = ba->bits, e = p + len; p < e; p++)
		if(*p != 0xFFFFFFFF)
			break;
	if(p == e)
		return Notabno;

	/*
	 *  find the first 0 bit
	 */
	v = *p;
	for(m = 1, i = 0; i < BtoUL; i++, m <<= 1)
		if((m|v) != v)
			break;

	/*
	 *  calculate block number
	 */
	i += (p - ba->bits)*BtoUL;
	if(i >= max)
		return Notabno;

	/*
	 *  set bit to 1
	 */
	*p = v | m;
	return i;
}

/*
 *  allocate a block
 *
 *  return Notabno if none left
 */
ulong
dalloc(Disk *d, Dptr *p)
{
	ulong	bno, max, rv;
	Bbuf	*b;
	Dalloc	*ba;

	max = d->nb;
	for(bno = 0; bno < d->nab; bno++){
		b = bcread(d, bno);
		ba = (Dalloc*)b->data;
		rv = _balloc(ba, max > d->b2b ? d->b2b : max);
		if(rv != Notabno){
			rv = bno*d->b2b + rv;
			if(p){
				p->start = p->end = 0;
				p->bno = rv;
			}
			bcmark(d, b);
			return rv;
		}
		max -= d->b2b;
	}
	if(p)
		p->bno = Notabno;
	return Notabno;
}

/*
 *  allocate a block of pointers
 */
ulong
dpalloc(Disk *d, Dptr *p)
{
	Bbuf *b;
	Dptr *sp, *ep;

	if(dalloc(d, p) == Notabno)
		return Notabno;

	/*
	 *  allocate the page and invalidate all the
	 *  pointers
	 */
	b = bcalloc(d, p->bno);
	if(b == 0)
		return -1;
	sp = (Dptr*)b->data;
	for(ep = sp + d->p2b; sp < ep; sp++){
		sp->bno = Notabno;
		sp->start = sp->end = 0;
	}
	p->bno |= Indbno;
	p->start = 0;
	p->end = d->bsize;

	/*
	 *  mark the page as dirty
	 */
	bcmark(d, b);
	return 0;
}

/*
 *  free a block
 */
int
_bfree(Disk *d, ulong i)
{
	ulong bno, m;
	ulong *p;
	Bbuf *b;
	Dalloc *ba;

	/*
	 *  get correct allocation block
	 */
	bno = i/d->b2b;
	if(bno >= d->nab)
		return -1;
	b = bcread(d, bno);
	if(b == 0)
		return -1;
	ba = (Dalloc*)b->data;

	/*
	 *  change bit
	 */
	i -= bno*d->b2b;
	p = ba->bits + (i/BtoUL);
	m = 1<<(i%BtoUL);
	*p &= ~m;
	bcmark(d, b);

	return 0;
}

/*
 *  free a block (or blocks)
 */
int
dfree(Disk *d, Dptr *dp)
{
	ulong bno;
	Dptr *sp, *ep;
	Bbuf *b;

	bno = dp->bno;
	dp->bno = Notabno;

	/*
	 *  nothing to free
	 */
	if(bno == Notabno)
		return 0;

	/*
	 *  direct pointer
	 */
	if((bno & Indbno) == 0)
		return _bfree(d, bno);

	/*
	 *  first indirect page
	 */
	bno &= ~Indbno;
	_bfree(d, bno);

	/*
	 *  then all the pages it points to
	 *
	 *	DANGER: this algorithm may fail if there are more
	 *		allocation blocks than block buffers
	 */
	b = bcread(d, bno);
	if(b == 0)
		return -1;
	sp = (Dptr*)b->data;
	for(ep = sp + d->p2b; sp < ep; sp++)
		if(dfree(d, sp) < 0)
			return -1;
	return 0;
}
