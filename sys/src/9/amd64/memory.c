/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "amd64.h"

void
meminit(void)
{
	int cx = 0;

	for(PAMap *m = pamap; m != nil; m = m->next){
		DBG("meminit: addr %#P end %#P type %d size %P\n",
		    m->addr, m->addr + m->size,
		    m->type, m->size);
		PTE pgattrs = PteP;
		switch(m->type){
		default:
			DBG("(Skipping)\n");
			continue;
		case PamKTEXT:
			pgattrs |= PteG;
			break;
		case PamDEV:
			pgattrs |= PtePCD;
		case PamMEMORY:
		case PamKRDWR:
			pgattrs |= PteRW;
		case PamACPI:
		case PamPRESERVE:
		case PamRESERVED:
		case PamKRDONLY:
			pgattrs |= PteNX;
		}
		mmukphysmap(UINT2PTR(machp()->MMU.pml4->va), m->addr, pgattrs, m->size);

		/*
		 * Fill in conf data.
		 */
		if(m->type != PamMEMORY)
			continue;
		if(cx >= nelem(conf.mem))
			continue;
		uintmem lo = ROUNDUP(m->addr, PGSZ);
		conf.mem[cx].base = lo;
		uintmem hi = ROUNDDN(m->addr + m->size, PGSZ);
		conf.mem[cx].npage = (hi - lo) / PGSZ;
		conf.npage += conf.mem[cx].npage;
		DBG("cm %d: addr %#llx npage %lu\n",
		    cx, conf.mem[cx].base, conf.mem[cx].npage);
		cx++;
	}
	mmukflushtlb();

	/*
	 * Fill in more legacy conf data.
	 * This is why I hate Plan 9.
	 */
	conf.upages = conf.npage;
	conf.ialloc = 64 * MiB;	       // Arbitrary.
	DBG("npage %llu upage %lu\n", conf.npage, conf.upages);
}

static void
setphysmembounds(void)
{
	uintmem pmstart, pmend;

	pmstart = ROUNDUP(PADDR(end), 2 * MiB);
	pmend = pmstart;
	for(PAMap *m = pamap; m != nil; m = m->next){
		if(m->type == PamMODULE && m->addr + m->size > pmstart)
			pmstart = ROUNDUP(m->addr + m->size, 2 * MiB);
		if(m->type == PamMEMORY && m->addr + m->size > pmend)
			pmend = ROUNDDN(m->addr + m->size, 2 * MiB);
	}
	sys->pmstart = pmstart;
	sys->pmend = pmend;
}

void
umeminit(void)
{
	extern void physallocdump(void);
	setphysmembounds();

	for(PAMap *m = pamap; m != nil; m = m->next){
		if(m->type != PamMEMORY)
			continue;
		if(m->addr < 2 * MiB)
			continue;
		// if(m->size < 2*MiB)
		// 	continue;
		// usize offset=ROUNDUP(m->addr, 2*MiB)-m->addr;
		// m->size -= offset;
		// m->addr += offset;
		// if (m->size == 0)
		// 	continue;
		physinit(m->addr, m->size);
	}
	physallocdump();
}
