#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <auth.h>
#include <fcall.h>
#include <plumb.h>
#include "dat.h"
#include "fns.h"

static	Block	*blist;

int
tempfile(void)
{
	char dir[DIRLEN];
	char buf[128];
	int i, fd;

	snprint(buf, sizeof buf, "/tmp/X%d.%.4sacme", getpid(), getuser());
	for(i='A'; i<='Z'; i++){
		buf[5] = i;
		if(stat(buf, dir) == 0)
			continue;
		fd = create(buf, ORDWR|ORCLOSE|OCEXEC, 0600);
		if(fd >= 0)
			return fd;
	}
	return -1;
}

Disk*
diskinit(void)
{
	Disk *d;

	d = emalloc(sizeof(Disk));
	d->fd = tempfile();
	if(d->fd < 0){
		fprint(2, "acme: can't create temp file: %r\n");
		exits("diskinit");
	}
	return d;
}

static
uint
ntosize(uint n, uint *ip)
{
	uint size;

	if(n > Maxblock)
		error("internal error: ntosize");
	size = n;
	if(size & (Blockincr-1))
		size += Blockincr - (size & (Blockincr-1));
	/* last bucket holds blocks of exactly Maxblock */
	if(ip)
		*ip = size/Blockincr;
	return size * sizeof(Rune);
}

Block*
disknewblock(Disk *d, uint n)
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
			blist = emalloc(100*sizeof(Block));
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
diskrelease(Disk *d, Block *b)
{
	uint i;

	ntosize(b->n, &i);
	b->next = d->free[i];
	d->free[i] = b;
}

void
diskwrite(Disk *d, Block **bp, Rune *r, uint n)
{
	int size, nsize;
	Block *b;

	b = *bp;
	size = ntosize(b->n, nil);
	nsize = ntosize(n, nil);
	if(size != nsize){
		diskrelease(d, b);
		b = disknewblock(d, n);
		*bp = b;
	}
	if(seek(d->fd, b->addr, 0) < 0)
		error("seek error in temp file");
	if(write(d->fd, r, n*sizeof(Rune)) != n*sizeof(Rune))
		error("write error to temp file");
	b->n = n;
}

void
diskread(Disk *d, Block *b, Rune *r, uint n)
{
	if(n > b->n)
		error("internal error: diskread");

	ntosize(b->n, nil);
	if(seek(d->fd, b->addr, 0) < 0)
		error("seek error in temp file");
	if(read(d->fd, r, n*sizeof(Rune)) != n*sizeof(Rune))
		error("read error from temp file");
}
