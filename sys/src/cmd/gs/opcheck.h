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

/* opcheck.h */
/* Operand checking for Ghostscript operators */
/* Requires ialloc.h (for imemory), iref.h, errors.h */

/*
 * Check the type of an object.  Operators almost always use check_type,
 * which is defined in oper.h; check_type_only is for checking
 * subsidiary objects obtained from places other than the stack.
 */
#define check_type_only(rf,typ)\
  if ( !r_has_type(&rf,typ) ) return_error(e_typecheck)
#define check_stype_only(rf,styp)\
  if ( !r_has_stype(&rf,imemory,styp) ) return_error(e_typecheck)
/* Check for array */
#define check_array_else(rf,errstat)\
  if ( !r_has_type(&rf, t_array) ) errstat
#define check_array_only(rf)\
  check_array_else(rf, return_error(e_typecheck))
/* Check for procedure.  check_proc_failed includes the stack underflow */
/* check, but it doesn't do any harm in the off-stack case. */
int check_proc_failed(P1(const ref *));
#define check_proc(rf)\
  if ( !r_is_proc(&rf) ) return_error(check_proc_failed(&rf));
#define check_proc_only(rf) check_proc(rf)

/* Check for read, write, or execute access. */
#define check_access(rf,acc1)\
  if ( !r_has_attr(&rf,acc1) ) return_error(e_invalidaccess)
#define check_read(rf) check_access(rf,a_read)
#define check_type_access_only(rf,typ,acc1)\
  if ( !r_has_type_attrs(&rf,typ,acc1) )\
    return_error((!r_has_type(&rf,typ) ? e_typecheck : e_invalidaccess))
#define check_read_type_only(rf,typ)\
  check_type_access_only(rf,typ,a_read)
#define check_write(rf) check_access(rf,a_write)
#define check_execute(rf) check_access(rf,a_execute)

/* Check for an integer value within an unsigned bound. */
#define check_int_leu(orf, u)\
  check_type(orf, t_integer);\
  if ( (ulong)(orf).value.intval > (u) ) return_error(e_rangecheck)
#define check_int_leu_only(rf, u)\
  check_type_only(rf, t_integer);\
  if ( (ulong)(rf).value.intval > (u) ) return_error(e_rangecheck)
#define check_int_ltu(orf, u)\
  check_type(orf, t_integer);\
  if ( (ulong)(orf).value.intval >= (u) ) return_error(e_rangecheck)
