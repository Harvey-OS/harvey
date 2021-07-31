#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int chattyzero;

enum
{
	HugeIoSize = 2*1024*1024
};
void
zeroPart(Part *part, int blockSize)
{
	ZBlock *b;
	u64int addr;
	int w;
	int iosize;

	iosize = HugeIoSize;
{char *p = getenv("XXXIOSIZE"); if(p) iosize = atoi(p); assert(iosize>0);}
	fprint(2, "clearing the partition\n");

	b = allocZBlock(iosize, 1);

	w = 0;
	for(addr = PartBlank; addr + iosize <= part->size; addr += iosize){
		if(!writePart(part, addr, b->data, iosize))
			fatal("can't initialize %s, writing block %d failed: %r", part->name, w);
		w++;
		if(chattyzero)
			print("%lld %lld\n", addr, part->size);
	}

	for(; addr + blockSize <= part->size; addr += blockSize){
		if(!writePart(part, addr, b->data, blockSize))
			fatal("can't initialize %s: %r", part->name);
		if(chattyzero)
			print("%lld %lld\n", addr, part->size);
	}

	freeZBlock(b);
}
