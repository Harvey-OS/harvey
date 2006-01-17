/* Copyright (C) 1989, 1992, 1993, 1994, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: estack.h,v 1.6 2002/06/16 04:47:10 lpd Exp $ */
/* Definitions for the execution stack */

#ifndef estack_INCLUDED
#  define estack_INCLUDED

#include "iestack.h"
#include "icstate.h"		/* for access to exec_stack */

/* Define access to the cached current_file pointer. */
#define esfile (iexec_stack.current_file)
#define esfile_clear_cache() estack_clear_cache(&iexec_stack)
#define esfile_set_cache(pref) estack_set_cache(&iexec_stack, pref)
#define esfile_check_cache() estack_check_cache(&iexec_stack)

/* Define the execution stack pointers for operators. */
#define iexec_stack (i_ctx_p->exec_stack)
#define e_stack (iexec_stack.stack)

#define esbot (e_stack.bot)
#define esp (e_stack.p)
#define estop (e_stack.top)

/*
 * The execution stack holds several different kinds of objects (refs)
 * related to executing PostScript code:
 *
 *      - Procedures being executed are held here.  They always have
 * type = t_array, t_mixedarray, or t_shortarray, with a_executable set.
 * More specifically, the e-stack holds the as yet unexecuted tail of the
 * procedure.
 *
 *      - if, ifelse, etc. push arguments to be executed here.  They may be
 * any kind of object whatever.  Similarly, looping operators (forall, for,
 * etc.) push the procedure that is to be executed for each iteration.
 *
 *      - Control operators (filenameforall, for, repeat, loop, forall,
 * pathforall, run, stopped, ...) use continuations as described below.
 *
 * Note that there are many internal operators that need to use
 * continuations -- for example, all the 'show' operators, since they may
 * call out to BuildChar procedures.
 */

/*
 * Because the Ghostscript architecture doesn't allow recursive calls to the
 * interpreter, any operator that needs to call out to PostScript code (for
 * example, the 'show' operators calling a BuildChar procedure, or setscreen
 * sampling a spot function) must use a continuation -- an internal
 * "operator" procedure that continues the logical thread of execution after
 * the callout.  Operators needing to use continuations push the following
 * onto the execution stack (from bottom to top):
 *
 *	- An e-stack mark -- an executable null that indicates the bottom of
 *	the block associated with a callout.  (This should not be confused
 *	with a PostScript mark, a ref of type t_mark on the operand stack.)
 *	See make_mark_estack and push_mark_estack below.  The value.opproc
 *	member of the e-stack mark contains a procedure to execute in case
 *	the e-stack is stripped back beyond this point by a 'stop' or
 *	'exit': see pop_estack in zcontrol.c for details.
 *
 *	- Any number of refs holding information that the continuation
 *	operator needs -- i.e., the saved logical state of the thread of
 *	execution.  For example, 'for' stores the procedure, the current
 *	value, the increment, and the limit here.
 *
 *	- The continuation procedure itself -- the pseudo-operator to be
 *	called after returns from the interpreter callout.  See
 *	make_op_estack and push_op_estack below.
 *
 *	- The PostScript procedure for the interpreter to execute.
 *
 * The operator then returns o_push_estack, indicating to the interpreter
 * that the operator has pushed information on the e-stack for the
 * interpreter to process.
 *
 * When the interpreter finishes executing the PostScript procedure, it pops
 * the next item off the e-stack, which is the continuation procedure.  When
 * the continuation procedure gets control, the top of the e-stack (esp)
 * points just below the continuation procedure slot -- i.e., to the topmost
 * saved state item.  The continuation procedure normally pops all of the
 * saved state, and the e-stack mark, and continues execution normally,
 * eventually returning o_pop_estack to tell the interpreter that the
 * "operator" has popped information off the e-stack.  (Loop operators do
 * something a bit more efficient than popping the information and then
 * pushing it again: refer to the examples in zcontrol.c for details.)
 *
 * Continuation procedures are called just like any other operator, so they
 * can call each other, or be called from ordinary operator procedures, as
 * long as the e-stack is in the right state.  The most complex example of
 * this is probably the Type 1 character rendering code in zchar1.c, where
 * continuation procedures either call each other directly or call out to
 * the interpreter to execute optional PostScript procedures like CDevProc.
 */

/* Macro for marking the execution stack */
#define make_mark_estack(ep, es_idx, proc)\
  make_tasv(ep, t_null, a_executable, es_idx, opproc, proc)
#define push_mark_estack(es_idx, proc)\
  (++esp, make_mark_estack(esp, es_idx, proc))
#define r_is_estack_mark(ep)\
  r_has_type_attrs(ep, t_null, a_executable)
#define estack_mark_index(ep) r_size(ep)
#define set_estack_mark_index(ep, es_idx) r_set_size(ep, es_idx)

/* Macro for pushing an operator on the execution stack */
/* to represent a continuation procedure */
#define make_op_estack(ep, proc)\
  make_oper(ep, 0, proc)
#define push_op_estack(proc)\
  (++esp, make_op_estack(esp, proc))

/* Macro to ensure enough room on the execution stack */
#define check_estack(n)\
  if ( esp > estop - (n) )\
    { int es_code_ = ref_stack_extend(&e_stack, n);\
      if ( es_code_ < 0 ) return es_code_;\
    }

/* Macro to ensure enough entries on the execution stack */
#define check_esp(n)\
  if ( esp < esbot + ((n) - 1) )\
    { e_stack.requested = (n); return_error(e_ExecStackUnderflow); }

/* Define the various kinds of execution stack marks. */
#define es_other 0		/* internal use */
#define es_show 1		/* show operators */
#define es_for 2		/* iteration operators */
#define es_stopped 3		/* stopped operator */

/*
 * Pop a given number of elements off the execution stack,
 * executing cleanup procedures as necessary.
 */
void pop_estack(i_ctx_t *, uint);

/*
 * The execution stack is implemented as a linked list of blocks;
 * operators that can push or pop an unbounded number of values, or that
 * access the entire e-stack, must take this into account.  These are:
 *      exit  .stop  .instopped  countexecstack  execstack  currentfile
 *      .execn
 *      pop_estack(exit, stop, error recovery)
 *      gs_show_find(all the show operators)
 * In addition, for e-stack entries created by control operators, we must
 * ensure that the mark and its data are never separated.  We do this
 * by ensuring that when splitting the top block, at least N items
 * are kept in the new top block above the bottommost retained mark,
 * where N is the largest number of data items associated with a mark.
 * Finally, in order to avoid specific checks for underflowing a block,
 * we put a guard entry at the bottom of each block except the top one
 * that contains a procedure that returns an internal "exec stack block
 * underflow" error.
 */

#endif /* estack_INCLUDED */
