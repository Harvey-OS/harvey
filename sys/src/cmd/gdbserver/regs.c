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

#include "debug_core.h"
#include "gdb.h"

Reg *
gdb_get_reg_by_name(char *reg)
{
	for (int i = 0; i < GDB_MAX_REG; i++) {
		Reg *r = &gdbregs[i];
		if (!strcmp(reg, r->name)) {
			return r;
		}
	}

	return nil;
}

Reg *
gdb_get_reg_by_id(int id)
{
	for (int i = 0; i < GDB_MAX_REG; i++) {
		Reg *r = &gdbregs[i];
		if (id == r->idx) {
			return r;
		}
	}

	return nil;
}
