/* Copyright (C) 1991, 1994, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: ostack.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Definitions for Ghostscript operand stack */

#ifndef ostack_INCLUDED
#  define ostack_INCLUDED

#include "iostack.h"
#include "icstate.h"		/* for access to op_stack */

/* Define the operand stack pointers for operators. */
#define iop_stack (i_ctx_p->op_stack)
#define o_stack (iop_stack.stack)

#define osbot (o_stack.bot)
#define osp (o_stack.p)
#define ostop (o_stack.top)

/* Macro to ensure enough room on the operand stack */
#define check_ostack(n)\
  if ( ostop - osp < (n) )\
    { o_stack.requested = (n); return_error(e_stackoverflow); }

/* Operand stack manipulation. */

/* Note that push sets osp to (the new value of) op. */
#define push(n)\
  BEGIN\
    if ( (op += (n)) > ostop )\
      { o_stack.requested = (n); return_error(e_stackoverflow); }\
    else osp = op;\
  END

/*
 * Note that the pop macro only decrements osp, not op.  For this reason,
 *
 *      >>>     pop should only be used just before returning,  <<<
 *      >>>     or else op must be decremented explicitly.      <<<
 */
#define pop(n) (osp -= (n))

/*
 * Note that the interpreter does not check for operand stack underflow
 * before calling the operator procedure.  There are "guard" entries
 * with invalid types and attributes just below the bottom of the
 * operand stack: if the operator returns with a typecheck error,
 * the interpreter checks for underflow at that time.
 * Operators that don't typecheck their arguments must check for
 * operand stack underflow explicitly; operators that take a variable
 * number of arguments must also check for stack underflow in those cases
 * where they expect more than their minimum number of arguments.
 * (This is because the interpreter can only recognize that a typecheck
 * is really a stackunderflow when the stack has fewer than the
 * operator's declared minimum number of entries.)
 */
#define check_op(nargs)\
  if ( op < osbot + ((nargs) - 1) ) return_error(e_stackunderflow)
/*
 * Similarly, in order to simplify some overflow checks, we allocate
 * a few guard entries just above the top of the o-stack.
 */

/*
 * The operand stack is implemented as a linked list of blocks:
 * operators that can push or pop an unbounded number of values, or that
 * access the entire o-stack, must take this into account.  These are:
 *      (int)copy  index  roll  clear  count  cleartomark
 *      counttomark  aload  astore  packedarray
 *      .get/.putdeviceparams .gethardwareparams
 */

#endif /* ostack_INCLUDED */
