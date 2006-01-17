/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gsnogc.c,v 1.10 2002/06/16 05:48:55 lpd Exp $ */
/* String freelist implementation and ersatz garbage collector */
#include "gx.h"
#include "gsmdebug.h"
#include "gsnogc.h"
#include "gsstruct.h"
#include "gxalloc.h"

/*
 * We implement string freelists here, because they are only useful
 * in non-garbage-collected environments.
 */

/*
 * Get and put unaligned 32-bit values.  NOTE: these procedures must match
 * the value of SFREE_NB defined in gxalloc.h.
 */
#define NB SFREE_NB
#if NB == 4
private uint
get_uu32(const byte *ptr)
{
    return (ptr[0] << 24) + (ptr[1] << 16) + (ptr[2] << 8) + ptr[3];
}
private void
put_uu32(byte *ptr, uint val)
{
    ptr[0] = val >> 24;
    ptr[1] = (byte)(val >> 16);
    ptr[2] = (byte)(val >> 8);
    ptr[3] = (byte)val;
}
#endif /* otherwise, undefined procedures will give an error */

/* Allocate a string. */
/* Scan the current chunk's free list if the request is large enough. */
/* Currently we require an exact match of the block size. */
private byte *
sf_alloc_string(gs_memory_t * mem, uint nbytes, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;

    if (nbytes >= 40 && nbytes < imem->large_size) {
	byte *base = csbase(&imem->cc);
	byte *prev = 0;
	uint offset = imem->cc.sfree;
	uint next;
	byte *ptr;

	for (; offset != 0; prev = ptr, offset = next) {
	    ptr = base + offset;
	    next = get_uu32(ptr + NB);
	    if (get_uu32(ptr) != nbytes)
		continue;
	    /* Take this block. */
	    if (prev == 0)
		imem->cc.sfree = next;
	    else
		put_uu32(prev + NB, next);
	    if_debug4('A', "[a%d:+>F]%s(%u) = 0x%lx\n", imem->space,
		      client_name_string(cname), nbytes, (ulong) ptr);
	    gs_alloc_fill(ptr, gs_alloc_fill_alloc, nbytes);
	    imem->lost.strings -= nbytes;
	    return ptr;
	}
    }
    return (*gs_ref_memory_procs.alloc_string) (mem, nbytes, cname);
}

/* Free a string. */
private void
sf_free_string(gs_memory_t * mem, byte * str, uint size, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    chunk_t *cp;
    uint str_offset;

    if (str == imem->cc.ctop) {
	if_debug4('A', "[a%d:-> ]%s(%u) 0x%lx\n", imem->space,
		  client_name_string(cname), size, (ulong) str);
	imem->cc.ctop += size;
	gs_alloc_fill(str, gs_alloc_fill_free, size);
	return;
    }
    if_debug4('A', "[a%d:->#]%s(%u) 0x%lx\n", imem->space,
	      client_name_string(cname), size, (ulong) str);
    imem->lost.strings += size;
    if (ptr_is_in_chunk(str, &imem->cc)) {
	cp = &imem->cc;
	/* We already tested for the string being at ctop. */
    } else {
	chunk_locator_t loc;

	loc.memory = imem;
	loc.cp = imem->clast;
	if (!chunk_locate_ptr(str, &loc))
	    return;		/* something is probably wrong.... */
	cp = loc.cp;
	if (str == cp->ctop) {
	    cp->ctop += size;
	    return;
	}
    }
    str_offset = str - csbase(cp);
    if (size >= 2 * NB) {
	byte *prev;
	uint next;

	put_uu32(str, size);
	if (cp->sfree == 0 || str_offset < cp->sfree) {
	    /* Put the string at the head of the free list. */
	    put_uu32(str + NB, cp->sfree);
	    cp->sfree = str_offset;
	    return;
	}
	/* Insert the new string in address order. */
	prev = csbase(cp) + cp->sfree;
#ifdef DEBUG
	if (gs_debug_c('?')) {
	    if (prev < str + size && prev + get_uu32(prev) > str) {
		lprintf4("freeing string 0x%lx(%u), overlaps 0x%lx(%u)!\n",
			 (ulong) str, size, (ulong) prev, get_uu32(prev));
		return;
	    }
	}
#endif
	for (;;) {
	    next = get_uu32(prev + NB);
#ifdef DEBUG
	    if (gs_debug_c('?') && next != 0) {
		byte *pnext = csbase(cp) + next;

		if (pnext < str + size && pnext + get_uu32(pnext) > str) {
		    lprintf4("freeing string 0x%lx(%u), overlaps 0x%lx(%u)!\n",
			     (ulong) str, size, (ulong) pnext, get_uu32(pnext));
		    return;
		}
	    }
#endif
	    if (next >= str_offset || next == 0)
		break;
	    prev = csbase(cp) + next;
	}
	put_uu32(str + NB, next);
	put_uu32(prev + NB, str_offset);
	gs_alloc_fill(str + 2 * NB, gs_alloc_fill_free, size - 2 * NB);
    } else {
	/*
	 * Insert the string in the 1-byte free list(s).  Note that
	 * if it straddles a 256-byte block, we need to do this twice.
	 */
	uint *pfree1 = &cp->sfree1[str_offset >> 8];
	uint count = size;
	byte *prev;
	byte *ptr;

	if (*pfree1 == 0) {
	    *str = 0;
	    *pfree1 = str_offset;
	    if (!--count)
		return;
	    prev = str;
	    ptr = str + 1;
	} else if (str_offset < *pfree1) {
	    *str = *pfree1 - str_offset;
	    *pfree1 = str_offset;
	    if (!--count)
		return;
	    prev = str;
	    ptr = str + 1;
	} else {
	    uint next;

	    prev = csbase(cp) + *pfree1;
	    while ((next = *prev) != 0 && prev + next < str)
		prev += next;
	    ptr = str;
	}
	for (;;) {
	    /*
	     * Invariants:
	     *      ptr == str + size - count
	     *      prev < sfbase + str_offset
	     *      *prev == 0 || prev + *prev > sfbase + str_offset
	     */
	    *ptr = (*prev == 0 ? 0 : prev + *prev - ptr);
	    *prev = ptr - prev;
	    if (!--count)
		break;
	    prev = ptr++;
	    if (!(++str_offset & 255)) {
		/* Move to the next block of 256 bytes. */
		++pfree1;
		*ptr = (byte) * pfree1;
		*pfree1 = str_offset;
		if (!--count)
		    break;
		prev = ptr++;
		++str_offset;
	    }
	}
    }
}

/* Enable or disable freeing. */
private void
sf_enable_free(gs_memory_t * mem, bool enable)
{
    (*gs_ref_memory_procs.enable_free) (mem, enable);
    if (enable)
	mem->procs.free_string = sf_free_string;
}

/* Merge free strings at the bottom of a chunk's string storage. */
private void
sf_merge_strings(chunk_t * cp)
{
    for (;;) {
	byte *ctop = cp->ctop;
	uint top_offset = ctop - csbase(cp);
	uint *pfree1;

	if (cp->sfree == top_offset) {	/* Merge a large free block. */
	    cp->sfree = get_uu32(ctop + NB);
	    cp->ctop += get_uu32(ctop);
	    continue;
	}
	if (!cp->sfree1)
	    break;
	pfree1 = &cp->sfree1[top_offset >> 8];
	if (*pfree1 != top_offset)
	    break;
	/* Merge a small (1-byte) free block. */
	*pfree1 = (*ctop ? *pfree1 + *ctop : 0);
	cp->ctop++;
    }
}

/* Consolidate free space. */
private void
sf_consolidate_free(gs_memory_t *mem)
{
    gs_ref_memory_t *imem = (gs_ref_memory_t *)mem;
    chunk_t *cp;

    alloc_close_chunk(imem);
    for (cp = imem->clast; cp != 0; cp = cp->cprev) {
	byte *top = cp->ctop;

	sf_merge_strings(cp);
	imem->lost.strings -= cp->ctop - top;
        if (cp->ctop == cp->climit && cp->smark_size != 0) {
	    /*
	     * No string space is being used.  Since we are not using the
	     * 'string marking table' for GC, we can recover its space by
	     * deleting the smark area, setting smark_size = 0, and sliding
	     * up ctop and climit.  We also need to recompute the size of
	     * the string freelist area (it will be larger, since the
	     * space potentially allocated to strings is larger).
	     */
	    cp->smark_size = 0;
	    cp->smark = 0;
	    /*
	     * Reserve enough space for the string freelist all the way to
	     * cend even though the climit will be moved to before the
	     * freelist area.  This recovers most of the space.
	     */
	    cp->climit = cp->cend;
	    cp->climit -= STRING_FREELIST_SPACE(cp);
	    cp->ctop = cp->climit;
	    alloc_init_free_strings(cp);
	}
    }
    alloc_open_chunk(imem);

    /* Merge free objects, detecting entirely free chunks. */
    ialloc_consolidate_free(imem);
}

/*
 * This procedure has the same API as the garbage collector used by the
 * PostScript interpreter, but it is designed to be used in environments
 * that don't need garbage collection and don't use save/restore.  All it
 * does is coalesce free blocks at the high end of the object area of each
 * chunk, and free strings at the low end of the string area, and then if
 * not 'controlled' memory, free completely empty chunks.
 *
 * Note that any string marking area will be added to the free space
 * within the chunk if possible.
 */

private void use_string_freelists(gs_ref_memory_t *mem);
void
gs_nogc_reclaim(vm_spaces * pspaces, bool global)
{
    int space;
    gs_ref_memory_t *mem_prev = 0;

    for (space = 0; space < countof(pspaces->memories.indexed); ++space) {
	gs_ref_memory_t *mem = pspaces->memories.indexed[space];

	if (mem == 0 || mem == mem_prev)
	    continue;
	mem_prev = mem;
	use_string_freelists(mem);
	if (mem->stable_memory != (gs_memory_t *)mem &&
	    mem->stable_memory != 0
	    )
	    use_string_freelists((gs_ref_memory_t *)mem->stable_memory);
    }
}
#ifdef VMS
#pragma optimize ansi_alias=off
#endif
private void
use_string_freelists(gs_ref_memory_t *rmem)
{
    /*
     * ANSI made an incompatible change to the C language standard that
     * caused the following to generate aliasing warnings:
	gs_memory_t *mem = (gs_memory_t *)rmem;
     * Consequently, we now use rmem rather than mem in the assignments
     * below, even though this degrades code readability by obscuring the
     * fact that they are only manipulating fields of the more abstract
     * superclass.
     * For OpenVMS this still gets us a warning so we switch the warning
     * message off.
     */
#ifdef VMS
#pragma message disable BADANSIALIAS
#endif

    /* Change the allocator to use string freelists in the future.  */
    rmem->procs.alloc_string = sf_alloc_string;
    if (rmem->procs.free_string != gs_ignore_free_string)
	rmem->procs.free_string = sf_free_string;
    rmem->procs.enable_free = sf_enable_free;
    rmem->procs.consolidate_free = sf_consolidate_free;

    /* Merge free objects, detecting entirely free chunks. */
    gs_consolidate_free((gs_memory_t *)rmem);
}
