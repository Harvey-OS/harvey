#include <u.h>
#include <libc.h>

static
long
ioreadv(int fd, IOchunk *io, int nio, vlong offset)
{
	int i;
	long m, n, tot;
	char *buf, *p;

	tot = 0;
	for(i=0; i<nio; i++)
		tot += io[i].len;
	buf = malloc(tot);
	if(buf == nil)
		return -1;

	tot = pread(fd, buf, tot, offset);

	p = buf;
	n = tot;
	for(i=0; i<nio; i++){
		if(n <= 0)
			break;
		m = io->len;
		if(m > n)
			m = n;
		memmove(io->addr, p, m);
		n -= m;
		p += m;
		io++;
	}

	free(buf);
	return tot;
}

long
readv(int fd, IOchunk *io, int nio)
{
	return ioreadv(fd, io, nio, -1LL);
}

long
preadv(int fd, IOchunk *io, int nio, vlong off)
{
	return ioreadv(fd, io, nio, off);
}
