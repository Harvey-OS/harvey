/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

#include <u.h>
#include <libc.h>

int
setjmp(jmp_buf buf)
{
	return __builtin_setjmp(buf);
}

void
longjmp(jmp_buf buf, int n)
{
	return __builtin_longjmp(buf, 1);
}
