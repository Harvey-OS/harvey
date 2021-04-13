/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"

/*
 * An IEStream is a sorted list of index entries.
 */
struct IEStream
{
	Part	*part;
	u64	off;		/* read position within part */
	u64	n;		/* number of valid ientries left to read */
	u32	size;		/* allocated space in buffer */
	u8	*buf;
	u8	*pos;		/* current place in buffer */
	u8	*epos;		/* end of valid buffer contents */
};

IEStream*
initiestream(Part *part, u64 off, u64 clumps, u32 size)
{
	IEStream *ies;

/* out of memory? */
	ies = MKZ(IEStream);
	ies->buf = MKN(u8, size);
	ies->epos = ies->buf;
	ies->pos = ies->epos;
	ies->off = off;
	ies->n = clumps;
	ies->size = size;
	ies->part = part;
	return ies;
}

void
freeiestream(IEStream *ies)
{
	if(ies == nil)
		return;
	free(ies->buf);
	free(ies);
}

/*
 * Return the next IEntry (still packed) in the stream.
 */
static u8 *
peekientry(IEStream *ies)
{
	u32 n, nn;

	n = ies->epos - ies->pos;
	if(n < IEntrySize){
		memmove(ies->buf, ies->pos, n);
		ies->epos = &ies->buf[n];
		ies->pos = ies->buf;
		nn = ies->size;
		if(nn > ies->n * IEntrySize)
			nn = ies->n * IEntrySize;
		nn -= n;
		if(nn == 0)
			return nil;
//fprint(2, "peek %d from %llu into %p\n", nn, ies->off, ies->epos);
		if(readpart(ies->part, ies->off, ies->epos, nn) < 0){
			seterr(EOk, "can't read sorted index entries: %r");
			return nil;
		}
		ies->epos += nn;
		ies->off += nn;
	}
	return ies->pos;
}

/*
 * Compute the bucket number for the given IEntry.
 * Knows that the score is the first thing in the packed
 * representation.
 */
static u32
iebuck(Index *ix, u8 *b, IBucket *ib, IEStream *ies)
{
	USED(ies);
	USED(ib);
	return hashbits(b, 32) / ix->div;
}

/*
 * Fill ib with the next bucket in the stream.
 */
u32
buildbucket(Index *ix, IEStream *ies, IBucket *ib, u32 maxdata)
{
	IEntry ie1, ie2;
	u8 *b;
	u32 buck;

	buck = TWID32;
	ib->n = 0;
	while(ies->n){
		b = peekientry(ies);
		if(b == nil)
			return TWID32;
/* fprint(2, "b=%p ies->n=%lld ib.n=%d buck=%d score=%V\n", b, ies->n, ib->n, iebuck(ix, b, ib, ies), b); */
		if(ib->n == 0)
			buck = iebuck(ix, b, ib, ies);
		else{
			if(buck != iebuck(ix, b, ib, ies))
				break;
			if(ientrycmp(&ib->data[(ib->n - 1)* IEntrySize], b) == 0){
				/*
				 * guess that the larger address is the correct one to use
				 */
				unpackientry(&ie1, &ib->data[(ib->n - 1)* IEntrySize]);
				unpackientry(&ie2, b);
				seterr(EOk, "duplicate index entry for score=%V type=%d", ie1.score, ie1.ia.type);
				ib->n--;
				if(ie1.ia.addr > ie2.ia.addr)
					memmove(b, &ib->data[ib->n * IEntrySize], IEntrySize);
			}
		}
		if((ib->n+1)*IEntrySize > maxdata){
			seterr(EOk, "bucket overflow");
			return TWID32;
		}
		memmove(&ib->data[ib->n * IEntrySize], b, IEntrySize);
		ib->n++;
		ies->n--;
		ies->pos += IEntrySize;
	}
	return buck;
}
