/* Copyright (C) 1994, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: igc.h,v 1.9 2004/09/28 05:34:29 giles Exp $ */
/* Internal interfaces in Ghostscript GC */

#ifndef igc_INCLUDED
#  define igc_INCLUDED

#include "istruct.h"

/* Declare the vm_reclaim procedure for the real GC. */
extern vm_reclaim_proc(gs_gc_reclaim);

/* Define the procedures shared among a "genus" of structures. */
/* Currently there are only two genera: refs, and all other structures. */
struct struct_shared_procs_s {

    /* Clear the relocation information in an object. */

#define gc_proc_clear_reloc(proc)\
  void proc(obj_header_t *pre, uint size)
    gc_proc_clear_reloc((*clear_reloc));

    /* Compute any internal relocation for a marked object. */
    /* Return true if the object should be kept. */
    /* The reloc argument shouldn't be required, */
    /* but we need it for ref objects. */

#define gc_proc_set_reloc(proc)\
  bool proc(obj_header_t *pre, uint reloc, uint size)
    gc_proc_set_reloc((*set_reloc));

    /* Compact an object. */

#define gc_proc_compact(proc)\
  void proc(const gs_memory_t *cmem, obj_header_t *pre, obj_header_t *dpre, uint size)
    gc_proc_compact((*compact));

};

/* Define the structure for holding GC state. */
/*typedef struct gc_state_s gc_state_t; *//* in gsstruct.h */
#ifndef name_table_DEFINED
#  define name_table_DEFINED
typedef struct name_table_s name_table;
#endif
struct gc_state_s {
    const gc_procs_with_refs_t *procs;	/* must be first */
    chunk_locator_t loc;
    vm_spaces spaces;
    int min_collect;		/* avm_space */
    bool relocating_untraced;	/* if true, we're relocating */
    /* pointers from untraced spaces */
    gs_memory_t *heap;	/* for extending mark stack */
    name_table *ntable;		/* (implicitly referenced by names) */
#ifdef DEBUG
    chunk_t *container;
#endif
};

/* Exported by igcref.c for igc.c */
ptr_proc_unmark(ptr_ref_unmark);
ptr_proc_mark(ptr_ref_mark);
/*ref_packed *gs_reloc_ref_ptr(const ref_packed *, gc_state_t *); */

/* Exported by ilocate.c for igc.c */
void ialloc_validate_memory(const gs_ref_memory_t *, gc_state_t *);
void ialloc_validate_chunk(const chunk_t *, gc_state_t *);
void ialloc_validate_object(const obj_header_t *, const chunk_t *,
			    gc_state_t *);

/* Exported by igc.c for ilocate.c */
const gs_memory_t * gcst_get_memory_ptr(gc_state_t *gcst);

/* Macro for returning a relocated pointer */
const void *print_reloc_proc(const void *obj, const char *cname,
			     const void *robj);
#ifdef DEBUG
#  define print_reloc(obj, cname, nobj)\
	(gs_debug_c('9') ? print_reloc_proc(obj, cname, nobj) : nobj)
#else
#  define print_reloc(obj, cname, nobj) (nobj)
#endif

#endif /* igc_INCLUDED */
