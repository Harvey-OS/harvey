/* Copyright (C) 1994, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: igc.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
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
  void proc(P2(obj_header_t *pre, uint size))
    gc_proc_clear_reloc((*clear_reloc));

    /* Compute any internal relocation for a marked object. */
    /* Return true if the object should be kept. */
    /* The reloc argument shouldn't be required, */
    /* but we need it for ref objects. */

#define gc_proc_set_reloc(proc)\
  bool proc(P3(obj_header_t *pre, uint reloc, uint size))
    gc_proc_set_reloc((*set_reloc));

    /* Compact an object. */

#define gc_proc_compact(proc)\
  void proc(P3(obj_header_t *pre, obj_header_t *dpre, uint size))
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
    gs_raw_memory_t *heap;	/* for extending mark stack */
    name_table *ntable;		/* (implicitly referenced by names) */
};

/* Exported by igcref.c for igc.c */
ptr_proc_unmark(ptr_ref_unmark);
ptr_proc_mark(ptr_ref_mark);
/*ref_packed *gs_reloc_ref_ptr(P2(const ref_packed *, gc_state_t *)); */

/* Exported by ilocate.c for igc.c */
void ialloc_validate_memory(P2(const gs_ref_memory_t *, gc_state_t *));
void ialloc_validate_chunk(P2(const chunk_t *, gc_state_t *));
void ialloc_validate_object(P3(const obj_header_t *, const chunk_t *,
			       gc_state_t *));

/* Macro for returning a relocated pointer */
const void *print_reloc_proc(P3(const void *obj, const char *cname,
				const void *robj));
#ifdef DEBUG
#  define print_reloc(obj, cname, nobj)\
	(gs_debug_c('9') ? print_reloc_proc(obj, cname, nobj) : nobj)
#else
#  define print_reloc(obj, cname, nobj) (nobj)
#endif

#endif /* igc_INCLUDED */
