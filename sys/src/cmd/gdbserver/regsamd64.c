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


Reg gdbregs[] = {
	{ GDB_AX,	"AX",	8,	0,	},
	{ GDB_BX,	"BX",	8, 	0,	},
	{ GDB_CX,	"CX",	8, 	0,	},
	{ GDB_DX,	"DX",	8, 	0,	},
	{ GDB_SI,	"SI",	8, 	0,	},
	{ GDB_DI,	"DI",	8, 	0,	},
	{ GDB_BP,	"BP",	8, 	0,	},
	{ GDB_SP,	"SP",	8, 	0,	},
	{ GDB_R8,	"R8",	8, 	0,	},
	{ GDB_R9,	"R9",	8, 	0,	},
	{ GDB_R10,	"R10",	8, 	0,	},
	{ GDB_R11,	"R11",	8, 	0,	},
	{ GDB_R12,	"R12",	8, 	0,	},
	{ GDB_R13,	"R13",	8, 	0,	},
	{ GDB_R14,	"R14",	8, 	0,	},
	{ GDB_R15,	"R15",	8, 	0,	},
	{ GDB_PC,	"PC",	8, 	0,	},
	{ GDB_PS,	"PS",	4, 	0,	},
	{ GDB_CS,	"CS",	4, 	0,	},
	{ GDB_SS,	"SS",	4, 	0,	},
	{ GDB_DS,	"DS",	4, 	0,	},
	{ GDB_ES,	"ES",	4, 	0,	},
	{ GDB_FS,	"FS",	4, 	0,	},
	{ GDB_GS,	"GS",	4, 	0,	},
};

/* 17 64 bit regs and 7 32 bit regs */
#define NUMGPREGBYTES ((17 * 8) + (7 * 4))

void gdb_init_regs(void) {
	int offset = 0;
	for (int i = 0; i < GDB_MAX_REG; i++) {
		gdbregs[i].offset = offset;
		offset += gdbregs[i].size;
	}
}

/* all because gdb has stupid register layouts. Too bad. */

static char *
gdb_hex_reg_helper(uintptr_t *gdb_regs, int regnum, char *out)
{
	if (regnum >= GDB_MAX_REG) {
		memset(out, 0, sizeof(uint32_t));
		return nil;
	}

	Reg *reg = &gdbregs[regnum];
	uintptr_t regaddr = (uintptr_t)gdb_regs + reg->offset;
	return mem2hex((void *)regaddr, out, reg->size);
}

/* Handle the 'p' individual register get */
void
gdb_cmd_reg_get(struct state *ks)
{
	unsigned long regnum;
	char *ptr = (char *)&remcom_in_buffer[1];

	hex2long(&ptr, &regnum);
	syslog(0, "gdbserver", "Get reg %p: ", regnum);
	
	if (gdb_hex_reg_helper(ks->gdbregs, regnum, (char *)ptr)) {
		syslog(0, "gdbserver", "returns :%s:", ptr);
	} else {
		syslog(0, "gdbserver", "fails");
		error_packet(remcom_out_buffer, Einval);
	}
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
	if (regnum >= GDB_MAX_REG) {
		return 0;
	}

	Reg *reg = &gdbregs[regnum];
	uintptr_t regaddr = (uintptr_t)ks->gdbregs + reg->offset;
	if (reg->size == 4) {
		return *(uint32_t*)regaddr;
	} else if (reg->size == 8) {
		return *(uint64_t*)regaddr;
	}

	return 0;
}

uint64_t
arch_get_pc(struct state *ks)
{
	uint64_t pc = arch_get_reg(ks, GDB_PC);
	syslog(0, "gdbserver", "get_pc: %p", pc);
	return pc;
}

void arch_set_pc(uintptr_t *regs, unsigned long pc)
{
	// not yet.
}

// Copy GP and FP registers into a single block in state ks.
char *
gpr(struct state *ks, int pid)
{
	if (ks->gdbregs == nil) {
		ks->gdbregs = malloc(NUMGPREGBYTES);
		ks->gdbregsize = NUMGPREGBYTES;
	}

	if (pid <= 0) {
		syslog(0, "gdbserver", "%s: FUCK. pid <= 0", __func__);
		pid = 1;
	}

	// Read GP regs
	char *cp = ks->gdbregs;
	char *regname = smprint("/proc/%d/gdbregs", pid);
	int fd = open(regname, 0);
	if (fd < 0) {
		syslog(0, "gdbserver", "open(%s, 0): %r", regname);
		free(regname);
		return errstring(Enoent);
	}
	free(regname);

	if (pread(fd, cp, NUMGPREGBYTES, 0) < NUMGPREGBYTES){
		close(fd);
		return errstring(Eio);
	}
	close(fd);

	return nil;
}
