/* Copyright (C) 1994, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: interp.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Internal interfaces to interp.c and iinit.c */

#ifndef interp_INCLUDED
#  define interp_INCLUDED

/* ------ iinit.c ------ */

/* Enter a name and value into systemdict. */
int i_initial_enter_name(P3(i_ctx_t *, const char *, const ref *));
#define initial_enter_name(nstr, pvalue)\
  i_initial_enter_name(i_ctx_p, nstr, pvalue)

/* Remove a name from systemdict. */
void i_initial_remove_name(P2(i_ctx_t *, const char *));
#define initial_remove_name(nstr)\
  i_initial_remove_name(i_ctx_p, nstr)

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
void gs_interp_make_oper(P3(ref * opref, op_proc_t, int index));

/* Get the name corresponding to an error number. */
int gs_errorname(P3(i_ctx_t *, int, ref *));

/* Put a string in $error /errorinfo. */
int gs_errorinfo_put_string(P2(i_ctx_t *, const char *));

/* Initialize the interpreter. */
int gs_interp_init(P3(i_ctx_t **pi_ctx_p, const ref *psystem_dict,
		      gs_dual_memory_t *dmem));

#ifndef gs_context_state_t_DEFINED
#  define gs_context_state_t_DEFINED
typedef struct gs_context_state_s gs_context_state_t;
#endif

/*
 * Create initial stacks for the interpreter.
 * We export this for creating new contexts.
 */
int gs_interp_alloc_stacks(P2(gs_ref_memory_t * smem,
			      gs_context_state_t * pcst));

/*
 * Free the stacks when destroying a context.  This is the inverse of
 * create_stacks.
 */
void gs_interp_free_stacks(P2(gs_ref_memory_t * smem,
			      gs_context_state_t * pcst));

/* Reset the interpreter. */
void gs_interp_reset(P1(i_ctx_t *i_ctx_p));

/* Define the top-level interface to the interpreter. */
int gs_interpret(P5(i_ctx_t **pi_ctx_p, ref * pref, int user_errors,
		    int *pexit_code, ref * perror_object));

#endif /* interp_INCLUDED */
