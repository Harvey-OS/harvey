#include <u.h>
#include <libc.h>
#include "cformat.h"
#include "lru.h"
#include "bcache.h"
#include "disk.h"
#include "inode.h"
#include "file.h"

/*
 *  merge data with that which already exists in a block
 *
 *  we allow only one range per block, always use the new
 *  data if the ranges don't overlap.
 */
void
fmerge(Dptr *p, char *to, char *from, int start, int len)
{
	int end;

	end = start + len;
	memmove(to+start, from, end-start);

	/*
	 *  if ranges do not overlap...
	 */
	if(start>p->end || p->start>end){
		/*
		 *  just use the new data
		 */
		p->start = start;
		p->end = end;
	} else {
		/*
		 *  merge ranges
		 */
		if(start < p->start)
			p->start = start;
		if(end > p->end)
			p->end = end;
	}

}

/*
 *  write a block (or less) of data onto a disk, follow it with any necessary
 *  pointer writes.
 *
 *	N.B. ordering is everything
 */
int
fbwrite(Icache *ic, Ibuf *b, char *a, ulong off, int len)
{
	int wrinode;
	ulong fbno;
	Bbuf *dbb;	/* data block */
	Bbuf *ibb;	/* indirect block */
	Dptr *p;
	Dptr t;

	fbno = off / ic->bsize;
	p = &b->inode.ptr;
	ibb = 0;
	wrinode = 0;

	/*
	 *  are there any pages for this inode?
	 */
	if(p->bno == Notabno){
		wrinode = 1;
		goto dowrite;
	}
	
	/*
 	 *  is it an indirect block?
	 */
	if(p->bno & Indbno){
		ibb = bcread(ic, p->bno);
		if(ibb == 0)
			return -1;
		p = (Dptr*)ibb->data;
		p += fbno % ic->p2b;
		goto dowrite;
	}

	/*
 	 *  is it the wrong direct block?
	 */
	if((p->fbno%ic->p2b) != (fbno%ic->p2b)){
		/*
		 *  yes, make an indirect block
		 */
		t = *p;
		dpalloc(ic, p);
		if(p->bno == Notabno){
			*p = t;
			return -1;
		}
		ibb = bcalloc(ic, p->bno);
		if(ibb == 0){
			*p = t;
			return -1;
		}
		p = (Dptr*)ibb->data;
		p += t.fbno % ic->p2b;
		*p = t;
		p = (Dptr*)ibb->data;
		p += fbno % ic->p2b;
	}
	wrinode = 1;

dowrite:
	/*
	 *  get the data block into the block cache
	 */
	if(p->bno == Notabno){
		/*
		 *  create a new block
		 */
		dalloc(ic, p);
		if(p->bno == Notabno)
			return -1;		/* no blocks left (maybe) */
		dbb = bcalloc(ic, p->bno);
	} else {
		/*
		 *  use what's there
		 */
		dbb = bcread(ic, p->bno);
	}
	if(dbb == 0)
		return -1;

	/*
	 *  merge in the new data
	 */
	if(p->fbno != fbno){
		p->start = p->end = 0;
		p->fbno = fbno;
	}
	fmerge(p, dbb->data, a, off % ic->bsize, len);

	/*
	 *  write changed blocks back in the
	 *  correct order
	 */
	bcmark(ic, dbb);
	if(ibb)
		bcmark(ic, ibb);
	if(wrinode)
		if(iwrite(ic, b) < 0)
			return -1;
	return len;
}

/*
 *  write `n' bytes to the cache
 *
 *  return number of bytes written
 */
long
fwrite(Icache *ic, Ibuf *b, char *a, ulong off, long n)
{
	int len;
	long sofar;

	for(sofar = 0; sofar < n; sofar += len){
		len = ic->bsize - ((off+sofar)%ic->bsize);
		if(len > n - sofar)
			len = n - sofar;
		if(fbwrite(ic, b, a+sofar, off+sofar, len) < 0)
			return sofar;
	}
	return sofar;
}

/*
 *  get a pointer to the next valid data at or after `off'
 */
Dptr *
fpget(Icache *ic, Ibuf *b, ulong off)
{
	ulong fbno;
	long doff;
	Bbuf *ibb;	/* indirect block */
	Dptr *p, *p0, *pf;

	fbno = off / ic->bsize;
	p = &b->inode.ptr;

	/*
	 *  are there any pages for this inode?
	 */
	if(p->bno == Notabno)
		return 0;
	
	/*
 	 *  if it's a direct block, life is easy?
	 */
	if(!(p->bno & Indbno)){
		/*
		 *  a direct block, return p if it's at least past what we want
		 */
		if(p->fbno > fbno)
			return p;
		if(p->fbno < fbno)
			return 0;
		doff = off % ic->bsize;
		if(doff>=p->start && doff<p->end)
			return p;
		else
			return 0;
	}

	/*
	 *  read the indirect block
	 */
	ibb = bcread(ic, p->bno);
	if(ibb == 0)
		return 0;

	/*
	 *  find the next valid pointer
	 */
	p0 = (Dptr*)ibb->data;
	pf = p0 + (fbno % ic->p2b);
	if(pf->bno!=Notabno && pf->fbno==fbno){
		doff = off % ic->bsize;
		if(doff<pf->end)
			return pf;
	}
	for(p = pf+1; p < p0 + ic->p2b; p++){
		fbno++;
		if(p->fbno==fbno && p->bno!=Notabno && p->start<p->end)
			return p;
	}
	for(p = p0; p < pf; p++){
		fbno++;
		if(p->fbno==fbno && p->bno!=Notabno && p->start<p->end)
			return p;
	}
	return 0;
}

/*
 *  read `n' bytes from the cache.
 *
 *  if we hit a gap and we've read something,
 *  return number of bytes read so far.
 *
 *  if we start with a gap, return minus the number of bytes
 *  to the next data.
 *
 *  if there are no bytes cached, return 0.
 */
long
fread(Icache *ic, Ibuf *b, char *a, ulong off, long n)
{
	int len, start;
	long sofar, gap;
	Dptr *p;
	Bbuf *bb;

	for(sofar = 0; sofar < n; sofar += len, off += len){
		/*
		 *  get pointer to next data
		 */
		len = n - sofar;
		p = fpget(ic, b, off);

		/*
		 *  if no more data, return what we have so far
		 */
		if(p == 0)
			return sofar;

		/*
		 *  if there's a gap, return the size of the gap
		 */
		gap = (ic->bsize*p->fbno + p->start) - off;
		if(gap>0)
			if(sofar == 0)
				return -gap;
			else
				return sofar;

		/*
		 *  return what we have
		 */
		bb = bcread(ic, p->bno);
		if(bb == 0)
			return sofar;
		start = p->start - gap;
		if(p->end - start < len)
			len = p->end - start;
		memmove(a + sofar, bb->data + start, len);
	}
	return sofar;
}
