/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* igc.h */
/* Internal interfaces in Ghostscript GC */

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
/*typedef struct gc_state_s gc_state_t;*/	/* in gsstruct.h */
struct gc_state_s {
	chunk_locator_t loc;
	vm_spaces spaces;
};

/* Exported by igc.c for igcstr.c */
bool gc_locate(P2(const void *, gc_state_t *));

/* Exported by igcref.c for igc.c */
void gs_mark_refs(P3(ref_packed *, ref *, bool));
void ptr_ref_unmark(P2(void *, gc_state_t *));
bool ptr_ref_mark(P2(void *, gc_state_t *));
ref_packed *gs_reloc_ref_ptr(P2(ref_packed *, gc_state_t *));

/* Exported by igcstr.c for igc.c */
void gc_strings_set_marks(P2(chunk_t *, bool));
bool gc_string_mark(P4(const byte *, uint, bool, gc_state_t *));
void gc_strings_clear_reloc(P1(chunk_t *));
void gc_strings_set_reloc(P1(chunk_t *));
void gc_strings_compact(P1(chunk_t *));

/* Macro for returning a relocated pointer */
#ifdef DEBUG
void *print_reloc_proc(P3(void *obj, const char *cname, void *robj));
#  define print_reloc(obj, cname, nobj)\
	(gs_debug_c('9') ? print_reloc_proc(obj, cname, nobj) :\
	 (void *)(nobj))
#else
#  define print_reloc(obj, cname, nobj)\
	(void *)(nobj)
#endif
