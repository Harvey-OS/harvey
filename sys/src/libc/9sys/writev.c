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
long
iowritev(int fd, IOchunk *io, int nio, vlong offset)
{
	int i;
	long tot;
	char *buf, *p;

	tot = 0;
	for(i=0; i<nio; i++)
		tot += io[i].len;
	buf = malloc(tot);
	if(buf == nil)
		return -1;

	p = buf;
	for(i=0; i<nio; i++){
		memmove(p, io->addr, io->len);
		p += io->len;
		io++;
	}

	tot = pwrite(fd, buf, tot, offset);

	free(buf);
	return tot;
}

long
writev(int fd, IOchunk *io, int nio)
{
	return iowritev(fd, io, nio, -1LL);
}

long
pwritev(int fd, IOchunk *io, int nio, vlong off)
{
	return iowritev(fd, io, nio, off);
}
