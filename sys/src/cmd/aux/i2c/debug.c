/* Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * See LICENSE for details.
 *
 * Arch-independent kernel debugging */

#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

static int tab_depth = 0;
int printx_on = 0;

int min(int a, int b)
{
	return a < b ? a : b;
}

void __print_func_entry(const char *func, const char *file, int line)
{
	char tentabs[] = "\t\t\t\t\t\t\t\t\t\t"; // ten tabs and a \0
	char *ourtabs = &tentabs[10 - min(tab_depth, 10)];
	if (!printx_on)
		return;
	fprint(2,"%s%s()@%d in %s\n", ourtabs, func, line, file);
	tab_depth++;
}

void __print_func_exit(const char *func, const char *file, int line)
{
	char tentabs[] = "\t\t\t\t\t\t\t\t\t\t"; // ten tabs and a \0
	char *ourtabs;
	if (!printx_on)
		return;
	tab_depth--;
	ourtabs = &tentabs[10 - min(tab_depth, 10)];
	fprint(2,"%s---- %s()@%d\n", ourtabs, func, line);
}

void set_printx(int mode)
{
	printx_on = mode;
}

