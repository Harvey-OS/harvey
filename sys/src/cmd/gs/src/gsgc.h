/* Copyright (C) 1996, 1999 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*$Id: gsgc.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
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

/* Define an array of allocators indexed by space. */
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
  void proc(P2(vm_spaces *pspaces, bool global))
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
