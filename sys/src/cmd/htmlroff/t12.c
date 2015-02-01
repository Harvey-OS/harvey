/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "a.h"

/*
 * 12. Overstrike, bracket, line-drawing, graphics, and zero-width functions.
 */

/*
	\o'asdf'
	\zc
	\b'asdf'
	\l'Nc'
	\L'Nc'
	\D'xxx'
*/

int
e_o(void)
{
	pushinputstring(getqarg());
	return 0;
}

int
e_z(void)
{
	getnext();
	return 0;
}

int
e_b(void)
{
	pushinputstring(getqarg());
	return 0;
}

int
e_l(void)
{
	getqarg();
	return 0;
}

int
e_L(void)
{
	getqarg();
	return 0;
}

int
e_D(void)
{
	getqarg();
	return 0;
}

void
t12init(void)
{
	addesc('o', e_o, 0);
	addesc('z', e_z, 0);
	addesc('b', e_b, 0);
	addesc('l', e_l, 0);
	addesc('L', e_L, 0);
	addesc('D', e_D, 0);
}
