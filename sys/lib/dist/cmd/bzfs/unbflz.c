#include <u.h>
#include <libc.h>
#include <bio.h>
#include "bzfs.h"

int
Bgetint(Biobuf *b)
{
	uchar p[4];

	if(Bread(b, p, 4) != 4)
		sysfatal("short read");
	return (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3];
}

/*
 * memmove but make sure overlap works properly.
 */
void
copy(uchar *dst, uchar *src, int n)
{
	while(n-- > 0)
		*dst++ = *src++;
}

int
unbflz(int in)
{
	int rv, out, p[2];
	Biobuf *b, bin;
	char buf[5];
	uchar *data;
	int i, j, length, n, m, o, sum;
	ulong *blk;
	int nblk, mblk;

	if(pipe(p) < 0)
		sysfatal("pipe: %r");

	rv = p[0];
	out = p[1];
	switch(rfork(RFPROC|RFFDG|RFNOTEG|RFMEM)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		close(rv);
		break;
	default:
		close(in);
		close(out);
		return rv;
	}

	Binit(&bin, in, OREAD);
	b = &bin;

	if(Bread(b, buf, 4) != 4)
		sysfatal("short read");

	if(memcmp(buf, "BLZ\n", 4) != 0)
		sysfatal("bad header");

	length = Bgetint(b);
	data = malloc(length);
	if(data == nil)
		sysfatal("out of memory");
	sum = 0;
	nblk = 0;
	mblk = 0;
	blk = nil;
	while(sum < length){
		if(nblk>=mblk){
			mblk += 16384;
			blk = realloc(blk, (mblk+1)*sizeof(blk[0]));
			if(blk == nil)
				sysfatal("out of memory");
		}
		n = Bgetint(b);
		blk[nblk++] = n;
		if(n&(1<<31))
			n &= ~(1<<31);
		else
			blk[nblk++] = Bgetint(b);
		sum += n;
	}
	if(sum != length)
		sysfatal("bad compressed data %d %d", sum, length);
	i = 0;
	j = 0;
	while(i < length){
		assert(j < nblk);
		n = blk[j++];
		if(n&(1<<31)){
			n &= ~(1<<31);
			if((m=Bread(b, data+i, n)) != n)
				sysfatal("short read %d %d", n, m);
		}else{
			o = blk[j++];
			copy(data+i, data+o, n);
		}
		i += n;
	}
	write(out, data, length);
	close(in);
	close(out);
	_exits(0);
	return -1;
}
