/* Copyright (C) 1989, 1992, 1993, 1994, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: estack.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
/* Definitions for the execution stack */

#ifndef estack_INCLUDED
#  define estack_INCLUDED

#include "iestack.h"
#include "icstate.h"		/* for access to exec_stack */

/* There's only one exec stack right now.... */
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
 * The execution stack is used for three purposes:
 *
 *      - Procedures being executed are held here.  They always have
 * type = t_array, t_mixedarray, or t_shortarray, with a_executable set.
 * More specifically, the e-stack holds the as yet unexecuted tail of the
 * procedure.
 *
 *      - if, ifelse, etc. push arguments to be executed here.
 * They may be any kind of object whatever.
 *
 *      - Control operators (filenameforall, for, repeat, loop, forall,
 * pathforall, run, stopped, ...) mark the stack by pushing whatever state
 * they need to save or keep track of and then an object with type = t_null,
 * attrs = a_executable, size = es_xxx (see below), and value.opproc = a
 * cleanup procedure that will get called whenever the execution stack is
 * about to get cut back beyond this point because of an error, stop, exit,
 * or quit.  (Executable null objects can't ever appear on the e-stack
 * otherwise: if a control operator pushes one, it gets popped immediately.)
 * The cleanup procedure is called with esp pointing just BELOW the mark,
 * i.e., the mark has already been popped.
 *
 * The loop operators also push whatever state they need,
 * followed by an operator object that handles continuing the loop.
 *
 * Note that there are many internal operators that need to be handled like
 * looping operators -- for example, all the 'show' operators, since they
 * may call out to BuildChar procedures.
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
void pop_estack(P2(i_ctx_t *, uint));

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
