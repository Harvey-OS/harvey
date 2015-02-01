/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

unsigned long
nptohl(void *p)
{
	unsigned char *up;
	unsigned long x;

	up = p;
	x = (up[0]<<24)|(up[1]<<16)|(up[2]<<8)|up[3];
	return x;
}
