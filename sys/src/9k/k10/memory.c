/*
 * Size memory and create the kernel page-tables
 * on the fly while doing so.
 * Called from main(), this code should only be run
 * by the bootstrap processor.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

void
meminit(void)
{
	extern void asmmeminit(void);

	asmmeminit();
}

void
umeminit(void)
{
	extern void asmumeminit(void);

	asmumeminit();
}
