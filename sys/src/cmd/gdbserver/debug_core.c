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
#include <bio.h>
#include <mach.h>

#include "debug_core.h"
#include "gdb.h"

#define NR_CPUS 32

struct debuggerinfo_struct info[NR_CPUS];

/**
 * connected - Is a host GDB connected to us?
 */
int connected;

/*
 * Holds information about breakpoints. These breakpoints are
 * added and removed by gdb.
 */
static struct bkpt breakpoints[MAX_BREAKPOINTS] = {
	[0 ... MAX_BREAKPOINTS - 1] = {.state = BP_UNDEFINED}
};

/*
 * The PID# of the active PID, or -1 if none:
 */
int active = -1;
/*
 * Finally, some KGDB code :-)
 */

/*
 * Weak aliases for breakpoint management,
 * can be overriden by architectures when needed:
 */
char *
arch_set_breakpoint(struct state *ks, struct bkpt *bpt)
{
	char *err;

	err = rmem(bpt->saved_instr, ks->threadid, bpt->bpt_addr, machdata->bpsize);
	if (err)
		return err;

	err = wmem(bpt->bpt_addr, ks->threadid, machdata->bpinst, machdata->bpsize);
	return err;
}

char *
arch_remove_breakpoint(struct state *ks, struct bkpt *bpt)
{
	return wmem(bpt->bpt_addr, ks->threadid, bpt->saved_instr, machdata->bpsize);
}

unsigned long
_arch_pc(int exception, uint64_t * regs)
{
	return regs[16];
}

/*
 * SW breakpoint management:
 */
char *
dbg_activate_sw_breakpoints(struct state *ks)
{
	char *error;
	char *ret = nil;
	int i;

	for (i = 0; i < MAX_BREAKPOINTS; i++) {
		if (breakpoints[i].state != BP_SET)
			continue;

		error = arch_set_breakpoint(ks, &breakpoints[i]);
		if (error) {
			ret = error;
			fprint(2, "BP install failed: %lx", breakpoints[i].bpt_addr);
			continue;
		}

		breakpoints[i].state = BP_ACTIVE;
	}
	return ret;
}

char *
dbg_set_sw_break(struct state *s, unsigned long addr)
{
	int breakno = -1;
	int i;

	for (i = 0; i < MAX_BREAKPOINTS; i++) {
		if ((breakpoints[i].state == BP_SET) &&
			(breakpoints[i].bpt_addr == addr))
			return "Already set";
	}
	for (i = 0; i < MAX_BREAKPOINTS; i++) {
		if (breakpoints[i].state == BP_REMOVED &&
			breakpoints[i].bpt_addr == addr) {
			breakno = i;
			break;
		}
	}

	if (breakno == -1) {
		for (i = 0; i < MAX_BREAKPOINTS; i++) {
			if (breakpoints[i].state == BP_UNDEFINED) {
				breakno = i;
				break;
			}
		}
	}

	if (breakno == -1)
		return "no more breakpoints";

	breakpoints[breakno].state = BP_SET;
	breakpoints[breakno].type = BP_BREAKPOINT;
	breakpoints[breakno].bpt_addr = addr;

	return nil;
}

char *
dbg_deactivate_sw_breakpoints(struct state *s)
{
	char *error;
	char *ret = 0;
	int i;

	for (i = 0; i < MAX_BREAKPOINTS; i++) {
		if (breakpoints[i].state != BP_ACTIVE)
			continue;
		error = arch_remove_breakpoint(s, &breakpoints[i]);
		if (error) {
			fprint(2, "KGDB: BP remove failed: %lx\n", breakpoints[i].bpt_addr);
			ret = error;
		}

		//flush_swbreak_addr(breakpoints[i].bpt_addr);
		breakpoints[i].state = BP_SET;
	}
	return ret;
}

char *
dbg_remove_sw_break(struct state *s, unsigned long addr)
{
	char *error;
	int i;

	for (i = 0; i < MAX_BREAKPOINTS; i++) {
		if (breakpoints[i].bpt_addr != addr) {
			continue;
		}

		if (breakpoints[i].state == BP_SET) {
			breakpoints[i].state = BP_REMOVED;
			return nil;
		} else if (breakpoints[i].state == BP_ACTIVE) {
			error = arch_remove_breakpoint(s, &breakpoints[i]);
			if (error) {
				fprint(2, "dbg_remove_sw_break failed: %lx\n", breakpoints[i].bpt_addr);
				return error;
			}

			breakpoints[i].state = BP_REMOVED;
			return nil;
		}
	}
	return "no such breakpoint";
}

int
isremovedbreak(unsigned long addr)
{
	int i;

	for (i = 0; i < MAX_BREAKPOINTS; i++) {
		if ((breakpoints[i].state == BP_REMOVED) &&
			(breakpoints[i].bpt_addr == addr))
			return 1;
	}
	return 0;
}

char *
dbg_remove_all_break(struct state *s)
{
	char *error;
	int i;

	/* Clear memory breakpoints. */
	for (i = 0; i < MAX_BREAKPOINTS; i++) {
		if (breakpoints[i].state != BP_ACTIVE)
			goto setundefined;
		error = arch_remove_breakpoint(s, &breakpoints[i]);
		if (error)
			fprint(2, "KGDB: breakpoint remove failed: %lx\n",
				   breakpoints[i].bpt_addr);
setundefined:
		breakpoints[i].state = BP_UNDEFINED;
	}

	return 0;
}
