/*
 * Kernel Debug Core
 *
 * Maintainer: Jason Wessel <jason.wessel@windriver.com>
 *
 * Copyright (C) 2000-2001 VERITAS Software Corporation.
 * Copyright (C) 2002-2004 Timesys Corporation
 * Copyright (C) 2003-2004 Amit S. Kale <amitkale@linsyssoft.com>
 * Copyright (C) 2004 Pavel Machek <pavel@ucw.cz>
 * Copyright (C) 2004-2006 Tom Rini <trini@kernel.crashing.org>
 * Copyright (C) 2004-2006 LinSysSoft Technologies Pvt. Ltd.
 * Copyright (C) 2005-2009 Wind River Systems, Inc.
 * Copyright (C) 2007 MontaVista Software, Inc.
 * Copyright (C) 2008 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 * Contributors at various stages not listed above:
 *  Jason Wessel ( jason.wessel@windriver.com )
 *  George Anzinger <george@mvista.com>
 *  Anurekh Saxena (anurekh.saxena@timesys.com)
 *  Lake Stevens Instrument Division (Glenn Engel)
 *  Jim Kingdon, Cygnus Support.
 *
 * Original KGDB stub: David Grothe <dave@gcom.com>,
 * Tigran Aivazian <tigran@sco.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <u.h>
#include <libc.h>
#include <ureg.h>
#include <ctype.h>

#include "debug_core.h"
#include "gdb.h"

/* all because gdb has stupid register layouts. Too bad. */

static char *
gdb_hex_reg_helper(uintptr_t *gdb_regs, int regnum, char *out)
{
	if (regnum <= GDB_PC) {
		return mem2hex((void *)&gdb_regs[regnum], out, sizeof(uintptr_t));
	}

	if (regnum <= GDB_GS) {
		uint32_t* reg32base = (uint32_t*)&gdb_regs[GDB_PS];
		int reg32idx = regnum - GDB_PS;
		return mem2hex((void *)&reg32base[reg32idx], out, sizeof(uint32_t));
	}

	memset(out, 0, sizeof(uint32_t));
	return nil;
}

/* Handle the 'p' individual regster get */
void
gdb_cmd_reg_get(struct state *ks)
{
	unsigned long regnum;
	char *ptr = (char *)&remcom_in_buffer[1];

	hex2long(&ptr, &regnum);
	syslog(0, "gdbserver", "Get reg %p: ", regnum);
	if (regnum >= DBG_MAX_REG_NUM) {
		syslog(0, "gdbserver", "fails");
		error_packet(remcom_out_buffer, Einval);
		return;
	}
	syslog(0, "gdbserver", "returns :%s:", ptr);
	gdb_hex_reg_helper(ks->gdbregs, regnum, (char *)ptr);
}


/* Handle the 'P' individual regster set */
void
gdb_cmd_reg_set(struct state *ks)
{
	fprint(2, "%s: NOET YET\n", __func__);
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

uint64_t
arch_get_reg(struct state *ks, int regnum) {
	uint64_t value = 0;
	if (regnum <= GDB_PC) {
		value = ((uint64_t*)ks->gdbregs)[regnum];

	} else if (regnum <= GDB_GS) {
		uint32_t* reg32base = (uint32_t*)&(((uint64_t*)ks->gdbregs)[GDB_PS]);
		int reg32idx = regnum - GDB_PS;
		value = reg32base[reg32idx];
	}

	return value;
}

uint64_t
arch_get_pc(struct state *ks)
{
	uint64_t pc = ((uint64_t*)ks->gdbregs)[GDB_PC];
	syslog(0, "gdbserver", "get_pc: %p", pc);
	return pc;
}

void arch_set_pc(uintptr_t *regs, unsigned long pc)
{
	// not yet.
}

char *
gpr(struct state *ks, int pid)
{
	if (ks->gdbregs == nil)
		ks->gdbregs = malloc(NUMREGBYTES);

	if (pid <= 0) {
		syslog(0, "gdbserver", "%s: FUCK. pid <= 0", __func__);
		pid = 1;
	}
	char *cp = ks->gdbregs;
	char *regname = smprint("/proc/%d/gdbregs", pid);
	int fd = open(regname, 0);
	if (fd < 0) {
		syslog(0, "gdbserver", "open(%s, 0): %r", regname);
		return errstring(Enoent);
	}

	if (pread(fd, cp, NUMREGBYTES, 0) < NUMREGBYTES){
		close(fd);
		return errstring(Eio);
	}
	close(fd);

	return nil;

}
