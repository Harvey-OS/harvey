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

/* interp.h */
/* Internal interfaces to interp.c and iinit.c */

/* ------ iinit.c ------ */

/* Enter a name and value into systemdict. */
void initial_enter_name(P2(const char *, const ref *));

/* ------ interp.c ------ */

/*
 * Maximum number of arguments (and results) for an operator,
 * determined by operand stack block size.
 */
extern const int gs_interp_max_op_num_args;

/*
 * Number of slots to reserve at the start of op_def_table for
 * operators which are hard-coded into the interpreter loop.
 */
extern const int gs_interp_num_special_ops;

/*
 * Create an operator during initialization.
 * If operator is hard-coded into the interpreter,
 * assign it a special type and index.
 */
void gs_interp_make_oper(P3(ref *opref, op_proc_p, int index));

/* Get the name corresponding to an error number. */
int gs_errorname(P2(int, ref *));

/* Put a string in $error /errorinfo. */
int gs_errorinfo_put_string(P1(const char *));

/* Initialize the interpreter. */
void gs_interp_init(P0());

/* Reset the interpreter. */
void gs_interp_reset(P0());
