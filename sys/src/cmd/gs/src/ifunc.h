/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: ifunc.h,v 1.3.6.2 2002/01/17 06:57:55 dancoby Exp $ */
/* Internal interpreter interfaces for Functions */

#ifndef ifunc_INCLUDED
#  define ifunc_INCLUDED

#include "gsfunc.h"

/* Define build procedures for the various function types. */
#define build_function_proc(proc)\
  int proc(P6(i_ctx_t *i_ctx_p, const ref *op, const gs_function_params_t *params, int depth,\
	      gs_function_t **ppfn, gs_memory_t *mem))
typedef build_function_proc((*build_function_proc_t));

/* Define the table of build procedures, indexed by FunctionType. */
typedef struct build_function_type_s {
    int type;
    build_function_proc_t proc;
} build_function_type_t;
extern const build_function_type_t build_function_type_table[];
extern const uint build_function_type_table_count;

/* Build a function structure from a PostScript dictionary. */
int fn_build_function(P4(i_ctx_t *i_ctx_p, const ref * op, gs_function_t ** ppfn,
			 gs_memory_t *mem));
int fn_build_sub_function(P5(i_ctx_t *i_ctx_p, const ref * op, gs_function_t ** ppfn,
			     int depth, gs_memory_t *mem));

/* Allocate an array of function objects. */
int alloc_function_array(P3(uint count, gs_function_t *** pFunctions,
			    gs_memory_t *mem));

/*
 * Collect a heap-allocated array of floats.  If the key is missing, set
 * *pparray = 0 and return 0; otherwise set *pparray and return the number
 * of elements.  Note that 0-length arrays are acceptable, so if the value
 * returned is 0, the caller must check whether *pparray == 0.
 */
int fn_build_float_array(P6(const ref * op, const char *kstr, bool required,
			    bool even, const float **pparray,
			    gs_memory_t *mem));

/*
 * If a PostScript object is a Function procedure, return the function
 * object, otherwise return 0.
 */
gs_function_t *ref_function(P1(const ref *op));

/*
 * Operator to execute a function.
 * <in1> ... <function_struct> %execfunction <out1> ...
 */
int zexecfunction(P1(i_ctx_t *));

#endif /* ifunc_INCLUDED */
