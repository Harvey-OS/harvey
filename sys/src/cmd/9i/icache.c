#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mach.h>
#define Extern extern
#include "sim.h"

void
icacheinit(void)
{
}

void
updateicache(ulong addr)
{
	USED(addr);
}

