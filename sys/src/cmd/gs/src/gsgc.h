/* Copyright (C) 1996, 1999, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsgc.h,v 1.6 2002/06/16 08:45:42 lpd Exp $ */
/* Library-level interface to garbage collector */

/*
 * This API is not strictly at the library level, since it references
 * gs_ref_memory_t and the 4 PostScript memory spaces; however, the former
 * concept already leaks into the library's standard allocator, and the
 * latter is relatively small and harmless.
 */

#ifndef gsgc_INCLUDED
#  define gsgc_INCLUDED

/*
 * Define the VM space numbers, in increasing order of dynamism.  Pointers
 * from a higher-numbered space to the same or a lower-numbered space are
 * always allowed, but not vice versa.  Foreign space (the most static) is
 * internal, the rest are visible to the programmer; the index of foreign
 * space must be 0, so that we don't have to set any space bits in scalar
 * refs (PostScript objects).
 */
typedef enum {
    i_vm_foreign = 0,		/* must be 0 */
    i_vm_system,
    i_vm_global,
    i_vm_local,
    i_vm_max = i_vm_local
} i_vm_space;

/*
 * Define an array of allocators indexed by space.  Note that the first
 * ("foreign") element of this array is always 0: foreign pointers, by
 * definition, point to objects that are not managed by a Ghostscript
 * allocator (typically, static const objects, or objects allocated with
 * malloc by some piece of code other than Ghostscript).
 */
#ifndef gs_ref_memory_DEFINED
#  define gs_ref_memory_DEFINED
typedef struct gs_ref_memory_s gs_ref_memory_t;
#endif
/*
 * r_space_bits is only defined in PostScript interpreters, but if it is
 * defined, we want to make sure it's 2.
 */
#ifdef r_space_bits
#  if r_space_bits != 2
Error_r_space_bits_is_not_2;
#  endif
#endif
typedef struct vm_spaces_s vm_spaces;
/*
 * The garbage collection procedure is named vm_reclaim so as not to
 * collide with the reclaim member of gs_dual_memory_t.
 */
#define vm_reclaim_proc(proc)\
  void proc(vm_spaces *pspaces, bool global)
struct vm_spaces_s {
    vm_reclaim_proc((*vm_reclaim));
    union {
	gs_ref_memory_t *indexed[4 /*1 << r_space_bits */ ];
	struct _ssn {
	    gs_ref_memory_t *foreign;
	    gs_ref_memory_t *system;
	    gs_ref_memory_t *global;
	    gs_ref_memory_t *local;
	} named;
    } memories;
};

/*
 * By convention, the vm_spaces member of structures, and local variables
 * of type vm_spaces, are named spaces.
 */
#define space_foreign spaces.memories.named.foreign
#define space_system spaces.memories.named.system
#define space_global spaces.memories.named.global
#define space_local spaces.memories.named.local
#define spaces_indexed spaces.memories.indexed

/*
 * Define the top-level entry to the garbage collectors.
 */
#define GS_RECLAIM(pspaces, global) ((pspaces)->vm_reclaim(pspaces, global))
/* Backward compatibility */
#define gs_reclaim(pspaces, global) GS_RECLAIM(pspaces, global)

#endif /* gsgc_INCLUDED */
