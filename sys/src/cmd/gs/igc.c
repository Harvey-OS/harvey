/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* igc.c */
/* Garbage collector for Ghostscript */
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "gsexit.h"
#include "gsstruct.h"
#include "gsutil.h"
#include "iastate.h"
#include "isave.h"
#include "isstate.h"
#include "idict.h"
#include "ipacked.h"
#include "istruct.h"
#include "igc.h"
#include "iname.h"
#include "dstack.h"			/* for dsbot, dsp, dict_set_top */
#include "estack.h"			/* for esbot, esp */
#include "ostack.h"			/* for osbot, osp */
#include "opdef.h"			/* for marking oparray names */
#include "store.h"			/* for make_array */

/* Import the debugging variables from gsmemory.c. */
extern byte gs_alloc_fill_collected;

/* Import the static interpreter refs from interp.c. */
extern ref ref_static_stacks;
extern ref ref_ref_stacks[3];

/* Import cleanup routines for names and dictionaries. */
extern void name_gc_cleanup(P1(gc_state_t *));
extern void dstack_gc_cleanup(P0());

/* Define an entry on the mark stack. */
typedef struct { void *ptr; uint index; bool is_refs; } ms_entry;

/* Forward references */
private void gs_vmreclaim(P2(gs_dual_memory_t *, bool));
private void gc_top_level(P2(gs_dual_memory_t *, bool));
private void gc_objects_clear_marks(P1(chunk_t *));
private void gc_unmark_names(P0());
private int gc_trace(P4(gs_gc_root_t *, gc_state_t *, ms_entry *, uint));
private int gc_trace_chunk(P4(chunk_t *, gc_state_t *, ms_entry *, uint));
private bool gc_trace_finish(P1(gc_state_t *));
private void gc_clear_reloc(P1(chunk_t *));
private void gc_objects_set_reloc(P1(chunk_t *));
private void gc_do_reloc(P3(chunk_t *, gs_ref_memory_t *, gc_state_t *));
private void gc_objects_compact(P2(chunk_t *, gc_state_t *));
private void gc_free_empty_chunks(P1(gs_ref_memory_t *));
#ifdef DEBUG
private void gc_validate_chunk(P2(const chunk_t *, gc_state_t *));
#endif

/* Forward references for pointer types */
private ptr_proc_unmark(ptr_struct_unmark);
private ptr_proc_mark(ptr_struct_mark);
private ptr_proc_unmark(ptr_string_unmark);
private ptr_proc_mark(ptr_string_mark);
/*ptr_proc_unmark(ptr_ref_unmark);*/	/* in igc.h */
/*ptr_proc_mark(ptr_ref_mark);*/	/* in igc.h */
/*ptr_proc_reloc(gs_reloc_struct_ptr, void);*/	/* in gsstruct.h */
/*ptr_proc_reloc(gs_reloc_ref_ptr, ref_packed);*/	/* in istruct.h */

/* Pointer type descriptors. */
/* Note that the trace/mark routine has special knowledge of ptr_ref_type */
/* and ptr_struct_type -- it assumes that no other types have embedded */
/* pointers.  Note also that the reloc procedures for string and ref */
/* pointers are never called. */
typedef ptr_proc_reloc((*ptr_proc_reloc_t), void);
const gs_ptr_procs_t ptr_struct_procs =
 { ptr_struct_unmark, ptr_struct_mark, (ptr_proc_reloc_t)gs_reloc_struct_ptr };
const gs_ptr_procs_t ptr_string_procs =
 { ptr_string_unmark, ptr_string_mark, NULL };
const gs_ptr_procs_t ptr_const_string_procs =
 { ptr_string_unmark, ptr_string_mark, NULL };
const gs_ptr_procs_t ptr_ref_procs =
 { ptr_ref_unmark, ptr_ref_mark, NULL };

/* ------ Main program ------ */

/* Initialize the GC hook in the allocator. */
private int ireclaim(P2(gs_dual_memory_t *, int));
private void
igc_init(void)
{	gs_imemory.reclaim = ireclaim;
}

/* GC hook called when the allocator returns a VMerror (which = -1), */
/* or for vmreclaim (which = 0 for local, 1 for global). */
private int
ireclaim(gs_dual_memory_t *dmem, int which)
{	bool global;
	gs_ref_memory_t *mem;
	if ( which < 0 )
	  {	/* Determine which allocator got the VMerror. */
		gs_memory_status_t stats;
		global = dmem->space_global->gc_status.requested > 0;
		mem = (global ? dmem->space_global : dmem->space_local);
		gs_memory_status((gs_memory_t *)mem, &stats);
		if ( stats.allocated >= mem->gc_status.max_vm )
		  {	/* We can't satisfy this request within max_vm. */
			return_error(e_VMerror);
		  }
	  }
	else
	  {	global = which > 0;
		mem = (global ? dmem->space_global : dmem->space_local);
	  }
/****************/
global = true;
/****************/
	gs_vmreclaim(dmem, global);
	ialloc_set_limit(mem);
	ialloc_reset_requested(dmem);
	return 0;
}

/* Interpreter entry to garbage collector. */
/* This registers the stacks before calling the main GC. */
private void near set_ref_chunk(P4(chunk_t *, ref *, ref *, gs_ref_memory_t *));
private void
gs_vmreclaim(gs_dual_memory_t *dmem, bool global)
{	/*
	 * Create pseudo-chunks to hold the interpreter roots:
	 * copies of the ref_stacks, and, if necessary,
	 * the statically allocated stack bodies.
	 */
	gs_ref_memory_t *mem = dmem->space_local;
	gs_ref_memory_t *gmem = dmem->space_global;
	gs_ref_memory_t *smem = dmem->space_system;
	struct ir_ {
		chunk_head_t head;
		obj_header_t prefix;
		ref refs[5+1];		/* +1 for extra relocation ref */
	} iroot_refs;
	chunk_t cir, css;
	void *piroot = &iroot_refs.refs[0];
	gs_gc_root_t iroot;

	alloc_close_chunk(mem);
	if ( gmem != mem )
	  alloc_close_chunk(gmem);
	alloc_close_chunk(smem);

	/* Copy the ref_stacks into the heap, so we can trace and */
	/* relocate them. */
#define get_stack(i, stk)\
  ref_stack_cleanup(&stk);\
  iroot_refs.refs[i+2] = ref_ref_stacks[i],\
  *r_ptr(&iroot_refs.refs[i+2], ref_stack) = stk
	get_stack(0, d_stack);
	get_stack(1, e_stack);
	get_stack(2, o_stack);
#undef get_stack

	/* Make the root chunk. */
	iroot_refs.refs[1] = ref_static_stacks;
	make_array(&iroot_refs.refs[0], avm_system, 4, &iroot_refs.refs[1]);
	set_ref_chunk(&cir, &iroot_refs.refs[0], &iroot_refs.refs[5], mem);
	gs_register_ref_root((gs_memory_t *)mem, &iroot, &piroot, "gs_gc_main");

	/* If necessary, make the static stack chunk. */
#define css_array iroot_refs.refs[1]
#define css_base css_array.value.refs
	if ( css_base != NULL )
	  set_ref_chunk(&css, css_base, css_base + r_size(&css_array), mem);

	gc_top_level(dmem, global);

	/* Remove the temporary chunks. */
	if ( css_base != NULL )
	  alloc_unlink_chunk(&css, mem);
	gs_unregister_root((gs_memory_t *)mem, &iroot, "gs_gc_main");
	alloc_unlink_chunk(&cir, mem);
#undef css_array
#undef css_base

	/* Update the static copies of the ref_stacks. */
#define put_stack(i, stk)\
  ref_ref_stacks[i].value.pstruct = iroot_refs.refs[i+2].value.pstruct,\
  stk = *r_ptr(&iroot_refs.refs[i+2], ref_stack)
	put_stack(0, d_stack);
	put_stack(1, e_stack);
	put_stack(2, o_stack);
#undef put_stack

	/* Update the cached value pointers in names. */

	dstack_gc_cleanup();

	/* Reopen the active chunks. */

	alloc_open_chunk(smem);
	if ( gmem != mem )
	  alloc_open_chunk(gmem);
	alloc_open_chunk(mem);

	/* Update caches */

	{ uint dcount = ref_stack_count(&d_stack);
	  ref_systemdict = *ref_stack_index(&d_stack, dcount - 1);
	}
	dict_set_top();
}
private void near
set_ref_chunk(chunk_t *cp, ref *bot, ref *top, gs_ref_memory_t *mem)
{	obj_header_t *pre = (obj_header_t *)bot - 1;
	chunk_head_t *head = (chunk_head_t *)pre - 1;
	pre->o_large = 1;		/* not relocatable */
	pre->o_lsize = 0;
	pre->o_lmark = o_l_unmarked;
	pre->o_size = (byte *)(top + 1) - (byte *)bot;
	pre->o_type = &st_refs;
	alloc_init_chunk(cp, (byte *)head, (byte *)(top + 1), false, NULL);	/* +1 for extra reloc ref */
	cp->cbot = cp->ctop;
	alloc_link_chunk(cp, mem);
	make_int(top, 0);		/* relocation ref */
}

/* Top level of garbage collector. */
#ifdef DEBUG
private void
end_phase(const char _ds *str)
{	if ( gs_debug_c('6') )
	  {	dprintf1("[6]---------------- end %s ----------------\n",
			 (const char *)str);
		fflush(dstderr);
	  }
}
#else
#  define end_phase(str) DO_NOTHING
#endif
private void
gc_top_level(gs_dual_memory_t *dmem, bool global)
{
#define nspaces 3
	gs_ref_memory_t *spaces[nspaces];
	gs_gc_root_t space_roots[nspaces];
	int ntrace, ncollect, ispace;
	gs_ref_memory_t *mem;
	chunk_t *cp;
	gs_gc_root_t *rp;
	gc_state_t state;
	ms_entry default_mark_stack[100];
	ms_entry *mark_stack = default_mark_stack;
	uint ms_size = countof(default_mark_stack);

	/* Determine how many spaces we are collecting. */
	
	spaces[0] = dmem->space_local;
	spaces[1] = dmem->space_system;
	spaces[2] = dmem->space_global;
	if ( dmem->space_global != dmem->space_local )
		ntrace = 3;
	else
		ntrace = 2;
	ncollect = (global ? ntrace : 1);

#define for_spaces(i, n)\
  for ( i = 0; i < n; i++ )
#define for_space_mems(i, mem)\
  for ( mem = spaces[i]; mem != 0; mem = &mem->saved->state )
#define for_mem_chunks(mem, cp)\
  for ( cp = (mem)->cfirst; cp != 0; cp = cp->cnext )
#define for_space_chunks(i, mem, cp)\
  for_space_mems(i, mem) for_mem_chunks(mem, cp)
#define for_chunks(n, mem, cp)\
  for_spaces(ispace, n) for_space_chunks(ispace, mem, cp)
#define for_roots(n, mem, rp)\
  for_spaces(ispace, n)\
    for ( mem = spaces[ispace], rp = mem->roots; rp != 0; rp = rp->next )

	/* Initialize the state. */
	state.loc.memory = spaces[0];	/* either one will do */
	state.loc.cp = 0;
	state.space_local = spaces[0];
	state.space_system = spaces[1];
	state.space_global = spaces[2];

	/* Register the allocators themselves as roots, */
	/* so we mark and relocate the change and save lists properly. */

	for_spaces(ispace, ntrace)
	  gs_register_struct_root((gs_memory_t *)spaces[ispace],
				  &space_roots[ispace],
				  (void **)&spaces[ispace],
				  "gc_top_level");

	end_phase("register space roots");

#ifdef DEBUG

	/* Pre-validate the state.  This shouldn't be necessary.... */

	for_chunks(ntrace, mem, cp)
	  gc_validate_chunk(cp, &state);

	end_phase("pre-validate pointers");

#endif

	/* Clear marks in spaces to be collected; set them, */
	/* and clear relocation, in spaces that are only being traced. */

	for_chunks(ncollect, mem, cp)
	  {	gc_objects_clear_marks(cp);
		gc_strings_set_marks(cp, false);
	  }
	for ( ispace = ncollect; ispace < ntrace; ispace++ )
	  for_space_chunks(ispace, mem, cp)
		gc_clear_reloc(cp);

	end_phase("clear chunk marks");

	/* Clear the marks of roots.  We must do this explicitly, */
	/* since some roots are not in any chunk. */

	for_roots(ntrace, mem, rp)
	  {	void *vptr = *rp->p;
		if_debug_root('6', "[6]unmarking root", rp);
		(*rp->ptype->unmark)(vptr, &state);
	  }

	end_phase("clear root marks");

	gc_unmark_names();

	/* Find the largest available block to use as the mark stack. */

	for_chunks(ntrace, mem, cp)
	  {	uint avail = (cp->ctop - cp->cbot) / sizeof(ms_entry);
		if ( avail > ms_size  && !cp->inner_count )
		  mark_stack = (ms_entry *)cp->cbot,
		  ms_size = avail;
	  }

	/* Mark from roots. */

	{	int more = 0;
		for_roots(ntrace, mem, rp)
		{	if_debug_root('6', "[6]marking root", rp);
			more |= gc_trace(rp, &state, mark_stack, ms_size);
		}

		end_phase("mark");

		while ( more < 0 )		/* stack overflowed */
		  {	more = 0;
			for_chunks(ntrace, mem, cp)
			  more |= gc_trace_chunk(cp, &state, mark_stack,
						 ms_size);
		  }

		end_phase("mark overflow");
	}

	gc_trace_finish(&state);

	end_phase("finish trace");

	/* Set the relocation of roots outside any chunk to o_untraced, */
	/* so we won't try to relocate pointers to them. */
	/* (Currently, this is only the allocators.) */

	for_spaces(ispace, ntrace)
	  o_set_untraced(((obj_header_t *)spaces[ispace] - 1));

	/* Compute relocation based on marks, in the spaces */
	/* we are going to compact. */

	for_chunks(ncollect, mem, cp)
	{	gc_objects_set_reloc(cp);
		gc_strings_set_reloc(cp);
	}

	end_phase("set reloc");

	/* Remove unmarked names, and relocate name string pointers. */

	name_gc_cleanup(&state);

	end_phase("clean up names");

	/* Relocate pointers. */

	for_chunks(ntrace, mem, cp)
	  gc_do_reloc(cp, mem, &state);

	end_phase("relocate chunks");

	for_roots(ntrace, mem, rp)
	{	if_debug3('6', "[6]relocating root 0x%lx: 0x%lx -> 0x%lx\n",
			  (ulong)rp, (ulong)rp->p, (ulong)*rp->p);
		if ( rp->ptype == ptr_ref_type )
		{	ref *pref = (ref *)*rp->p;
			gs_reloc_refs((ref_packed *)pref,
				      (ref_packed *)(pref + 1),
				      &state);
		}
		else
			*rp->p = (*rp->ptype->reloc)(*rp->p, &state);
	}
	/* The allocators themselves aren't in any chunk, */
	/* so we have to relocate their references specially. */
	for_spaces(ispace, ntrace)
	  { mem = spaces[ispace];
	    (*st_ref_memory.reloc_ptrs)(mem, sizeof(gs_ref_memory_t), &state);
	  }

	end_phase("relocate roots");

	/* Compact data.  We only do this for spaces we are collecting. */

	for_spaces(ispace, ncollect)
	  { for_space_mems(ispace, mem)
	      { for_mem_chunks(mem, cp)
		  { if_debug_chunk('6', "[6]compacting chunk", cp);
		    gc_objects_compact(cp, &state);
		    gc_strings_compact(cp);
		    if_debug_chunk('6', "[6]after compaction:", cp);
		    if ( mem->pcc == cp )
		      mem->cc = *cp;
		  }
		mem->saved = mem->reloc_saved;
		ialloc_reset_free(mem);
	      }
	  }

	end_phase("compact");

	/* Free empty chunks. */

	for_spaces(ispace, ncollect)
	  for_space_mems(ispace, mem)
	    gc_free_empty_chunks(mem);

	end_phase("free empty chunks");

	/* Update previous_status.  We must do this by working back-to-front */
	/* along the save chain, using pointer reversal. */

	for_spaces(ispace, ncollect)
	  {	/* Reverse the pointers. */
		alloc_save_t *curr;
		alloc_save_t *prev = 0;
		alloc_save_t *next;
		gs_memory_status_t total;
		for ( curr = spaces[ispace]->saved; curr != 0;
		      prev = curr, curr = next
		    )
		  { next = curr->state.saved;
		    curr->state.saved = prev;
		  }
		/* Now work the other way, accumulating the values. */
		total.allocated = 0, total.used = 0;
		for ( curr = prev, prev = 0; curr != 0;
		      prev = curr, curr = next
		    )
		  { mem = &curr->state;
		    next = mem->saved;
		    mem->saved = prev;
		    mem->previous_status = total;
		    if_debug3('6',
			      "[6]0x%lx previous allocated=%lu, used=%lu\n",
			      (ulong)mem, total.allocated, total.used);
		    gs_memory_status((gs_memory_t *)mem, &total);
		    mem->gc_allocated = mem->allocated + total.allocated;
		  }
		mem = spaces[ispace];
		mem->previous_status = total;
		mem->gc_allocated = mem->allocated + total.allocated;
		if_debug3('6', "[6]0x%lx previous allocated=%lu, used=%lu\n",
			  (ulong)mem, total.allocated, total.used);
	  }

	end_phase("update stats");

	/* Clear marks in spaces we didn't compact. */

	for ( ispace = ncollect; ispace < ntrace; ispace++ )
	  for_space_chunks(ispace, mem, cp)
	    gc_objects_clear_marks(cp);

	end_phase("post-clear marks");

	/* Unregister the allocator roots. */

	for_spaces(ispace, ntrace)
	  gs_unregister_root((gs_memory_t *)spaces[ispace],
			     &space_roots[ispace], "gc_top_level");

	end_phase("unregister space roots");

#ifdef DEBUG

	/* Validate the state.  This shouldn't be necessary.... */

	for_chunks(ntrace, mem, cp)
	  gc_validate_chunk(cp, &state);

	end_phase("validate pointers");

#endif
}

/* ------ Debugging utilities ------ */

#ifdef DEBUG
/* Validate a pointer to an object header. */
private void
debug_check_object(const obj_header_t *pre, const chunk_t *cp,
  gc_state_t *gcst)
{	ulong size = pre_obj_contents_size(pre);
	gs_memory_type_ptr_t otype = pre->o_type;
	const char *oname;

	if ( !gs_debug_c('?') )
		return;			/* no check */
	if ( cp == 0 && gcst != 0 )
	{	gc_state_t st;
		st = *gcst;		/* no side effects! */
		if ( !gc_locate(pre, &st) )
		{	lprintf1("Object 0x%lx not in any chunk!\n",
				 (ulong)pre);
			/*gs_abort();*/
		}
		else
			cp = st.loc.cp;
	}
	if ( (cp != 0 &&
	      !(pre->o_large ? (byte *)pre == cp->cbase :
		size <= cp->ctop - (byte *)(pre + 1))) ||
	     size % otype->ssize != 0 ||
	     (oname = struct_type_name_string(otype),
	      *oname < 33 || *oname > 126)
	   )
	{	lprintf4("Bad object 0x%lx(%lu), ssize = %u, in chunk 0x%lx!\n",
			 (ulong)pre, (ulong)size, otype->ssize, (ulong)cp);
		gs_abort();
	}
}
#else
#  define debug_check_object(pre, cp, gcst) DO_NOTHING
#endif

/* ------ Unmarking phase ------ */

/* Unmark a single struct. */
private void
ptr_struct_unmark(void *vptr, gc_state_t *ignored)
{	if ( vptr != 0 )
	  o_set_unmarked(((obj_header_t *)vptr - 1));
}

/* Unmark a single string. */
private void
ptr_string_unmark(void *vptr, gc_state_t *gcst)
{	discard(gc_string_mark(((gs_string *)vptr)->data,
			       ((gs_string *)vptr)->size,
			       false, gcst));
}

/* Unmark the objects in a chunk. */
private void
gc_objects_clear_marks(chunk_t *cp)
{	if_debug_chunk('6', "[6]unmarking chunk", cp);
	SCAN_CHUNK_OBJECTS(cp)
	  DO_ALL
		struct_proc_clear_marks((*proc)) =
			pre->o_type->clear_marks;
		debug_check_object(pre, cp, NULL);
		if_debug3('7', " [7](un)marking %s(%lu) 0x%lx\n",
			  struct_type_name_string(pre->o_type),
			  (ulong)size, (ulong)pre);
		o_set_unmarked(pre);
		if ( proc != 0 )
			(*proc)(pre + 1, size);
	END_OBJECTS_SCAN
}

/* Mark 0- and 1-character names, and those referenced from the */
/* op_array_nx_table, and unmark all the rest. */
private void
gc_unmark_names()
{	uint count = name_count();
	register uint i;
	for ( i = 0; i < count; i++ )
	{	name *pname = name_index_ptr(i);
		if ( pname->string_size <= 1 )
		  pname->mark = 1;
		else
		  pname->mark = 0;
	}
	for ( i = 0; i < op_array_count; i++ )
	{	uint nidx = op_array_nx_table[i];
		name_index_ptr(nidx)->mark = 1;
	}
}

/* ------ Marking phase ------ */

/* Mark starting from all marked objects in a chunk. */
private int
gc_trace_chunk(chunk_t *cp, gc_state_t *pstate, ms_entry *mark_stack,
  uint ms_size)
{	gs_gc_root_t root;
	void *comp;
	int more = 0;
	root.p = &comp;
	if_debug_chunk('6', "[6]marking from chunk", cp);
	SCAN_CHUNK_OBJECTS(cp)
	  DO_ALL
		if_debug2('7', " [7]scanning/marking 0x%lx(%lu)\n",
			  (ulong)pre, (ulong)size);
		if ( pre->o_type == &st_refs )
		  {	ref_packed *rp = (ref_packed *)(pre + 1);
			char *end = (char *)rp + size;
			root.ptype = ptr_ref_type;
			while ( (char *)rp < end )
			{	comp = rp;
				if ( r_is_packed(rp) )
				  { if ( r_has_pmark(rp) )
				      { r_clear_pmark(rp);
					more |= gc_trace(&root, pstate,
							 mark_stack, ms_size);
				      }
				    rp++;
				  }
				else
				  { if ( r_has_attr((ref *)rp, l_mark) )
				      { r_clear_attrs((ref *)rp, l_mark);
					more |= gc_trace(&root, pstate,
							 mark_stack, ms_size);
				      }
				    rp += packed_per_ref;
				  }
			}
		  }
		else if ( !o_is_unmarked(pre) )
		  {	struct_proc_clear_marks((*proc)) =
			  pre->o_type->clear_marks;
			root.ptype = ptr_struct_type;
			comp = pre + 1;
			if ( !o_is_untraced(pre) )
			  o_set_unmarked(pre);
			if ( proc != 0 )
			  (*proc)(comp, size);
			more |= gc_trace(&root, pstate, mark_stack, ms_size);
		  }
	END_OBJECTS_SCAN
	return more;
}

/* Recursively mark from a (root) pointer. */
/* Return -1 if we overflowed the mark stack, */
/* 0 if we completed successfully without marking any new objects, */
/* 1 if we completed and marked some new objects. */
private int
gc_trace(gs_gc_root_t *rp, gc_state_t *pstate, ms_entry *mark_stack,
  uint ms_size)
{	ms_entry *sp = mark_stack;
	ms_entry *stop = mark_stack + ms_size - 1;
	int new = 0;
	void *nptr = *rp->p;
#define mark_name(i)\
  { name *pname = name_index_ptr(i);\
    if ( !pname->mark )\
     {  pname->mark = 1;\
	new |= 1;\
	if_debug2('8', "  [8]marked name 0x%lx(%u)\n", (ulong)pname, i);\
     }\
  }

	/* Initialize the stack */
	sp->ptr = nptr;
	if ( rp->ptype == ptr_ref_type )
		sp->index = 1, sp->is_refs = true;
	else
	{	sp->index = 0, sp->is_refs = false;
		if ( (*rp->ptype->mark)(nptr, pstate) )
		  new |= 1;
	}
	while ( sp >= mark_stack )
	{	gs_ptr_type_t ptp;
#ifdef DEBUG
		static const char *dots = "..........";
#define depth_dots\
  (sp >= &mark_stack[10] ? dots : dots + 10 - (sp - mark_stack))
#endif
		if ( !sp->is_refs )	/* struct */
		{	obj_header_t *ptr = sp->ptr;
			ulong osize = pre_obj_contents_size(ptr - 1);
			struct_proc_enum_ptrs((*mproc));
			debug_check_object(ptr - 1, NULL, NULL);
			if_debug4('7', " [7]%smarking %s 0x%lx[%u]",
				  depth_dots,
				  struct_type_name_string(ptr[-1].o_type),
				  (ulong)ptr, sp->index);
			mproc = ptr[-1].o_type->enum_ptrs;
			ptp = (mproc == 0 ? (gs_ptr_type_t)0 :
				(*mproc)(ptr, osize, sp->index++, &nptr));
			if ( ptp == NULL )	/* done with structure */
			{	if_debug0('7', " - done\n");
				sp--;
				continue;
			}
			if_debug1('7', " = 0x%lx\n", (ulong)nptr);
			sp[1].index = (ptp == ptr_ref_type ? 1 : 0);
		}
		else			/* refs */
		{	ref_packed *pptr = sp->ptr;
			if_debug3('8', "  [8]%smarking refs 0x%lx[%u]\n",
				  depth_dots, (ulong)pptr, sp->index - 1);
#define rptr ((ref *)pptr)
			if ( r_is_packed(rptr) )
			{	if ( !--(sp->index) ) sp--;
				else sp->ptr = pptr + 1;
				if ( r_has_pmark(pptr) )
				  continue;
				r_set_pmark(pptr);
				new |= 1;
				if ( r_packed_is_name(pptr) )
				{	uint nidx = packed_name_index(pptr);
					mark_name(nidx);
				}
				continue;
			}
			if ( !--(sp->index) ) sp--;
			else sp->ptr = rptr + 1;
			if ( r_has_attr(rptr, l_mark) )
			  continue;
			r_set_attrs(rptr, l_mark);
			new |= 1;
			switch ( r_type(rptr) )
			   {
			/* Struct cases */
			case t_file:
				nptr = rptr->value.pfile;
rs:				if ( r_is_foreign(rptr) )
				  continue;
				ptp = ptr_struct_type;
				sp[1].index = 0;
				break;
			case t_device:
				nptr = rptr->value.pdevice; goto rs;
			case t_fontID:
			case t_struct:
			case t_astruct:
				nptr = rptr->value.pstruct; goto rs;
			/* Non-trivial non-struct cases */
			case t_dictionary:
				nptr = rptr->value.pdict;
				sp[1].index = sizeof(dict) / sizeof(ref);
				goto rrp;
			case t_array:
				nptr = rptr->value.refs;
rr:				if ( (sp[1].index = r_size(rptr)) == 0 )
				{	/* Set the base pointer to 0, */
					/* so we never try to relocate it. */
					rptr->value.refs = 0;
					continue;
				}
rrp:				if ( r_is_foreign(rptr) )
					continue;
				if ( sp == stop )	/* stack overflow */
				{	new = -1;
					if_debug0('6', "[6]mark stack overflow\n");
					continue;
				}
				(++sp)->ptr = nptr;
				sp->is_refs = true;
				continue;
			case t_mixedarray: case t_shortarray:
				nptr = (void *)rptr->value.packed; /* discard const */
				goto rr;
			case t_name:
				mark_name(name_index(rptr));
				continue;
			case t_string:
				if ( r_is_foreign(rptr) )
				  continue;
				if ( gc_string_mark(rptr->value.bytes, r_size(rptr), true, pstate) )
				  new |= 1;
				continue;
			default:		/* includes packed refs */
				continue;
			   }
#undef rptr
		}
		/* Descend into nptr, whose pointer type is ptp. */
		if ( ptp == ptr_ref_type )
			sp[1].is_refs = 1;
		else
		{	if ( !(*ptp->mark)(nptr, pstate) )
			  continue;
			new |= 1;
			if ( ptp != ptr_struct_type )
			  {	/* We assume this is some non-pointer- */
				/* containing type. */
				continue;
			  }
			sp[1].is_refs = 0;
		}
		if ( sp == stop )
		{	new = -1;
			continue;
		}
		(++sp)->ptr = nptr;
		/* index and is_refs are already set */
	}
	return new;
}

/* Mark a struct.  Return true if new mark. */
private bool
ptr_struct_mark(void *vptr, gc_state_t *ignored)
{	obj_header_t *ptr = vptr;
	if ( vptr == 0 )
		return false;
	ptr--;			/* point to header */
	if ( !o_is_unmarked(ptr) )
		return false;
	o_mark(ptr);
	return true;
}

/* Mark a string.  Return true if new mark. */
private bool
ptr_string_mark(void *vptr, gc_state_t *gcst)
{	return gc_string_mark(((gs_string *)vptr)->data,
			      ((gs_string *)vptr)->size,
			      true, gcst);
}

/* Finish tracing by marking names. */
private bool
gc_trace_finish(gc_state_t *pstate)
{	uint count = name_count();
	bool marked = false;
	register uint c;
	for ( c = 0; c < count; c++ )
	{	uint i = name_count_to_index(c);
		name *pname = name_index_ptr(i);
		if ( pname->mark )
		{	if ( !pname->foreign_string && gc_string_mark(pname->string_bytes, pname->string_size, true, pstate) )
				marked = true;
			marked |= ptr_struct_mark(name_index_ptr_sub_table(i, pname), pstate);
		}
	}
	return marked;
}

/* ------ Relocation planning phase ------ */

/* Initialize the relocation information in the chunk header. */
private void
gc_init_reloc(chunk_t *cp)
{	chunk_head_t *chead = cp->chead;
	chead->dest = cp->cbase;
	chead->free.o_back =
	  offset_of(chunk_head_t, free) >> obj_back_shift;
	chead->free.o_size = sizeof(obj_header_t);
	chead->free.o_nreloc = 0;
}

/* Set marks and clear relocation for chunks that won't be compacted. */
private void
gc_clear_reloc(chunk_t *cp)
{	gc_init_reloc(cp);
	SCAN_CHUNK_OBJECTS(cp)
	  DO_ALL
		const struct_shared_procs_t _ds *procs =
			pre->o_type->shared;
		if ( procs != 0 )
		  (*procs->clear_reloc)(pre, size);
		o_set_untraced(pre);
	END_OBJECTS_SCAN
	gc_strings_set_marks(cp, true);
	gc_strings_clear_reloc(cp);
}

/* Set the relocation for the objects in a chunk. */
/* This will never be called for a chunk with any o_untraced objects. */
private void
gc_objects_set_reloc(chunk_t *cp)
{	uint reloc = 0;
	chunk_head_t *chead = cp->chead;
	byte *pfree = (byte *)&chead->free;	/* most recent free object */
	if_debug_chunk('6', "[6]setting reloc for chunk", cp);
	gc_init_reloc(cp);
	SCAN_CHUNK_OBJECTS(cp)
	  DO_ALL
		struct_proc_finalize((*finalize));
		const struct_shared_procs_t _ds *procs =
		  pre->o_type->shared;
		if ( (procs == 0 ? o_is_unmarked(pre) :
		      !(*procs->set_reloc)(pre, reloc, size))
		   )
		  {	/* Free object */
			reloc += sizeof(obj_header_t) + obj_align_round(size);
			if ( (finalize = pre->o_type->finalize) != 0 )
			  (*finalize)(pre + 1);
			if ( pre->o_large )
			  {	/* We should chop this up into small */
				/* free blocks, but there's no value */
				/* in doing this right now. */
				o_set_unmarked_large(pre);
			  }
			else
			  {	pfree = (byte *)pre;
				pre->o_back =
				  (pfree - (byte *)chead) >> obj_back_shift;
				pre->o_nreloc = reloc;
			  }
			if_debug3('7', " [7]at 0x%lx, unmarked %lu, new reloc = %u\n",
				  (ulong)pre, (ulong)size, reloc);
		  }
		else
		  {	/* Useful object */
			debug_check_object(pre, cp, NULL);
			if ( pre->o_large )
			  {	if ( o_is_unmarked_large(pre) )
				  o_mark_large(pre);
			  }
			else
			  pre->o_back =
			    ((byte *)pre - pfree) >> obj_back_shift;
		  }
	END_OBJECTS_SCAN
#ifdef DEBUG
	if ( reloc != 0 )
	{ if_debug1('6', "[6]freed %u", reloc);
	  if_debug_chunk('6', " in", cp);
	}
#endif
}

/* ------ Relocation phase ------ */

/* Relocate the pointers in all the objects in a chunk. */
private void
gc_do_reloc(chunk_t *cp, gs_ref_memory_t *mem, gc_state_t *pstate)
{	chunk_head_t *chead = cp->chead;
	if_debug_chunk('6', "[6]relocating in chunk", cp);
	SCAN_CHUNK_OBJECTS(cp)
	  DO_ALL
		/* We need to relocate the pointers in an object iff */
		/* it is o_untraced, or it is a useful object. */
		/* An object is free iff its back pointer points to */
		/* the chunk_head structure. */
		if ( o_is_untraced(pre) ||
		     (pre->o_large ? !o_is_unmarked(pre) :
		      pre->o_back << obj_back_shift !=
		        (byte *)pre - (byte *)chead)
		   )
		  {	struct_proc_reloc_ptrs((*proc)) =
				pre->o_type->reloc_ptrs;
			if_debug3('7',
				  " [7]relocating ptrs in %s(%lu) 0x%lx\n",
				  struct_type_name_string(pre->o_type),
				  (ulong)size, (ulong)pre);
			if ( proc != 0 )
				(*proc)(pre + 1, size, pstate);
		  }
	END_OBJECTS_SCAN
}

/* Print pointer relocation if debugging. */
/* We have to provide this procedure even if DEBUG is not defined, */
/* in case one of the other GC modules was compiled with DEBUG. */
void *
print_reloc_proc(void *obj, const char *cname, void *robj)
{	if_debug3('9', "  [9]relocate %s * 0x%lx to 0x%lx\n",
		  cname, (ulong)obj, (ulong)robj);
	return robj;
}

/* Relocate a pointer to an (aligned) object. */
void * /* obj_header_t * */
gs_reloc_struct_ptr(void * /* obj_header_t * */ obj, gc_state_t *gcst)
{	void *robj;
	if ( obj == 0 )
	  return print_reloc(obj, "NULL", 0);
#define optr ((obj_header_t *)obj)
	debug_check_object(optr - 1, NULL, gcst);
	/* The following should be a conditional expression, */
	/* but Sun's cc compiler can't handle it. */
	if ( optr[-1].o_large )
	  robj = obj;
	else
	  {	uint back = optr[-1].o_back;
		if ( back == o_untraced )
		  robj = obj;
		else
		  {	obj_header_t *pfree = (obj_header_t *)
			  ((char *)(optr - 1) - (back << obj_back_shift));
			chunk_head_t *chead = (chunk_head_t *)
			  ((char *)pfree - (pfree->o_back << obj_back_shift));
			robj = chead->dest +
			  ((char *)obj - (char *)(chead + 1) - pfree->o_nreloc);
#ifdef DEBUG
			/* Do some sanity checking. */
			if ( back > gcst->space_local->chunk_size >> obj_back_shift )
			  {	lprintf2("Invalid back pointer %u at 0x%lx!\n",
					 back, (ulong)obj);
				gs_abort();
			  }
#endif
		  }
	  }
	return print_reloc(obj,
			   struct_type_name_string(optr[-1].o_type),
			   robj);
#undef optr
}

/* ------ Compaction phase ------ */

/* Compact the objects in a chunk. */
/* This will never be called for a chunk with any o_untraced objects. */
private void
gc_objects_compact(chunk_t *cp, gc_state_t *gcst)
{	chunk_head_t *chead = cp->chead;
	obj_header_t *dpre = (obj_header_t *)chead->dest;
	SCAN_CHUNK_OBJECTS(cp)
	  DO_ALL
		/* An object is free iff its back pointer points to */
		/* the chunk_head structure. */
		if ( (pre->o_large ? !o_is_unmarked(pre) :
		      pre->o_back << obj_back_shift !=
		        (byte *)pre - (byte *)chead)
		   )
		  {	const struct_shared_procs_t _ds *procs =
			  pre->o_type->shared;
			debug_check_object(pre, cp, gcst);
			if_debug4('7',
				  " [7]compacting %s 0x%lx(%lu) to 0x%lx\n",
				  struct_type_name_string(pre->o_type),
				  (ulong)pre, (ulong)size, (ulong)dpre);
			if ( procs == 0 )
			  { if ( dpre != pre )
			      memmove(dpre, pre,
				      sizeof(obj_header_t) + size);
			  }
			else
			  (*procs->compact)(pre, dpre, size);
			dpre = (obj_header_t *)
			  ((byte *)dpre + obj_size_round(size));
		  }
	END_OBJECTS_SCAN
	if ( cp->outer == 0 && chead->dest != cp->cbase )
	  dpre = (obj_header_t *)cp->cbase; /* compacted this chunk into another */
	if ( gs_alloc_debug )
	  memset(dpre, gs_alloc_fill_collected, cp->cbot - (byte *)dpre);
	cp->cbot = (byte *)dpre;
	cp->rcur = 0;
	cp->rtop = 0;		/* just to be sure */
}

/* ------ Cleanup ------ */

/* Free empty chunks. */
private void
gc_free_empty_chunks(gs_ref_memory_t *mem)
{	chunk_t *cp;
	chunk_t *csucc;
	/* Free the chunks in reverse order, */
	/* to encourage LIFO behavior. */
	for ( cp = mem->clast; cp != 0; cp = csucc )
	{	/* Make sure this isn't an inner chunk, */
		/* or a chunk that has inner chunks. */
		csucc = cp->cprev; 	/* save before freeing */
		if ( cp->cbot == cp->cbase && cp->ctop == cp->climit &&
		     cp->outer == 0 && cp->inner_count == 0
		   )
		  {	alloc_free_chunk(cp, mem);
			if ( mem->pcc == cp )
			  mem->pcc = 0;
		  }
	}
}

#ifdef DEBUG

/* Validate all the objects in a chunk. */
private void
gc_validate_chunk(const chunk_t *cp, gc_state_t *gcst)
{	if_debug_chunk('6', "[6]validating chunk", cp);
	SCAN_CHUNK_OBJECTS(cp);
	  DO_ALL
		debug_check_object(pre, cp, gcst);
		if_debug3('7', " [7]validating %s(%lu) 0x%lx\n",
			  struct_type_name_string(pre->o_type),
			  (ulong)size, (ulong)pre);
#define opre(ptr) ((const obj_header_t *)(ptr) - 1)
		if ( pre->o_type == &st_refs )
		  {	const ref_packed *rp = (const ref_packed *)(pre + 1);
			const char *end = (char *)rp + size;
			while ( (const char *)rp < end )
			  if ( r_is_packed(rp) )
			    rp++;
			  else
			  { const void *optr = 0;
#define pref ((const ref *)rp)
			    if ( r_space(pref) != avm_foreign )
			      switch ( r_type(pref) )
			    {
			    case t_file:
				optr = opre(pref->value.pfile);
				goto cks;
			    case t_device:
				optr = opre(pref->value.pdevice);
				goto cks;
			    case t_fontID:
			    case t_struct:
			    case t_astruct:
				optr = opre(pref->value.pstruct);
cks:				if ( optr != 0 )
				  debug_check_object(optr, NULL, gcst);
				break;
			    case t_name:
				if ( name_index_ptr(r_size(pref)) !=
				     pref->value.pname
				   )
				  {	lprintf3("At 0x%lx, bad name %u, pname = 0x%lx\n",
						 (ulong)pref,
						 (uint)r_size(pref),
						 (ulong)pref->value.pname);
					break;
				  }
				if ( !pref->value.pname->foreign_string &&
				     !gc_locate(pref->value.pname->string_bytes, gcst)
				   )
				  {	lprintf4("At 0x%lx, bad name %u, pname = 0x%lx, string 0x%lx not in any chunk\n",
						 (ulong)pref,
						 (uint)r_size(pref),
						 (ulong)pref->value.pname,
						 (ulong)pref->value.pname->string_bytes);
					break;
				  }
				break;
			    case t_string:
				if ( r_size(pref) != 0 &&
				     !gc_locate(pref->value.bytes, gcst)
				   )
				  {	lprintf3("At 0x%lx, string ptr 0x%lx[%u] not in any chunk\n",
						 (ulong)pref,
						 (ulong)pref->value.bytes,
						 (uint)r_size(pref));
				  }
				break;
			    /****** SHOULD ALSO CHECK: ******/
			    /****** arrays, dict ******/
			    }
#undef pref
			    rp += packed_per_ref;
			  }
		  }
		else
		  {	struct_proc_enum_ptrs((*proc)) =
			  pre->o_type->enum_ptrs;
			uint index = 0;
			void *ptr;
			gs_ptr_type_t ptype;
			if ( proc != 0 )
			  for ( ; (ptype = (*proc)(pre + 1, size, index, &ptr)) != 0; ++index )
			    if ( ptr != 0 && ptype == ptr_struct_type )
			      debug_check_object(opre(ptr), NULL, gcst);
		  }
#undef opre
	END_OBJECTS_SCAN
}

#endif

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(igc_l2_op_defs) {
		op_def_begin_level2(),
END_OP_DEFS(igc_init) }

/* ---------------- Utilities ---------------- */

/* Locate a pointer in the chunks of either space being collected. */
/* This is only used for strings and for debugging. */
bool
gc_locate(const void *ptr, gc_state_t *gcst)
{	const gs_ref_memory_t *mem;
	if ( chunk_locate(ptr, &gcst->loc) )
	  return true;
	mem = gcst->loc.memory;
	/* Try the other space, if there is one. */
	if ( gcst->space_local != gcst->space_global )
	  { gcst->loc.memory =
	      (mem->space == avm_local ? gcst->space_global : gcst->space_local);
	    gcst->loc.cp = 0;
	    if ( chunk_locate(ptr, &gcst->loc) )
	      return true;
	    /* Try other save levels of this space. */
	    while ( gcst->loc.memory->saved != 0 )
	      { gcst->loc.memory = &gcst->loc.memory->saved->state;
		gcst->loc.cp = 0;
		if ( chunk_locate(ptr, &gcst->loc) )
		  return true;
	      }
	  }
	/* Try the system space.  This is simpler because it isn't */
	/* subject to save/restore. */
	if ( mem != gcst->space_system )
	{	gcst->loc.memory = gcst->space_system;
		gcst->loc.cp = 0;
		if ( chunk_locate(ptr, &gcst->loc) )
		  return true;
	}
	/* Try other save levels of the initial space, */
	/* or of global space if the original space was system space. */
	/* In the latter case, try all levels. */
	gcst->loc.memory =
	  (mem == gcst->space_system || mem->space == avm_global ?
	   gcst->space_global : gcst->space_local);
	for ( ; ; )
	  { if ( gcst->loc.memory != mem )	/* don't do twice */
	      { gcst->loc.cp = 0;
		if ( chunk_locate(ptr, &gcst->loc) )
		  return true;
	      }
	    if ( gcst->loc.memory->saved == 0 )
	      break;
	    gcst->loc.memory = &gcst->loc.memory->saved->state;
	  }
	/* Restore locator to a legal state. */
	gcst->loc.memory = mem;
	gcst->loc.cp = 0;
	return false;
}
