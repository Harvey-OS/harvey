#include <alef.h>
#include <libg.h>
#include <frame.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

intern	Block	*blist;

Disk*
Disk.init()
{
	byte buf[128];
	int i;
	byte dir[DIRLEN];
	Disk *d;

	d = malloc(sizeof(Disk));
	sprint(buf, "/tmp/X%d.%.4sacme", getpid(), getuser());
	for(i='A'; i<='Z'; i++){
		buf[5] = i;
		if(stat(buf, dir) == 0)
			continue;
		d->fd = create(buf, ORDWR|ORCLOSE|OCEXEC, 0600);
		if(d->fd >= 0)
			break;
	}
	if(d->fd < 0){
		fprint(2, "acme: can't create temp file %s: %r\n", buf);
		exits("disk.init");
	}
	return d;
}

intern
uint
ntosize(uint n, uint *ip)
{
	uint size;

	check n <= Maxblock;
	size = n;
	if(size & (Blockincr-1))
		size += Blockincr - (size & (Blockincr-1));
	/* last bucket holds blocks of exactly Maxblock */
	if(ip)
		*ip = size/Blockincr;
	return size * sizeof(Rune);
}

Block*
Disk.new(Disk *d, uint n)
{
	uint i, j, size;
	Block *b;

	size = ntosize(n, &i);
	b = d->free[i];
	if(b)
		d->free[i] = b->next;
	else{
		/* allocate in chunks to reduce malloc overhead */
		if(blist == nil){
			blist = malloc(100*sizeof(Block));
			for(j=0; j<100-1; j++)
				blist[j].next = &blist[j+1];
		}
		b = blist;
		blist = b->next;
		b->addr = d->addr;
		d->addr += size;
	}
	b->n = n;
	return b;
}

void
Disk.release(Disk *d, Block *b)
{
	uint i;

	ntosize(b->n, &i);
	b->next = d->free[i];
	d->free[i] = b;
}

void
Disk.write(Disk *d, Block **bp, Rune *r, uint n)
{
	int size, nsize;
	Block *b;

	b = *bp;
	size = ntosize(b->n, nil);
	nsize = ntosize(n, nil);
	if(size != nsize){
		d->release(b);
		b = d->new(n);
		*bp = b;
	}
	if(seek(d->fd, b->addr, 0) < 0)
		error("seek error in temp file");
	if(write(d->fd, r, n*sizeof(Rune)) != n*sizeof(Rune))
		error("write error to temp file");
	b->n = n;
}

void
Disk.read(Disk *d, Block *b, Rune *r, uint n)
{
	check n<=b->n;
	ntosize(b->n, nil);
	if(seek(d->fd, b->addr, 0) < 0)
		error("seek error in temp file");
	if(read(d->fd, r, n*sizeof(Rune)) != n*sizeof(Rune))
		error("read error from temp file");
}
