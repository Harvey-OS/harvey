#include "stdinc.h"
#include "dat.h"
#include "fns.h"

void
zeroPart(Part *part, int blockSize)
{
	ZBlock *b;
	u64int addr;
	int w;

	fprint(2, "clearing the partition\n");

	b = allocZBlock(MaxIoSize, 1);

	w = 0;
	for(addr = PartBlank; addr + MaxIoSize <= part->size; addr += MaxIoSize){
		if(!writePart(part, addr, b->data, MaxIoSize))
			fatal("can't initialize %s, writing block %d failed: %r", part->name, w);
		w++;
	}

	for(; addr + blockSize <= part->size; addr += blockSize)
		if(!writePart(part, addr, b->data, blockSize))
			fatal("can't initialize %s: %r", part->name);

	freeZBlock(b);
}
