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

/* iparam.h */
/* Interpreter-level implementations of parameter dictionaries */
/* Requires istack.h */
#include "gsparam.h"

/*
 * This file defines the interface to iparam.c, which provides
 * several implementations of the parameter dictionary interface
 * defined in gsparam.h:
 *	- an implementation using dictionary objects;
 *	- an implementation using name/value pairs in an array;
 *	- an implementation using name/value pairs on a stack.
 *
 * When reading ('putting'), these implementations keep track of
 * which parameters have been referenced and which have caused errors.
 * The results array contains 0 for a parameter that has not been accessed,
 * 1 for a parameter accessed without error, or <0 for an error.
 */

typedef struct iparam_loc_s {
	ref *pvalue;			/* (actually const) */
	int *presult;
} iparam_loc;

#define iparam_list_common\
	gs_param_list_common;\
	union {\
	  struct {		/* reading */\
	    int (*read)(P3(iparam_list *, const ref *, iparam_loc *));\
	    ref policies;	/* policy dictionary or null */\
	    bool require_all;	/* if true, require all params to be known */\
	  } r;\
	  struct {		/* writing */\
	    int (*write)(P3(iparam_list *, const ref *, ref *));\
	    ref wanted;		/* desired keys or null */\
	  } w;\
	} u;\
	int *results;		/* (only used when reading, 0 when writing) */\
	uint count		/* # of key/value pairs */
typedef struct iparam_list_s iparam_list;
struct iparam_list_s {
	iparam_list_common;
};

typedef struct dict_param_list_s {
	iparam_list_common;
	ref dict;
} dict_param_list;
typedef struct array_param_list_s {
	iparam_list_common;
	ref *bot;
	ref *top;
} array_param_list;
/* For stack lists, the bottom of the list is just above a mark. */
typedef struct stack_param_list_s {
	iparam_list_common;
	ref_stack *pstack;
	uint skip;		/* # of top items to skip (reading only) */
} stack_param_list;

/* Procedural interface */
/*
 * The const ref * parameter is the policies dictionary when reading,
 * or the key selection dictionary when writing; It may be NULL in either case.
 * If the bool parameter is true, if there are any unqueried parameters,
 * the commit procedure will return an e_undefined error.
 */
int dict_param_list_read(P4(dict_param_list *, const ref * /*t_dictionary*/,
  const ref *, bool));
int dict_param_list_write(P3(dict_param_list *, ref * /*t_dictionary*/,
  const ref *));
int array_param_list_read(P5(array_param_list *, ref *, uint,
  const ref *, bool));
int stack_param_list_read(P5(stack_param_list *, ref_stack *, uint,
  const ref *, bool));
int stack_param_list_write(P3(stack_param_list *, ref_stack *,
  const ref *));
#define iparam_list_release(plist)\
  ifree_object((plist)->results, "iparam_list_release")
