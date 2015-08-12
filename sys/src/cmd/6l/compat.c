/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "l.h"

/*
 * fake malloc
 */
void *
malloc(uint32_t n)
{
	void *p;

	while(n & 7)
		n++;
	while(nhunk < n)
		gethunk();
	p = hunk;
	nhunk -= n;
	hunk += n;
	return p;
}

void
free(void *p)
{
	USED(p);
}

void *
calloc(uint32_t m, uint32_t n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

void *
realloc(void *, uint32_t)
{
	fprint(2, "realloc called\n");
	abort();
	return 0;
}

void *
mysbrk(uint32_t size)
{
	return sbrk(size);
}

void
setmalloctag(void *, uint32_t)
{
}

int
fileexists(char *s)
{
	uint8_t dirbuf[400];

	/* it's fine if stat result doesn't fit in dirbuf, since even then the file exists */
	return stat(s, dirbuf, sizeof(dirbuf)) >= 0;
}
