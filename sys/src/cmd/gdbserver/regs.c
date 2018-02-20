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
	if (id < GDB_MAX_REG) {
		return &gdbregs[id];
	}
	return nil;
}

/* Handle the 'p' individual register get */
void
gdb_cmd_reg_get(GdbState *ks)
{
	unsigned long regnum;
	char *ptr = (char *)&remcom_in_buffer[1];

	hex2long(&ptr, &regnum);
	syslog(0, "gdbserver", "Get reg %p: ", regnum);
	
	if (gdb_hex_reg_helper(ks, regnum, (char *)remcom_out_buffer)) {
		syslog(0, "gdbserver", "returns :%s:", remcom_out_buffer);
	} else {
		syslog(0, "gdbserver", "fails");
		error_packet(remcom_out_buffer, Einval);
	}
}

/* Handle the 'P' individual regster set */
void
gdb_cmd_reg_set(GdbState *ks)
{
	fprint(2, "%s: NOT YET\n", __func__);
#if 0 // not yet.
	unsigned long regnum;
	char *ptr = &remcom_in_buffer[1];
	int i = 0;

	hex2long(&ptr, &regnum);
	if (*ptr++ != '=' ||
		!dbg_get_reg(regnum, gdb_regs, ks->linux_regs)) {
		error_packet(remcom_out_buffer, -EINVAL);
		return;
	}
	memset(gdb_regs, 0, sizeof(gdb_regs));
	while (i < sizeof(gdb_regs) * 2)
		if (hex_to_bin(ptr[i]) >= 0)
			i++;
		else
			break;
	i = i / 2;
	hex2mem(ptr, (char *)gdb_regs, i);
	dbg_set_reg(regnum, gdb_regs, ks->linux_regs);
#endif
	strcpy((char *)remcom_out_buffer, "OK");
}
