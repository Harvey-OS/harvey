#include <u.h>
#include <libc.h>
#include <mach.h>
#include <bio.h>
#define Extern extern
#include "mips.h"

void
icacheinit(void)
{
}

void
updateicache(ulong addr)
{
	USED(addr);
}

