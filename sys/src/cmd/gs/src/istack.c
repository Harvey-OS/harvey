/* Copyright (C) 1992, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: istack.c,v 1.6 2003/09/03 03:22:59 giles Exp $ */
/* Manager for expandable stacks of refs */
#include "memory_.h"
#include "ghost.h"
#include "gsstruct.h"
#include "gsutil.h"
#include "ierrors.h"
#include "ialloc.h"
#include "istack.h"
#include "istkparm.h"
#include "istruct.h"		/* for RELOC_REF_VAR */
#include "iutil.h"
#include "ivmspace.h"		/* for local/global test */
#include "store.h"

/* Forward references */
private void init_block(ref_stack_t *pstack, const ref *pblock_array,
			uint used);
private int ref_stack_push_block(ref_stack_t *pstack, uint keep, uint add);

/* GC descriptors and procedures */
private_st_ref_stack_params();
private 
CLEAR_MARKS_PROC(ref_stack_clear_marks)
{
    ref_stack_t *const sptr = vptr;

    r_clear_attrs(&sptr->current, l_mark);
}
private 
ENUM_PTRS_WITH(ref_stack_enum_ptrs, ref_stack_t *sptr) return 0;
case 0: ENUM_RETURN_REF(&sptr->current);
case 1: return ENUM_OBJ(sptr->params);
ENUM_PTRS_END
private RELOC_PTRS_WITH(ref_stack_reloc_ptrs, ref_stack_t *sptr)
{
    /* Note that the relocation must be a multiple of sizeof(ref_packed) */
    /* * align_packed_per_ref, but it need not be a multiple of */
    /* sizeof(ref).  Therefore, we must do the adjustments using */
    /* ref_packed pointers rather than ref pointers. */
    ref_packed *bot = (ref_packed *) sptr->current.value.refs;
    long reloc;

    RELOC_REF_VAR(sptr->current);
    r_clear_attrs(&sptr->current, l_mark);
    reloc = bot - (ref_packed *) sptr->current.value.refs;
#define RELOC_P(p)\
  sptr->p = (ref *)((ref_packed *)sptr->p - reloc);
    RELOC_P(p);
    RELOC_P(bot);
    RELOC_P(top);
#undef RELOC_P
    RELOC_OBJ_VAR(sptr->params);
} RELOC_PTRS_END
/* Structure type for a ref_stack. */
public_st_ref_stack();

/* Initialize a stack. */
int
ref_stack_init(ref_stack_t *pstack, const ref *pblock_array,
	       uint bot_guard, uint top_guard, const ref *pguard_value,
	       gs_ref_memory_t *mem, ref_stack_params_t *params)
{
    uint size = r_size(pblock_array);
    uint avail = size - (stack_block_refs + bot_guard + top_guard);
    ref_stack_block *pblock = (ref_stack_block *)pblock_array->value.refs;
    s_ptr body = (s_ptr)(pblock + 1);

    if (params == 0) {
	params = gs_alloc_struct((gs_memory_t *)mem, ref_stack_params_t,
				 &st_ref_stack_params,
				 "ref_stack_alloc(stack.params)");
	if (params == 0)
	    return_error(-1);	/* avoid binding in any error codes */
    }

    pstack->bot = body + bot_guard;
    pstack->p = pstack->bot - 1;
    pstack->top = pstack->p + avail;
    pstack->current = *pblock_array;
    pstack->extension_size = 0;
    pstack->extension_used = 0;

    make_int(&pstack->max_stack, avail);
    pstack->requested = 0;
    pstack->margin = 0;
    pstack->body_size = avail;

    pstack->params = params;
    pstack->memory = mem;

    params->bot_guard = bot_guard;
    params->top_guard = top_guard;
    params->block_size = size;
    params->data_size = avail;
    if (pguard_value != 0)
	params->guard_value = *pguard_value;
    else
	make_tav(&params->guard_value, t__invalid, 0, intval, 0);
    params->underflow_error = -1;
    params->overflow_error = -1;
    params->allow_expansion = true;
    init_block(pstack, pblock_array, 0);
    refset_null_new(pstack->bot, avail, 0);
    make_empty_array(&pblock->next, 0);
    return 0;
}

/* Set whether a stack is allowed to expand.  The initial value is true. */
void
ref_stack_allow_expansion(ref_stack_t *pstack, bool expand)
{
    pstack->params->allow_expansion = expand;
}

/* Set the error codes for under- and overflow.  The initial values are -1. */
void
ref_stack_set_error_codes(ref_stack_t *pstack, int underflow_error,
			  int overflow_error)
{
    pstack->params->underflow_error = underflow_error;
    pstack->params->overflow_error = overflow_error;
}

/* Set the maximum number of elements allowed on a stack. */
int
ref_stack_set_max_count(ref_stack_t *pstack, long nmax)
{
    uint nmin = ref_stack_count_inline(pstack);

    if (nmax < nmin)
	nmax = nmin;
    if (nmax > max_uint / sizeof(ref))
	nmax = max_uint / sizeof(ref);
    if (!pstack->params->allow_expansion) {
	uint ncur = pstack->body_size;

	if (nmax > ncur)
	    nmax = ncur;
    }
    pstack->max_stack.value.intval = nmax;
    return 0;
}

/*
 * Set the margin between the limit and the top of the stack.
 * Note that this may require allocating a block.
 */
int
ref_stack_set_margin(ref_stack_t *pstack, uint margin)
{
    const ref_stack_params_t *params = pstack->params;
    uint data_size = params->data_size;

    if (margin <= pstack->margin) {
	refset_null_new(pstack->top + 1, pstack->margin - margin, 0);
    } else {
	if (margin > data_size >> 1)
	    return_error(e_rangecheck);
	if (pstack->top - pstack->p < margin) {
	    uint used = pstack->p + 1 - pstack->bot;
	    uint keep = data_size - margin;
	    int code = ref_stack_push_block(pstack, keep, used - keep);

	    if (code < 0)
		return code;
	}
    }
    pstack->margin = margin;
    pstack->body_size = data_size - margin;
    pstack->top = pstack->bot + pstack->body_size - 1;
    return 0;
}

/* Return the number of elements on a stack. */
uint
ref_stack_count(const ref_stack_t *pstack)
{
    return ref_stack_count_inline(pstack);
}

/*
 * Return a pointer to a given element from the stack, counting from
 * 0 as the top element.  If the index is out of range, return 0.
 */
ref *
ref_stack_index(const ref_stack_t *pstack, long idx)
{
    ref_stack_block *pblock;
    uint used = pstack->p + 1 - pstack->bot;

    if (idx < 0)
	return NULL;
    if (idx < used)		/* common case */
	return pstack->p - (uint) idx;
    pblock = (ref_stack_block *) pstack->current.value.refs;
    do {
	pblock = (ref_stack_block *) pblock->next.value.refs;
	if (pblock == 0)
	    return NULL;
	idx -= used;
	used = r_size(&pblock->used);
    } while (idx >= used);
    return pblock->used.value.refs + (used - 1 - (uint) idx);
}

/*
 * Count the number of elements down to and including the first mark.
 * If no mark is found, return 0.
 */
uint
ref_stack_counttomark(const ref_stack_t *pstack)
{
    uint scanned = 0;
    ref_stack_enum_t rsenum;

    ref_stack_enum_begin(&rsenum, pstack);
    do {
	uint count = rsenum.size;
	const ref *p = rsenum.ptr + count - 1;

	for (; count; count--, p--)
	    if (r_has_type(p, t_mark))
		return scanned + (rsenum.size - count + 1);
	scanned += rsenum.size;
    } while (ref_stack_enum_next(&rsenum));
    return 0;
}

/*
 * Do the store check for storing 'count' elements of a stack, starting
 * 'skip' elements below the top, into an array.  Return 0 or e_invalidaccess.
 */
int
ref_stack_store_check(const ref_stack_t *pstack, ref *parray, uint count,
		      uint skip)
{
    uint space = r_space(parray);

    if (space != avm_local) {
	uint left = count, pass = skip;
	ref_stack_enum_t rsenum;

	ref_stack_enum_begin(&rsenum, pstack);
	do {
	    ref *ptr = rsenum.ptr;
	    uint size = rsenum.size;

	    if (size <= pass)
		pass -= size;
	    else {
		int code;

		if (pass != 0)
		    size -= pass, pass = 0;
		ptr += size;
		if (size > left)
		    size = left;
		left -= size;
		code = refs_check_space(ptr - size, size, space);
		if (code < 0)
		    return code;
		if (left == 0)
		    break;
	    }
	} while (ref_stack_enum_next(&rsenum));
    }
    return 0;
}

/*
 * Store the top 'count' elements of a stack, starting 'skip' elements below
 * the top, into an array, with or without store/undo checking.  age=-1 for
 * no check, 0 for old, 1 for new.  May return e_rangecheck or
 * e_invalidaccess.
 */
#undef idmemory			/****** NOTA BENE ******/
int
ref_stack_store(const ref_stack_t *pstack, ref *parray, uint count,
		uint skip, int age, bool check, gs_dual_memory_t *idmemory,
		client_name_t cname)
{
    uint left, pass;
    ref *to;
    ref_stack_enum_t rsenum;

    if (count > ref_stack_count(pstack) || count > r_size(parray))
	return_error(e_rangecheck);
    if (check) {
	int code = ref_stack_store_check(pstack, parray, count, skip);

	if (code < 0)
	    return code;
    }
    to = parray->value.refs + count;
    left = count, pass = skip;
    ref_stack_enum_begin(&rsenum, pstack);
    do {
	ref *from = rsenum.ptr;
	uint size = rsenum.size;

	if (size <= pass)
	    pass -= size;
	else {
	    if (pass != 0)
		size -= pass, pass = 0;
	    from += size;
	    if (size > left)
		size = left;
	    left -= size;
	    switch (age) {
	    case -1:		/* not an array */
		while (size--) {
		    from--, to--;
		    ref_assign(to, from);
		}
		break;
	    case 0:		/* old array */
		while (size--) {
		    from--, to--;
		    ref_assign_old(parray, to, from, cname);
		}
		break;
	    case 1:		/* new array */
		while (size--) {
		    from--, to--;
		    ref_assign_new(to, from);
		}
		break;
	    }
	    if (left == 0)
		break;
	}
    } while (ref_stack_enum_next(&rsenum));
    r_set_size(parray, count);
    return 0;
}

/*
 * Pop the top N elements off a stack.
 * The number must not exceed the number of elements in use.
 */
void
ref_stack_pop(ref_stack_t *pstack, uint count)
{
    uint used;

    while ((used = pstack->p + 1 - pstack->bot) < count) {
	count -= used;
	pstack->p = pstack->bot - 1;
	ref_stack_pop_block(pstack);
    }
    pstack->p -= count;
}

/* Pop the top block off a stack.  May return underflow_error. */
int
ref_stack_pop_block(ref_stack_t *pstack)
{
    s_ptr bot = pstack->bot;
    uint count = pstack->p + 1 - bot;
    ref_stack_block *pcur =
    (ref_stack_block *) pstack->current.value.refs;
    ref_stack_block *pnext =
    (ref_stack_block *) pcur->next.value.refs;
    uint used;
    ref *body;
    ref next;

    if (pnext == 0)
	return_error(pstack->params->underflow_error);
    used = r_size(&pnext->used);
    body = (ref *) (pnext + 1) + pstack->params->bot_guard;
    next = pcur->next;
    /*
       * If the contents of the two blocks won't fit in a single block, we
       * move up the used part of the top block, and copy up as much of
       * the contents of the next block under it as will fit.  If the
       * contents of both blocks fit in a single block, we copy the used
       * part of the top block to the top of the next block, and free the
       * top block.
     */
    if (used + count > pstack->body_size) {
	/*
	 * The contents of the two blocks won't fit into a single block.
	 * On the assumption that we're recovering from a local stack
	 * underflow and need to increase the number of contiguous
	 * elements available, move up the used part of the top block, and
	 * copy up as much of the contents of the next block under it as
	 * will fit.
	 */
	uint moved = pstack->body_size - count;
	uint left;

	if (moved == 0)
	    return_error(e_Fatal);
	memmove(bot + moved, bot, count * sizeof(ref));
	left = used - moved;
	memcpy(bot, body + left, moved * sizeof(ref));
	refset_null_new(body + left, moved, 0);
	r_dec_size(&pnext->used, moved);
	pstack->p = pstack->top;
	pstack->extension_used -= moved;
    } else {
	/*
	   * The contents of the two blocks will fit into a single block.
	   * Copy the used part of the top block to the top of the next
	   * block, and free the top block.
	 */
	memcpy(body + used, bot, count * sizeof(ref));
	pstack->bot = bot = body;
	pstack->top = bot + pstack->body_size - 1;
	gs_free_ref_array(pstack->memory, &pstack->current,
			  "ref_stack_pop_block");
	pstack->current = next;
	pstack->p = bot + (used + count - 1);
	pstack->extension_size -= pstack->body_size;
	pstack->extension_used -= used;
    }
    return 0;
}

/*
 * Extend a stack to recover from an overflow condition.
 * May return overflow_error or e_VMerror.
 */
int
ref_stack_extend(ref_stack_t *pstack, uint request)
{
    uint keep = (pstack->top - pstack->bot + 1) / 3;
    uint count = pstack->p - pstack->bot + 1;
    const ref_stack_params_t *params = pstack->params;

    if (request > params->data_size)
	return_error(params->overflow_error);
    if (keep + request > pstack->body_size)
	keep = pstack->body_size - request;
    if (keep > count)
	keep = count;		/* required by ref_stack_push_block */
    return ref_stack_push_block(pstack, keep, request);
}

/*
 * Push N empty slots onto a stack.  These slots are not initialized:
 * the caller must immediately fill them.  May return overflow_error
 * (if max_stack would be exceeded, or the stack has no allocator)
 * or e_VMerror.
 */
int
ref_stack_push(ref_stack_t *pstack, uint count)
{
    /* Don't bother to pre-check for overflow: we must be able to */
    /* back out in the case of a VMerror anyway, and */
    /* ref_stack_push_block will make the check itself. */
    uint needed = count;
    uint added;

    for (; (added = pstack->top - pstack->p) < needed; needed -= added) {
	int code;

	pstack->p = pstack->top;
	code = ref_stack_push_block(pstack,
				    (pstack->top - pstack->bot + 1) / 3,
				    added);
	if (code < 0) {
	    /* Back out. */
	    ref_stack_pop(pstack, count - needed + added);
	    pstack->requested = count;
	    return code;
	}
    }
    pstack->p += needed;
    return 0;
}

/*
 * Push a block onto the stack, specifying how many elements of the current
 * top block should remain in the top block and also how many elements we
 * are trying to add.  Requires keep <= count.  May return overflow_error or
 * e_VMerror.
 */
private int
ref_stack_push_block(ref_stack_t *pstack, uint keep, uint add)
{
    const ref_stack_params_t *params = pstack->params;
    uint count = pstack->p - pstack->bot + 1;
    uint move = count - keep;
    ref_stack_block *pcur = (ref_stack_block *) pstack->current.value.refs;
    ref next;
    ref_stack_block *pnext;
    ref *body;
    int code;

    if (keep > count)
	return_error(e_Fatal);
    /* Check for overflowing the maximum size, */
    /* or expansion not allowed.  */
    if (pstack->extension_used + (pstack->top - pstack->bot) + add >=
	pstack->max_stack.value.intval ||
	!params->allow_expansion
	)
	return_error(params->overflow_error);
    code = gs_alloc_ref_array(pstack->memory, &next, 0,
			      params->block_size, "ref_stack_push_block");
    if (code < 0)
	return code;
    pnext = (ref_stack_block *) next.value.refs;
    body = (ref *) (pnext + 1);
    /* Copy the top keep elements into the new block, */
    /* and make the new block the top block. */
    init_block(pstack, &next, keep);
    body += params->bot_guard;
    memcpy(body, pstack->bot + move, keep * sizeof(ref));
    /* Clear the elements above the top of the new block. */
    refset_null_new(body + keep, params->data_size - keep, 0);
    /* Clear the elements above the top of the old block. */
    refset_null_new(pstack->bot + move, keep, 0);
    pnext->next = pstack->current;
    pcur->used.value.refs = pstack->bot;
    r_set_size(&pcur->used, move);
    pstack->current = next;
    pstack->bot = body;
    pstack->top = pstack->bot + pstack->body_size - 1;
    pstack->p = pstack->bot + keep - 1;
    pstack->extension_size += pstack->body_size;
    pstack->extension_used += move;
    return 0;
}

/* Begin enumerating the blocks of a stack. */
void
ref_stack_enum_begin(ref_stack_enum_t *prse, const ref_stack_t *pstack)
{
    prse->block = (ref_stack_block *)pstack->current.value.refs;
    prse->ptr = pstack->bot;
    prse->size = pstack->p + 1 - pstack->bot;
}

bool
ref_stack_enum_next(ref_stack_enum_t *prse)
{
    ref_stack_block *block =
	prse->block = (ref_stack_block *)prse->block->next.value.refs;

    if (block == 0)
	return false;
    prse->ptr = block->used.value.refs;
    prse->size = r_size(&block->used);
    return true;
}

/* Clean up a stack for garbage collection. */
void
ref_stack_cleanup(ref_stack_t *pstack)
{
    ref_stack_block *pblock =
	(ref_stack_block *) pstack->current.value.refs;

    refset_null_new(pstack->p + 1, pstack->top - pstack->p, 0);
    pblock->used = pstack->current;	/* set attrs */
    pblock->used.value.refs = pstack->bot;
    r_set_size(&pblock->used, pstack->p + 1 - pstack->bot);
}

/*
 * Free the entire contents of a stack, including the bottom block.
 * The client must still call ref_stack_free.  Note that after calling
 * ref_stack_release, the stack is no longer usable.
 */
void
ref_stack_release(ref_stack_t *pstack)
{
    gs_ref_memory_t *mem = pstack->memory;

    ref_stack_clear(pstack);
    /* Free the parameter structure. */
    gs_free_object((gs_memory_t *)mem, pstack->params,
		   "ref_stack_release(stack.params)");
    /* Free the original (bottom) block. */
    gs_free_ref_array(mem, &pstack->current, "ref_stack_release");
}

/*
 * Release a stack and then free the ref_stack object.
 */
void
ref_stack_free(ref_stack_t *pstack)
{
    gs_memory_t *mem = (gs_memory_t *)pstack->memory;

    ref_stack_release(pstack);
    gs_free_object(mem, pstack, "ref_stack_free");
}

/* ------ Internal routines ------ */

/* Initialize the guards and body of a stack block. */
private void
init_block(ref_stack_t *pstack, const ref *psb, uint used)
{
    ref_stack_params_t *params = pstack->params;
    ref *brefs = psb->value.refs;
    uint i;
    ref *p;

    for (i = params->bot_guard, p = brefs + stack_block_refs;
	 i != 0; i--, p++
	)
	ref_assign(p, &params->guard_value);
    /* The top guard elements will never be read, */
    /* but we need to initialize them for the sake of the GC. */
    /* We can use refset_null for this, because even though it uses */
    /* make_null_new and stack elements must not be marked new, */
    /* these slots will never actually be read or written. */
    if (params->top_guard) {
	ref *top = brefs + r_size(psb);
	int top_guard = params->top_guard;

	refset_null_new(top - top_guard, top_guard, 0);
    } {
	ref_stack_block *const pblock = (ref_stack_block *) brefs;

	pblock->used = *psb;
	pblock->used.value.refs = brefs + stack_block_refs + params->bot_guard;
	r_set_size(&pblock->used, 0);
    }
}
