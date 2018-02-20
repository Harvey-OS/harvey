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
	// Size and offset refer to /proc/xx/gdbregs structure
	{ GDB_AX,	"AX",		0,	8,	0,	},
	{ GDB_BX,	"BX",		0,	8, 	0,	},
	{ GDB_CX,	"CX",		0,	8, 	0,	},
	{ GDB_DX,	"DX",		0,	8, 	0,	},
	{ GDB_SI,	"SI",		0,	8, 	0,	},
	{ GDB_DI,	"DI",		0,	8, 	0,	},
	{ GDB_BP,	"BP",		0,	8, 	0,	},
	{ GDB_SP,	"SP",		0,	8, 	0,	},
	{ GDB_R8,	"R8",		0,	8, 	0,	},
	{ GDB_R9,	"R9",		0,	8, 	0,	},
	{ GDB_R10,	"R10",		0,	8, 	0,	},
	{ GDB_R11,	"R11",		0,	8, 	0,	},
	{ GDB_R12,	"R12",		0,	8, 	0,	},
	{ GDB_R13,	"R13",		0,	8, 	0,	},
	{ GDB_R14,	"R14",		0,	8, 	0,	},
	{ GDB_R15,	"R15",		0,	8, 	0,	},
	{ GDB_PC,	"PC",		0,	8, 	0,	},
	{ GDB_PS,	"PS",		0,	4, 	0,	},
	{ GDB_CS,	"CS",		0,	4, 	0,	},
	{ GDB_SS,	"SS",		0,	4, 	0,	},
	{ GDB_DS,	"DS",		0,	4, 	0,	},
	{ GDB_ES,	"ES",		0,	4, 	0,	},
	{ GDB_FS,	"FS",		0,	4, 	0,	},
	{ GDB_GS,	"GS",		0,	4, 	0,	},
	// Size and offset refer to /proc/xx/fpregs structure
	{ GDB_STMM0,	"STMM0",	0,	10,	0,	},
	{ GDB_STMM1,	"STMM1",	0,	10,	0,	},
	{ GDB_STMM2,	"STMM2",	0,	10,	0,	},
	{ GDB_STMM3,	"STMM3",	0,	10,	0,	},
	{ GDB_STMM4,	"STMM4",	0,	10,	0,	},
	{ GDB_STMM5,	"STMM5",	0,	10,	0,	},
	{ GDB_STMM6,	"STMM6",	0,	10,	0,	},
	{ GDB_STMM7,	"STMM7",	0,	10,	0,	},
	{ GDB_FCTRL,	"FCTRL",	1,	4,	0,	},	// GDB_FCW = GDB_FCTRL
	{ GDB_FSTAT,	"FSTAT",	1,	4,	0,	},	// GDB_FSW = GDB_FSTAT
	{ GDB_FTAG,	"FTAG",		1,	4,	0,	},	// GDB_FTW = GDB_FTAG
	{ GDB_FISEG,	"FISEG",	1,	4,	0,	},	// GDB_FPU_CS = GDB_FISEG
	{ GDB_FIOFF,	"FIOFF",	1,	4,	0,	},	// GDB_IP = GDB_FIOFF
	{ GDB_FOSEG,	"FOSEG",	1,	4,	0,	},	// GDB_FPU_DS = GDB_FOSEG
	{ GDB_FOOFF,	"FOOFF",	1,	4,	0,	},	// GDB_DP = GDB_FOOFF
	{ GDB_FOP,	"FOP",		1,	4,	0,	},
	{ GDB_XMM0,	"XMM0",		0,	32,	0,	},
	{ GDB_XMM1,	"XMM1",		0,	32,	0,	},
	{ GDB_XMM2,	"XMM2",		0,	32,	0,	},
	{ GDB_XMM3,	"XMM3",		0,	32,	0,	},
	{ GDB_XMM4,	"XMM4",		0,	32,	0,	},
	{ GDB_XMM5,	"XMM5",		0,	32,	0,	},
	{ GDB_XMM6,	"XMM6",		0,	32,	0,	},
	{ GDB_XMM7,	"XMM7",		0,	32,	0,	},
	{ GDB_XMM8,	"XMM8",		0,	32,	0,	},
	{ GDB_XMM9,	"XMM9",		0,	32,	0,	},
	{ GDB_XMM10,	"XMM10",	0,	32,	0,	},
	{ GDB_XMM11,	"XMM11",	0,	32,	0,	},
	{ GDB_XMM12,	"XMM12",	0,	32,	0,	},
	{ GDB_XMM13,	"XMM13",	0,	32,	0,	},
	{ GDB_XMM14,	"XMM14",	0,	32,	0,	},
	{ GDB_XMM15,	"XMM15",	0,	32,	0,	},
	{ GDB_MXCSR,	"MXCSR",	0,	4,	0,	},
	{ GDB_ORIGRAX,	"ORIGRAX",	1,	8,	0,	},
	{ GDB_FSBASE,	"FSBASE",	1,	8,	0,	},
	{ GDB_GSBASE,	"GSBASE",	1,	8,	0,	},
};


/* 17 64 bit regs and 7 32 bit regs */
#define NUMGPREGBYTES ((17 * 8) + (7 * 4))
// Based on size of Fxsave
#define NUMFPREGBYTES 512


void gdb_init_regs(void) {
	// We can work out the GP register offsets easily.
	int offset = 0;
	for (int i = GDB_FIRST_GP_REG; i <= GDB_LAST_GP_REG; i++) {
		gdbregs[i].offset = offset;
		offset += gdbregs[i].size;
	}

	// FP register offsets are slightly different, in this case based off
	// the Fxsave structure.
	int numMmxRegs = GDB_STMM7 - GDB_STMM0 + 1;
	for (int i = 0; i < numMmxRegs; i++) {
		gdbregs[GDB_STMM0 + i].offset = 32 + i*16;
	}

	// The x87 registers are a bit fiddly, so we don't support them yet.
	// MXCSR is straightforward, so it's here.
	gdbregs[GDB_MXCSR].offset = 24;

	int numXmmRegs = GDB_XMM15 - GDB_XMM0 + 1;
	for (int i = 0; i < numXmmRegs; i++) {
		gdbregs[GDB_XMM0 + i].offset = 160 + i*32;
	}
}

char *
gdb_hex_reg_helper(GdbState *ks, int regnum, char *out) {
	if (regnum >= GDB_MAX_REG) {
		syslog(0, "gdbserver", "unknown reg: %d", regnum);
		memset(out, 0, sizeof(uint32_t));
		return nil;
	}

	Reg *reg = &gdbregs[regnum];

	if (reg->unsupported) {
		syslog(0, "gdbserver", "unsupported reg: %d", regnum);
		return zerohex(out, reg->size);
	}

	uintptr_t regaddr;
	if (regnum <= GDB_LAST_GP_REG) {
		// GP register
		syslog(0, "gdbserver", "gp reg: %d", regnum);
		regaddr = (uintptr_t)ks->gdbregs + reg->offset;
	} else {
		// FPU register
		syslog(0, "gdbserver", "fpu reg: %d", regnum);
		regaddr = (uintptr_t)ks->fpregs + reg->offset;
	}

	return mem2hex((void *)regaddr, out, reg->size);
}

uint64_t
arch_get_reg(GdbState *ks, int regnum) {
	if (regnum >= GDB_MAX_REG) {
		return 0;
	}

	Reg *reg = &gdbregs[regnum];

	uintptr_t regaddr;
	if (regnum <= GDB_LAST_GP_REG) {
		// GP register
		regaddr = (uintptr_t)ks->gdbregs + reg->offset;
	} else {
		// FPU register
		regaddr = (uintptr_t)ks->fpregs + reg->offset;
	}

	if (reg->size == 4) {
		return *(uint32_t*)regaddr;
	} else if (reg->size == 8) {
		return *(uint64_t*)regaddr;
	}

	return 0;
}

uint64_t
arch_get_pc(GdbState *ks)
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
gpr(GdbState *ks, int pid)
{
	if (ks->gdbregs == nil) {
		ks->gdbregs = malloc(NUMGPREGBYTES);
		ks->gdbregsize = NUMGPREGBYTES;

		ks->fpregs = malloc(NUMFPREGBYTES);
		ks->fpregsize = NUMFPREGBYTES;
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
		syslog(0, "gdbserver", "couldn't read %s", regname);
		close(fd);
		return errstring(Eio);
	}
	close(fd);

	// Read FP regs
	cp = ks->fpregs;
	regname = smprint("/proc/%d/fpregs", pid);
	fd = open(regname, 0);
	if (fd < 0) {
		syslog(0, "gdbserver", "open(%s, 0): %r", regname);
		free(regname);
		return errstring(Enoent);
	}
	free(regname);

	if (pread(fd, cp, NUMFPREGBYTES, 0) < NUMFPREGBYTES){
		syslog(0, "gdbserver", "couldn't read %s", regname);
		close(fd);
		return errstring(Eio);
	}
	close(fd);

	return nil;
}
