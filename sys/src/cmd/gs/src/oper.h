/* Copyright (C) 1989, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: oper.h,v 1.6 2003/09/03 03:22:59 giles Exp $ */
/* Definitions for Ghostscript operators */

#ifndef oper_INCLUDED
#  define oper_INCLUDED

#include "ierrors.h"
#include "ostack.h"
#include "opdef.h"
#include "opextern.h"
#include "opcheck.h"
#include "iutil.h"

/*
 * Operator procedures take a single argument.  This is currently a pointer
 * to the current context state, but might conceivably change in the future.
 * They return 0 for success, a negative code for an error, or a positive
 * code for some uncommon situations (see below).
 */

/*
 * In order to combine typecheck and stackunderflow error checking
 * into a single test, we guard the bottom of the o-stack with
 * additional entries of type t__invalid.  However, if a type check fails,
 * we must make an additional check to determine which error
 * should be reported.  In order not to have to make this check in-line
 * in every type check in every operator, we define a procedure that takes
 * an o-stack pointer and returns e_stackunderflow if it points to
 * a guard entry, e_typecheck otherwise.
 *
 * Note that we only need to do this for typecheck, not for any other
 * kind of error such as invalidaccess, since any operator that can
 * generate the latter will do a check_type or a check_op first.
 * (See ostack.h for more information.)
 *
 * We define the operand type of check_type_failed as const ref * rather than
 * const_os_ptr simply because there are a number of routines that might
 * be used either with a stack operand or with a general operand, and
 * the check for t__invalid is harmless when applied to off-stack refs.
 */
int check_type_failed(const ref *);

/*
 * Check the type of an object.  Operators almost always use check_type,
 * which includes the stack underflow check described just above;
 * check_type_only is for checking subsidiary objects obtained from
 * places other than the stack.
 */
#define return_op_typecheck(op)\
  return_error(check_type_failed(op))
#define check_type(orf,typ)\
  if ( !r_has_type(&orf,typ) ) return_op_typecheck(&orf)
#define check_stype(orf,styp)\
  if ( !r_has_stype(&orf,imemory,styp) ) return_op_typecheck(&orf)
#define check_array(orf)\
  check_array_else(orf, return_op_typecheck(&orf))
#define check_type_access(orf,typ,acc1)\
  if ( !r_has_type_attrs(&orf,typ,acc1) )\
    return_error((!r_has_type(&orf,typ) ? check_type_failed(&orf) :\
		  e_invalidaccess))
#define check_read_type(orf,typ)\
  check_type_access(orf,typ,a_read)
#define check_write_type(orf,typ)\
  check_type_access(orf,typ,a_write)

/* Macro for as yet unimplemented operators. */
/* The if ( 1 ) is to prevent the compiler from complaining about */
/* unreachable code. */
#define NYI(msg) if ( 1 ) return_error(e_undefined)

/*
 * If an operator has popped or pushed something on the control stack,
 * it must return o_pop_estack or o_push_estack respectively,
 * rather than 0, to indicate success.
 * It is OK to return o_pop_estack if nothing has been popped,
 * but it is not OK to return o_push_estack if nothing has been pushed.
 *
 * If an operator has suspended the current context and wants the
 * interpreter to call the scheduler, it must return o_reschedule.
 * It may also have pushed or popped elements on the control stack.
 * (This is only used when the Display PostScript option is included.)
 *
 * These values must be greater than 1, and far enough apart from zero and
 * from each other not to tempt a compiler into implementing a 'switch'
 * on them using indexing rather than testing.
 */
#define o_push_estack 5
#define o_pop_estack 14
#define o_reschedule 22

#endif /* oper_INCLUDED */
