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


// TODO
Reg gdbregs[] = {
};

void gdb_init_regs(void) {
}

char *
gdb_hex_reg_helper(GdbState *ks, int regnum, char *out)
{
	fprint(2, "%s: NOT YET\n", __func__);
	memset(out, 0, sizeof(u32));
	return nil;
}

u64
arch_get_reg(GdbState *ks, int regnum)
{
	return 0;
}

u64
arch_get_pc(GdbState *ks)
{
	// not yet.
	return 0;
}

void arch_set_pc(uintptr_t *regs, unsigned long pc)
{
	// not yet.
}

char *
gpr(GdbState *ks, int pid)
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
		syslog(0, "gdbserver", "open(%s, 0): %r\n", regname);
		return errstring(Enoent);
	}

	if (pread(fd, cp, NUMREGBYTES, 0) < NUMREGBYTES){
		close(fd);
		return errstring(Eio);
	}
	close(fd);

	return nil;

}
