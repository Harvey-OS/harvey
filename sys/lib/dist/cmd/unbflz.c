#include <u.h>
#include <libc.h>
#include <bio.h>

void
usage(void)
{
	fprint(2, "usage: unbflz [file]\n");
	exits("usage");
}

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

void
main(int argc, char **argv)
{
	Biobuf *b, bin;
	char buf[5];
	uchar *data;
	ulong *blk, l;
	int nblk, mblk;
	int sum;
	int i, j, length, m, n, o;

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	switch(argc){
	default:
		usage();
	case 0:
		Binit(&bin, 0, OREAD);
		b = &bin;
		break;
	case 1:
		if((b = Bopen(argv[0], OREAD)) == nil)
			sysfatal("open %s: %r", argv[0]);
		break;
	}

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
		l = Bgetint(b);
		blk[nblk++] = l;
		if(l&(1<<31))
			l &= ~(1<<31);
		else
			blk[nblk++] = Bgetint(b);
		sum += l;
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
	write(1, data, length);
	exits(nil);
}
