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
