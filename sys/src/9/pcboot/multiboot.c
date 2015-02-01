/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

Mbi	mbhdr;
int	nmmap;

/* these need to end up in low memory */
Mbi	*multibootheader = &mbhdr;
MMap	mmap[32+1];

void
mkmultiboot(void)
{
	MMap *lmmap;

	/* reuse the bios table memory */
	multibootheader = (Mbi *)KADDR(BIOSTABLES);
	memset(multibootheader, 0, sizeof *multibootheader);

	lmmap = (MMap *)(multibootheader + 1);
	memmove(lmmap, mmap, sizeof mmap);

	multibootheader->cmdline = PADDR(BOOTLINE);
	multibootheader->flags |= Fcmdline;
	if(nmmap != 0){
		multibootheader->mmapaddr = PADDR(lmmap);
		multibootheader->mmaplength = nmmap*sizeof(MMap);
		multibootheader->flags |= Fmmap;
	}
	multibootheader = (Mbi *)PADDR(multibootheader);
	if(v_flag)
		print("PADDR(&multibootheader) %#p\n", multibootheader);
}
