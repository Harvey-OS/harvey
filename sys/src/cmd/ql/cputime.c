/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

double
cputime(void)
{
	int32_t t[4];
	int32_t times(int32_t*);
	int i;

	times(t);
	for(i=1; i<4; i++)
		t[0] += t[i];
	return t[0] / 100.;
}

int32_t
seek(int f, int32_t o, int p)
{
	int32_t lseek(int, int32_t, int);

	return lseek(f, o, p);
}

int
create(int8_t *n, int m, int32_t p)
{
	int creat(int8_t*, int);

	if(m != 1)
		return -1;
	return creat(n, p);
}
