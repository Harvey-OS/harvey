/* Copyright (C) 1992, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: istack.h,v 1.2 2000/09/19 19:00:46 lpd Exp $ */
/* Definitions for expandable stacks of refs */
/* Requires iref.h */

#ifndef istack_INCLUDED
#  define istack_INCLUDED

#include "isdata.h"

/*
 * The 3 principal Ghostscript stacks (operand, execution, and dictionary)
 * are implemented as a linked list of blocks.
 *
 * Since all operators exit cleanly in case of stack under- or overflow,
 * we handle all issues related to stack blocks in the top-level error
 * recovery code in interp.c.  A few situations require special treatment:
 * see ostack.h, estack.h, and dstack.h for details.
 */

/*
 * Define the structure for a stack block.
 * In order to simplify allocation, stack blocks are implemented as
 * t_array objects, with the first few elements used for special purposes.
 * The actual layout is as follows:
 *              ref_stack_block structure
 *              bottom guard if any (see below)
 *              used elements of block
 *              unused elements of block
 *              top guard if any (see below)
 * The `next' member of the next higher stack block includes all of this.
 * The `used' member only includes the used elements of this block.
 * Notes:
 *      - In the top block, the size of the `used' member may not be correct.
 *      - In all blocks but the top, we fill the unused elements with nulls.
 */
typedef struct ref_stack_block_s {
    ref next;			/* t_array, next lower block on stack */
    ref used;			/* t_array, subinterval of this block */
    /* Actual stack starts here */
} ref_stack_block;

#define stack_block_refs (sizeof(ref_stack_block) / sizeof(ref))

/* ------ Procedural interface ------ */

/*
 * Initialize a stack.  Note that this allocates the stack parameter
 * structure iff params is not NULL.
 */
int ref_stack_init(P7(ref_stack_t *pstack, const ref *pblock_array,
		      uint bot_guard, uint top_guard,
		      const ref *pguard_value, gs_ref_memory_t *mem,
		      ref_stack_params_t *params));

/* Set whether a stack is allowed to expand.  The initial value is true. */
void ref_stack_allow_expansion(P2(ref_stack_t *pstack, bool expand));

/* Set the error codes for under- and overflow.  The initial values are -1. */
void ref_stack_set_error_codes(P3(ref_stack_t *pstack, int underflow_error,
				  int overflow_error));

/*
 * Set the maximum number of elements allowed on a stack.
 * Note that the value is a long, not a uint or a ulong.
 */
int ref_stack_set_max_count(P2(ref_stack_t *pstack, long nmax));

/*
 * Set the margin between the limit and the top of the stack.
 * Note that this may require allocating a block.
 */
int ref_stack_set_margin(P2(ref_stack_t *pstack, uint margin));

/* Return the number of elements on a stack. */
uint ref_stack_count(P1(const ref_stack_t *pstack));

#define ref_stack_count_inline(pstk)\
  ((pstk)->p + 1 - (pstk)->bot + (pstk)->extension_used)

/* Return the maximum number of elements allowed on a stack. */
#define ref_stack_max_count(pstk) (uint)((pstk)->max_stack.value.intval)

/*
 * Return a pointer to a given element from the stack, counting from
 * 0 as the top element.  If the index is out of range, return 0.
 * Note that the index is a long, not a uint or a ulong.
 */
ref *ref_stack_index(P2(const ref_stack_t *pstack, long index));

/*
 * Count the number of elements down to and including the first mark.
 * If no mark is found, return 0.
 */
uint ref_stack_counttomark(P1(const ref_stack_t *pstack));

/*
 * Do the store check for storing 'count' elements of a stack, starting
 * 'skip' elements below the top, into an array.  Return 0 or e_invalidaccess.
 */
int ref_stack_store_check(P4(const ref_stack_t *pstack, ref *parray,
			     uint count, uint skip));

/*
 * Store the top 'count' elements of a stack, starting 'skip' elements below
 * the top, into an array, with or without store/undo checking.  age=-1 for
 * no check, 0 for old, 1 for new.  May return e_rangecheck or
 * e_invalidaccess.
 */
#ifndef gs_dual_memory_DEFINED
#  define gs_dual_memory_DEFINED
typedef struct gs_dual_memory_s gs_dual_memory_t;
#endif
int ref_stack_store(P8(const ref_stack_t *pstack, ref *parray, uint count,
		       uint skip, int age, bool check,
		       gs_dual_memory_t *idmem, client_name_t cname));

/*
 * Pop the top N elements off a stack.
 * The number must not exceed the number of elements in use.
 */
void ref_stack_pop(P2(ref_stack_t *pstack, uint count));

#define ref_stack_clear(pstk) ref_stack_pop(pstk, ref_stack_count(pstk))
#define ref_stack_pop_to(pstk, depth)\
  ref_stack_pop(pstk, ref_stack_count(pstk) - (depth))

/* Pop the top block off a stack.  May return underflow_error. */
int ref_stack_pop_block(P1(ref_stack_t *pstack));

/*
 * Extend a stack to recover from an overflow condition.
 * Uses the requested value to decide what to do.
 * May return overflow_error or e_VMerror.
 */
int ref_stack_extend(P2(ref_stack_t *pstack, uint request));

/*
 * Push N empty slots onto a stack.  These slots are not initialized:
 * the caller must immediately fill them.  May return overflow_error
 * (if max_stack would be exceeded, or the stack has no allocator)
 * or e_VMerror.
 */
int ref_stack_push(P2(ref_stack_t *pstack, uint count));

/*
 * Enumerate the blocks of a stack from top to bottom, as follows:

   ref_stack_enum_t rsenum;

   ref_stack_enum_begin(&rsenum, pstack);
   do {
      ... process rsenum.size refs starting at rsenum.ptr ...
   } while (ref_stack_enum_next(&rsenum));

 */
typedef struct ref_stack_enum_s {
    ref_stack_block *block;
    ref *ptr;
    uint size;
} ref_stack_enum_t;
void ref_stack_enum_begin(P2(ref_stack_enum_t *prse,
			     const ref_stack_t *pstack));
bool ref_stack_enum_next(P1(ref_stack_enum_t *prse));

/* Clean up a stack for garbage collection. */
void ref_stack_cleanup(P1(ref_stack_t *pstack));

/*
 * Free the entire contents of a stack, including the bottom block.  The
 * client must still free the ref_stack_t object.  Note that after calling
 * ref_stack_release, the stack is no longer usable.
 */
void ref_stack_release(P1(ref_stack_t *pstack));

/*
 * Release a stack and then free the stack object.
 */
void ref_stack_free(P1(ref_stack_t *pstack));

#endif /* istack_INCLUDED */
