/* Copyright (C) 1993, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iparam.h,v 1.5 2002/06/16 04:47:10 lpd Exp $ */
/* Definitions and interface for interpreter parameter list implementations */
/* Requires ialloc.h, istack.h */

#ifndef iparam_INCLUDED
#  define iparam_INCLUDED

#include "gsparam.h"

/*
 * This file defines the interface to iparam.c, which provides
 * several implementations of the parameter dictionary interface
 * defined in gsparam.h:
 *      - an implementation using dictionary objects;
 *      - an implementation using name/value pairs in an array;
 *      - an implementation using name/value pairs on a stack.
 *
 * When reading ('putting'), these implementations keep track of
 * which parameters have been referenced and which have caused errors.
 * The results array contains 0 for a parameter that has not been accessed,
 * 1 for a parameter accessed without error, or <0 for an error.
 */

typedef struct iparam_loc_s {
    ref *pvalue;		/* (actually const) */
    int *presult;
} iparam_loc;

#define iparam_list_common\
    gs_param_list_common;\
    gs_ref_memory_t *ref_memory; /* a properly typed copy of memory */\
    union {\
      struct {	/* reading */\
	int (*read)(iparam_list *, const ref *, iparam_loc *);\
	ref policies;	/* policy dictionary or null */\
	bool require_all;	/* if true, require all params to be known */\
      } r;\
      struct {		/* writing */\
	int (*write)(iparam_list *, const ref *, const ref *);\
	ref wanted;		/* desired keys or null */\
      } w;\
    } u;\
    int (*enumerate)(iparam_list *, gs_param_enumerator_t *, gs_param_key_t *, ref_type *);\
    int *results;		/* (only used when reading, 0 when writing) */\
    uint count;		/* # of key/value pairs */\
    bool int_keys		/* if true, keys are integers */
typedef struct iparam_list_s iparam_list;
struct iparam_list_s {
    iparam_list_common;
};

typedef struct dict_param_list_s {
    iparam_list_common;
    ref dict;			/* dictionary or array */
} dict_param_list;
typedef struct array_param_list_s {
    iparam_list_common;
    ref *bot;
    ref *top;
} array_param_list;

/* For stack lists, the bottom of the list is just above a mark. */
typedef struct stack_param_list_s {
    iparam_list_common;
    ref_stack_t *pstack;
    uint skip;			/* # of top items to skip (reading only) */
} stack_param_list;

/* Procedural interface */
/*
 * For dict_param_list_read (only), the second parameter may be NULL,
 * equivalent to an empty dictionary.
 * The 3rd (const ref *) parameter is the policies dictionary when reading,
 * or the key selection dictionary when writing; it may be NULL in either case.
 * If the bool parameter is true, if there are any unqueried parameters,
 * the commit procedure will return an e_undefined error.
 */
int dict_param_list_read(dict_param_list *, const ref * /*t_dictionary */ ,
			 const ref *, bool, gs_ref_memory_t *);
int dict_param_list_write(dict_param_list *, ref * /*t_dictionary */ ,
			  const ref *, gs_ref_memory_t *);
int array_indexed_param_list_read(dict_param_list *, const ref * /*t_*array */ ,
				  const ref *, bool, gs_ref_memory_t *);
int array_indexed_param_list_write(dict_param_list *, ref * /*t_*array */ ,
				   const ref *, gs_ref_memory_t *);
int array_param_list_read(array_param_list *, ref *, uint,
			  const ref *, bool, gs_ref_memory_t *);
int stack_param_list_read(stack_param_list *, ref_stack_t *, uint,
			  const ref *, bool, gs_ref_memory_t *);
int stack_param_list_write(stack_param_list *, ref_stack_t *,
			   const ref *, gs_ref_memory_t *);

#define iparam_list_release(plist)\
  gs_free_object((plist)->memory, (plist)->results, "iparam_list_release")

#endif /* iparam_INCLUDED */
