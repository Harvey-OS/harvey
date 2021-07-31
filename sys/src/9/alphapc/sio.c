#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

/*
 * 82378ZB Saturn-I/O (SIO) PCI-to-ISA Bus Bridge.
 */
static Pcidev* siodev;

void
siodump(void)
{
	if(siodev == nil && (siodev = pcimatch(nil, 0x8086, 0x0484)) == nil)
		return;
}
