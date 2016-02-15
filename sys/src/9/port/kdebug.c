/* Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * See LICENSE for details.
 *
 * Arch-independent kernel debugging */

#include <u.h>
#include <lib.h>
#include <mem.h>
#include <dat.h>
#include <fns.h>
#include <error.h>

int printx_on = 1;
static int tab_depth = 0;

static void __iprint_hdr(void)
{
	Proc *up = externup();
//	struct per_cpu_info *pcpui = &per_cpu_info[core_id()];
	iprint("Core %2d ", 0); //core_id());	/* may help with multicore output */
	if (! islo()) {
		iprint("IRQ       :");
	} else if (up) {
		iprint("%d: ", up->pid);
	}
}

void __print_func_entry(const char *func, const char *file)
{
	char tentabs[] = "\t\t\t\t\t\t\t\t\t\t"; // ten tabs and a \0
	char *ourtabs = &tentabs[10 - MIN(tab_depth, 10)];
	if (!printx_on)
		return;
//	if (is_blacklisted(func))
//		return;
	__iprint_hdr();
	iprint("%s%s() in %s\n", ourtabs, func, file);
	tab_depth++;
}

void __print_func_exit(const char *func, const char *file)
{
	char tentabs[] = "\t\t\t\t\t\t\t\t\t\t"; // ten tabs and a \0
	char *ourtabs;
	if (!printx_on)
		return;
//	if (is_blacklisted(func))
//		return;
	tab_depth--;
	ourtabs = &tentabs[10 - MIN(tab_depth, 10)];
	__iprint_hdr();
	iprint("%s---- %s()\n", ourtabs, func);
}

void set_printx(int mode)
{
	switch (mode) {
		case 0:
			printx_on = 0;
			break;
		case 1:
			printx_on = 1;
			break;
		case 2:
			printx_on = !printx_on;
			break;
	}
}

