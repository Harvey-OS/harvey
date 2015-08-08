/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

MMap mmap[20];
int nmmap;

Mbi multibootheader;

void
mkmultiboot(void)
{
	if(nmmap != 0) {
		multibootheader.cmdline = PADDR(BOOTLINE);
		multibootheader.flags |= Fcmdline;
		multibootheader.mmapaddr = PADDR(mmap);
		multibootheader.mmaplength = nmmap * sizeof(MMap);
		multibootheader.flags |= Fmmap;
		if(vflag)
			print("&multibootheader %#p\n", &multibootheader);
	}
}
