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

void
getcallstack(usize *pcs, usize npcs)
{
	assert(npcs < 6);

#if 0
	if (npcs > 0)
		pcs[0] = (usize)__builtin_return_address(2);
	if (npcs > 1)
		pcs[1] = (usize)__builtin_return_address(3);
	if (npcs > 2)
		pcs[2] = (usize)__builtin_return_address(4);
	if (npcs > 3)
		pcs[3] = (usize)__builtin_return_address(5);
	if (npcs > 4)
		pcs[4] = (usize)__builtin_return_address(6);
	if (npcs > 5)
		pcs[5] = (usize)__builtin_return_address(7);
#else
	if (npcs > 0)
		pcs[0] = (usize)0;
	if (npcs > 1)
		pcs[1] = (usize)0;
	if (npcs > 2)
		pcs[2] = (usize)0;
	if (npcs > 3)
		pcs[3] = (usize)0;
	if (npcs > 4)
		pcs[4] = (usize)0;
	if (npcs > 5)
		pcs[5] = (usize)0;
#endif
	if (npcs > 6)
		sysfatal("getcallstack: stack size must be <= 6, got %zu", npcs);
}
