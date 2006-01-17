/* Copyright (C) 1994, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: interp.h,v 1.6 2003/10/15 08:53:13 igor Exp $ */
/* Internal interfaces to interp.c and iinit.c */

#ifndef interp_INCLUDED
#  define interp_INCLUDED

/* ------ iinit.c ------ */

/* Enter a name and value into systemdict. */
int i_initial_enter_name(i_ctx_t *, const char *, const ref *);
#define initial_enter_name(nstr, pvalue)\
  i_initial_enter_name(i_ctx_p, nstr, pvalue)

/* Remove a name from systemdict. */
void i_initial_remove_name(i_ctx_t *, const char *);
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
void gs_interp_make_oper(ref * opref, op_proc_t, int index);

/*
 * Call the garbage collector, updating the context pointer properly.
 */
int interp_reclaim(i_ctx_t **pi_ctx_p, int space);

/* Get the name corresponding to an error number. */
int gs_errorname(i_ctx_t *, int, ref *);

/* Put a string in $error /errorinfo. */
int gs_errorinfo_put_string(i_ctx_t *, const char *);

/* Initialize the interpreter. */
int gs_interp_init(i_ctx_t **pi_ctx_p, const ref *psystem_dict,
		   gs_dual_memory_t *dmem);

#ifndef gs_context_state_t_DEFINED
#  define gs_context_state_t_DEFINED
typedef struct gs_context_state_s gs_context_state_t;
#endif

/*
 * Create initial stacks for the interpreter.
 * We export this for creating new contexts.
 */
int gs_interp_alloc_stacks(gs_ref_memory_t * smem,
			   gs_context_state_t * pcst);

/*
 * Free the stacks when destroying a context.  This is the inverse of
 * create_stacks.
 */
void gs_interp_free_stacks(gs_ref_memory_t * smem,
			   gs_context_state_t * pcst);

/* Reset the interpreter. */
void gs_interp_reset(i_ctx_t *i_ctx_p);

/* Define the top-level interface to the interpreter. */
int gs_interpret(i_ctx_t **pi_ctx_p, ref * pref, int user_errors,
		 int *pexit_code, ref * perror_object);

#endif /* interp_INCLUDED */
