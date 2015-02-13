/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>

static
int32_t
ioreadv(int fd, IOchunk *io, int nio, int64_t offset)
{
	int i;
	int32_t m, n, tot;
	int8_t *buf, *p;

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

int32_t
readv(int fd, IOchunk *io, int nio)
{
	return ioreadv(fd, io, nio, -1LL);
}

int32_t
preadv(int fd, IOchunk *io, int nio, int64_t off)
{
	return ioreadv(fd, io, nio, off);
}
