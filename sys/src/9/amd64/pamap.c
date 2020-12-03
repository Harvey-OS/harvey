/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

// Physical Address Map.
//
// Describes the layout of physical memory, including the
// kernel segments, MMIO regions, multiboot modules, etc.
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "amd64.h"

PAMap *pamap = nil;

void
pamapinit(void)
{
	// Nop.
}

const char *
pamtypename(int type)
{
	const char *names[] = {
	[PamNONE] 	= "NONE",
	[PamMEMORY]	= "MEMORY",
	[PamRESERVED]	= "RESERVED",
	[PamACPI]	= "ACPI",
	[PamPRESERVE]	= "PRESERVE",
	[PamUNUSABLE]	= "UNUSABLE",
	[PamDEV]	= "DEV",
	[PamMODULE]	= "MODULE",
	[PamKTEXT]	= "KTEXT",
	[PamKRDONLY]	= "KRDONLY",
	[PamKRDWR]	= "KRDWR",
	};
	assert(type < nelem(names));
	return names[type];
}

void
pamapdump(void)
{
	print("pamap: {\n");
	for(PAMap *p = pamap; p != nil; p = p->next){
		assert(p->type <= PamKRDWR);
		print("    [%#P, %#P) %-8s (%llu)\n",
			p->addr, p->addr + p->size,
			pamtypename(p->type), p->size);
	}
	print("}\n");
}

PAMap *
pamapnew(uintmem addr, usize size, int type)
{
	PAMap *m = malloc(sizeof(*m));
	assert(m != nil);
	memset(m, 0, sizeof(*m));
	m->addr = addr;
	m->size = size;
	m->type = type;
	m->next = nil;

	return m;
}

static void
pamapclearrange(uintmem addr, usize size, int type)
{
	PAMap **ppp = &pamap, *np = pamap;
	while(np != nil && size > 0){
		if(addr+size <= np->addr)
			break;		// The range isn't in the list.

		// Are we there yet?
		if(np->addr < addr && np->addr+np->size <= addr){
			ppp = &np->next;
			np = np->next;
			continue;
		}

		// We found overlap.
		//
		// If the left-side overlaps, adjust the current
		// node to end at the overlap, and insert a new
		// node at the overlap point.  It may be immediately
		// deleted, but that's ok.
		//
		// If the right side overlaps, adjust size and
		// delta accordingly.
		//
		// In both cases, we're trying to get the overlap
		// to start at the same place.
		//
		// If the ranges overlap and start at the same
		// place, adjust the current node and remove it if
		// it becomes empty.
		if(np->addr < addr){
			assert(addr < np->addr + np->size);
			uintmem osize = np->size;
			np->size = addr - np->addr;
			PAMap *tp = pamapnew(addr, osize - np->size, np->type);
			tp->next = np->next;
			np->next = tp;
			ppp = &np->next;
			np = tp;
		}else if(addr < np->addr){
			assert(np->addr < addr + size);
			usize delta = np->addr - addr;
			addr += delta;
			size -= delta;
		}
		if(addr == np->addr){
			usize delta = size;
			if (delta > np->size)
				delta = np->size;
			np->size -= delta;
			np->addr += delta;
			addr += delta;
			size -= delta;
		}

		// If the resulting range is empty, remove it.
		if(np->size == 0){
			PAMap *tmp = np->next;
			*ppp = tmp;
			free(np);
			np = tmp;
			continue;
		}
		ppp = &np->next;
		np = np->next;
	}
}

void
pamapinsert(uintmem addr, usize size, int type)
{
	PAMap *np, *pp, **ppp;

	assert(type <= PamKRDWR);

	// Ignore empty regions.
	if(size == 0)
		return;

	// If the list is empty, just add the entry and return.
	if(pamap == nil){
		pamap = pamapnew(addr, size, type);
		return;
	}

	// Remove this region from any existing regions.
	pamapclearrange(addr, size, type);

	// Find either a map entry with an address greater
	// than that being returned, or the end of the map.
	ppp = &pamap;
	np = pamap;
	pp = nil;
	while(np != nil && np->addr <= addr){
		ppp = &np->next;
		pp = np;
		np = np->next;
	}

	// See if we can combine with previous region.
	if(pp != nil && pp->type == type && pp->addr+pp->size == addr){
		pp->size += size;

		// And successor region?  If we do it here,
		// we free the successor node.
		if(np != nil && np->type == type && addr+size == np->addr){
			pp->size += np->size;
			pp->next = np->next;
			free(np);
		}

		return;
	}

	// Can we combine with the successor region?
	if(np != nil && np->type == type && addr+size == np->addr){
		np->addr = addr;
		np->size += size;
		return;
	}

	// Insert a new node.
	pp = pamapnew(addr, size, type);
	*ppp = pp;
	pp->next = np;
}

void
pamapmerge(void)
{
	// Extended BIOS Data Area
	pamapinsert(0x80000, 0xA0000-0x80000, PamKRDWR);

	// VGA/CGA MMIO region
	pamapinsert(0xA0000, 0xC0000-0xA0000, PamDEV);

	// BIOS ROM stuff
	pamapinsert(0xC0000, 0xF0000-0xC0000, PamKRDONLY);
	pamapinsert(0xF0000, 0x100000-0xF0000, PamKRDONLY);

	// Add the kernel segments.
	pamapinsert(PADDR((void*)KSYS), KTZERO-KSYS, PamKRDWR);
	pamapinsert(PADDR((void*)KTZERO), etext-(char*)KTZERO, PamKTEXT);
	pamapinsert(PADDR(etext), erodata-etext, PamKRDONLY);
	pamapinsert(PADDR(erodata), edata-erodata, PamKRDWR);
	pamapinsert(PADDR(edata), end-edata, PamKRDWR);
}
