#include "stdinc.h"
#include "dat.h"
#include "fns.h"

struct IEStream
{
	Part	*part;
	u64int	off;		/* read position within part */
	u64int	n;		/* number of valid ientries left to read */
	u32int	size;		/* allocated space in buffer */
	u8int	*buf;
	u8int	*pos;		/* current place in buffer */
	u8int	*epos;		/* end of valid buffer contents */
};

IEStream*
initIEStream(Part *part, u64int off, u64int clumps, u32int size)
{
	IEStream *ies;

//ZZZ out of memory?
	ies = MKZ(IEStream);
	ies->buf = MKN(u8int, size);
	ies->epos = ies->buf;
	ies->pos = ies->epos;
	ies->off = off;
	ies->n = clumps;
	ies->size = size;
	ies->part = part;
	return ies;
}

void
freeIEStream(IEStream *ies)
{
	if(ies == nil)
		return;
	free(ies->buf);
	free(ies);
}

static u8int*
peekIEntry(IEStream *ies)
{
	u32int n, nn;

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
		if(!readPart(ies->part, ies->off, ies->epos, nn)){
			setErr(EOk, "can't read sorted index entries: %R");
			return nil;
		}
		ies->epos += nn;
		ies->off += nn;
	}
	return ies->pos;
}

static u32int
ieBuck(Index *ix, u8int *b)
{
	return hashBits(b, 32) / ix->div;
}

u32int
buildBucket(Index *ix, IEStream *ies, IBucket *ib)
{
	IEntry ie1, ie2;
	u8int *b;
	u32int buck;

	buck = TWID32;
	ib->n = 0;
	ib->next = 0;
	while(ies->n){
		b = peekIEntry(ies);
		if(b == nil)
			return TWID32;
//fprint(2, "b=%p ies->n=%lld ib.n=%d buck=%d score=%V\n", b, ies->n, ib->n, ieBuck(ix, b), b);
		if(ib->n == 0)
			buck = ieBuck(ix, b);
		else{
			if(buck != ieBuck(ix, b))
				break;
			if(ientryCmp(&ib->data[(ib->n - 1)* IEntrySize], b) == 0){
				/*
				 * guess that the larger address is the correct one to use
				 */
				unpackIEntry(&ie1, &ib->data[(ib->n - 1)* IEntrySize]);
				unpackIEntry(&ie2, b);
				setErr(EOk, "duplicate index entry for score=%V type=%d\n", ie1.score, ie1.ia.type);
				ib->n--;
				if(ie1.ia.addr > ie2.ia.addr)
					memmove(b, &ib->data[ib->n * IEntrySize], IEntrySize);
			}
		}
		memmove(&ib->data[ib->n * IEntrySize], b, IEntrySize);
		ib->n++;
		ies->n--;
		ies->pos += IEntrySize;
	}
	return buck;
}
