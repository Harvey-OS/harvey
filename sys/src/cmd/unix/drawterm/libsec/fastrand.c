/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<libsec.h>

/* 
 *  use the X917 random number generator to create random
 *  numbers (faster than truerand() but not as random).
 */
ulong
fastrand(void)
{
	ulong x;
	
	genrandom((uchar*)&x, sizeof x);
	return x;
}
