#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#include "arm.h"

void
icacheinit(void)
{
}

void
updateicache(ulong addr)
{
	USED(addr);
}

