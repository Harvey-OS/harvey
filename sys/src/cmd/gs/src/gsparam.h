/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsparam.h,v 1.13 2005/09/05 13:58:55 leonardo Exp $ */
/* Client interface to parameter dictionaries */

#ifndef gsparam_INCLUDED
#  define gsparam_INCLUDED

#include "gsstype.h"

/*
 * Several interfaces use parameter dictionaries to communicate sets of
 * (key, value) pairs between a client and an object with complex state.
 * (Several of these correspond directly to similar interfaces in the
 * PostScript language.) This file defines the API for parameter dictionaries.
 */

/* ---------------- Generic interfaces ---------------- */

/* Define the abstract type for a parameter list. */
#ifndef gs_param_list_DEFINED
#  define gs_param_list_DEFINED
typedef struct gs_param_list_s gs_param_list;
#endif

/* Define the type for a parameter key name. */
typedef const char *gs_param_name;

/*
 * Parameter values fall into three categories:
 *      - Scalar (null, Boolean, int, long, float);
 *      - Homogenous collection (string/name, int array, float array,
 *      string/name array);
 *      - Heterogenous collection (dictionary, int-keyed dictionary, array).
 * Each category has its own representation and memory management issues.
 */
typedef enum {
    /* Scalar */
    gs_param_type_null, gs_param_type_bool, gs_param_type_int,
    gs_param_type_long, gs_param_type_float,
    /* Homogenous collection */
    gs_param_type_string, gs_param_type_name,
    gs_param_type_int_array, gs_param_type_float_array,
    gs_param_type_string_array, gs_param_type_name_array,
    /* Heterogenous collection */
    gs_param_type_dict, gs_param_type_dict_int_keys, gs_param_type_array
} gs_param_type;

/* Define a "don't care" type for reading typed values. */
#define gs_param_type_any ((gs_param_type)-1)

/*
 * Define the structures for homogenous collection values
 * (string/name, integer array, or floating point array).
 * The size is the number of elements, not the size in bytes.
 * A value is persistent if it is defined as static const,
 * or if it is allocated in garbage-collectable space and never freed.
 */

#define _param_array_struct(sname,etype)\
  struct sname { const etype *data; uint size; bool persistent; }
typedef _param_array_struct(gs_param_int_array_s, int) gs_param_int_array;
typedef _param_array_struct(gs_param_float_array_s, float) gs_param_float_array;
typedef _param_array_struct(gs_param_string_array_s, gs_param_string) gs_param_string_array;

#define param_string_from_string(ps, str)\
  ((ps).data = (const byte *)(str),\
   (ps).size = strlen((const char *)(ps).data),\
   (ps).persistent = true)

#define param_string_from_transient_string(ps, str)\
  ((ps).data = (const byte *)(str),\
   (ps).size = strlen((const char *)(ps).data),\
   (ps).persistent = false)

/*
 * Define the structure for heterogenous collection values (dictionaries
 * and heterogenous arrays).
 */
typedef struct gs_param_collection_s {
    gs_param_list *list;
    uint size;
} gs_param_collection;
typedef gs_param_collection gs_param_dict;
typedef gs_param_collection gs_param_array;

/*
 * Define the sizes of the various parameter value types, indexed by type.
 */
#define GS_PARAM_TYPE_SIZES(dict_size)\
  0, sizeof(bool), sizeof(int), sizeof(long), sizeof(float),\
  sizeof(gs_param_string), sizeof(gs_param_string),\
  sizeof(gs_param_int_array), sizeof(gs_param_float_array),\
  sizeof(gs_param_string_array), sizeof(gs_param_string_array),\
  (dict_size), (dict_size), (dict_size)
/*
 * Define the sizes of the underlying data types contained in or pointed
 * to by the various value types.
 */
#define GS_PARAM_TYPE_BASE_SIZES(dict_elt_size)\
  0, sizeof(bool), sizeof(int), sizeof(long), sizeof(float),\
  1, 1, sizeof(int), sizeof(float),\
  sizeof(gs_param_string), sizeof(gs_param_string),\
  (dict_elt_size), (dict_elt_size), (dict_elt_size)

/* Define tables with 0 for the sizes of the heterogenous collections. */
extern const byte gs_param_type_sizes[];
extern const byte gs_param_type_base_sizes[];

/* Define a union capable of holding any parameter value. */
#define GS_PARAM_VALUE_UNION(dict_type)\
	bool b;\
	int i;\
	long l;\
	float f;\
	gs_param_string s;\
	gs_param_string n;\
	gs_param_int_array ia;\
	gs_param_float_array fa;\
	gs_param_string_array sa;\
	gs_param_string_array na;\
	dict_type d
typedef union gs_param_value_s {
    GS_PARAM_VALUE_UNION(gs_param_collection);
} gs_param_value;

/*
 * Define a structure containing a dynamically typed value (a value along
 * with its type).
 */
typedef struct gs_param_typed_value_s {
    gs_param_value value;
    gs_param_type type;
} gs_param_typed_value;
/*
 * Garbage collection of gs_param_values depends on the value type and on
 * the 'd' member of the union.  We provide enum_ptrs and reloc_ptrs
 * procedures that handle all the other cases -- i.e., cases other than
 * heterogenous collections.
 */
struct_proc_enum_ptrs(gs_param_typed_value_enum_ptrs);
struct_proc_reloc_ptrs(gs_param_typed_value_reloc_ptrs);
#define gs_param_typed_value_max_ptrs 1

/*
 * Define the representation alternatives for heterogenous collections.
 * _any must be 0, for Boolean testing.
 */
typedef enum {

    /* Create or accept a general dictionary. */

    gs_param_collection_dict_any = 0,

    /* Create a dictionary with integer string keys ("0", "1", ...); */
    /* accept a dictionary with integer string keys, or a heterogenous */
    /* array. */

    gs_param_collection_dict_int_keys = 1,

    /* Create an array if possible, otherwise a dictionary with integer */
    /* string keys; accept the same types as dict_int_keys. */

    gs_param_collection_array = 2

} gs_param_collection_type_t;

/*
 * Define the 'policies' for handling out-of-range parameter values.
 * This is not an enum, because some parameters may recognize other values.
 */
#define gs_param_policy_signal_error 0
#define gs_param_policy_ignore 1
#define gs_param_policy_consult_user 2

/*
 * Define an enumerator used to iterate through the keys in a list.
 *
 * All the members of the union must be used such that memset(0) entire
 * union means 'beginning of enumeration'.
 */
typedef union gs_param_enumerator_s {
    int intval;
    long longval;
    void *pvoid;
    char *pchar;
} gs_param_enumerator_t;
typedef gs_param_string gs_param_key_t;

/*
 * Define the object procedures.  Note that the same interface is used
 * both for getting and for setting parameter values.  (This is a bit
 * of a hack, and we might change it someday.)  The procedures return
 * as follows:
 *      - 'reading' procedures ('put' operations from the client's viewpoint)
 * return 1 for a missing parameter, 0 for a valid parameter, <0 on error.
 *      - 'writing' procedures ('get' operations from the client's viewpoint)
 * return 0 or 1 if successful, <0 on error.
 *
 * A lazy implementation can use the default procedures for scalar and
 * homogenous collection types: these just called xmit_typed.
 */

/*
 * Transmitting variable-size objects requires some extra care.
 *      - When writing an array, string, name, or dictionary, the
 * implementation (not the client) sets all the fields of the value.
 *      - When reading an array, string, or name, the client must set
 * all the fields of the value.
 *      - When reading a dictionary, the client must set the size field
 * before calling begin_write_dict; the implementation of begin_write_dict
 * allocates the list.
 */

/*
 * Setting parameters must use a "two-phase commit" policy.  Specifically,
 * any put_params procedure must observe the following discipline:

 1. For each parameter known to the device, ask the parameter list if
 there is a new value, and if so, make all necessary validity checks.  If any
 check fails, call param_signal_error for that parameter, but continue to
 check further parameters.  Normally, this step should not alter the state of
 the device; however, if the device allows changing any parameters that are
 read-only by default (for example, BitsPerPixel or ProcessColorModel), or if
 it replaces the default put_params behavior for any parameter (for example,
 if it handles MediaSize or Resolution itself to forestall the normal closing
 of the device when these are set), step 1 of put_params must change the
 parameters in the device state, and step 2 must undo the changes if
 returning an error.

 2. Call the "superclass" put_params routine.  For printer devices,
 this is gdev_prn_put_params; for other devices, it is gx_default_put_params.
 Note that this must be done even if errors were detected in step 1.  If this
 routine returns an error code, or if step 1 detected an error, undo any
 changes that step 1 made in the device state, and return the error code.

 3. Install the new parameter values in the device.  If necessary,
 close the device first; a higher-level routine (gs_putdeviceparams) will
 reopen the device if necessary.

 */

typedef struct gs_param_list_procs_s {

    /* Transmit a typed value. */
    /*
     * Note that read/write_typed do a begin_read/write_collection
     * if the type is one of the heterogenous collection types.
     * Note also that even for reading, the caller must set pvalue->type
     * to the desired type or to gs_param_type_any.
     */

#define param_proc_xmit_typed(proc)\
    int proc(gs_param_list *, gs_param_name, gs_param_typed_value *)
	 param_proc_xmit_typed((*xmit_typed));
	 /* See below for param_read_[requested_]typed */
#define param_write_typed(plist, pkey, pvalue)\
	 (*(plist)->procs->xmit_typed)(plist, pkey, pvalue)

	 /* Start transmitting a dictionary or heterogenous value. */

#define param_proc_begin_xmit_collection(proc)\
	 int proc(gs_param_list *, gs_param_name, gs_param_dict *,\
		     gs_param_collection_type_t)
	 param_proc_begin_xmit_collection((*begin_xmit_collection));
#define param_begin_read_collection(plist, pkey, pvalue, coll_type)\
	 (*(plist)->procs->begin_xmit_collection)(plist, pkey, pvalue, coll_type)
#define param_begin_read_dict(l, k, v, int_keys)\
	 param_begin_read_collection(l, k, v,\
				     (int_keys ? gs_param_collection_dict_int_keys :\
				      gs_param_collection_dict_any))
#define param_begin_write_collection(plist, pkey, pvalue, coll_type)\
	 (*(plist)->procs->begin_xmit_collection)(plist, pkey, pvalue, coll_type)
#define param_begin_write_dict(l, k, v, int_keys)\
	 param_begin_write_collection(l, k, v,\
				      (int_keys ? gs_param_collection_dict_int_keys :\
				       gs_param_collection_dict_any))

	 /* Finish transmitting a collection value. */

#define param_proc_end_xmit_collection(proc)\
	 int proc(gs_param_list *, gs_param_name, gs_param_dict *)
	 param_proc_end_xmit_collection((*end_xmit_collection));
#define param_end_read_collection(plist, pkey, pvalue)\
	 (*(plist)->procs->end_xmit_collection)(plist, pkey, pvalue)
#define param_end_read_dict(l, k, v) param_end_read_collection(l, k, v)
#define param_end_write_collection(plist, pkey, pvalue)\
	 (*(plist)->procs->end_xmit_collection)(plist, pkey, pvalue)
#define param_end_write_dict(l, k, v) param_end_write_collection(l, k, v)

	 /* 
	  * Get the next key in sequence. 
	  * (Only used when reading.)
	  * Use param_init_enumerator(...) to reset to first key.
	  */

#define param_proc_next_key(proc)\
	 int proc(gs_param_list *, gs_param_enumerator_t *, gs_param_key_t *)
	 param_proc_next_key((*next_key));
#define param_get_next_key(plist, penum, pkey)\
	 (*(plist)->procs->next_key)(plist, penum, pkey)

	 /*
	  * Request a specific parameter. (Only used when writing, before
	  * writing any values.)  If no specific parameters are requested,
	  * param_requested always returns -1; if specific parameters
	  * are requested, param_requested will return 1 for those,
	  * and may return either 0 or 1 for others.
	  */

#define param_proc_request(proc)\
  int proc(gs_param_list *, gs_param_name)
	 param_proc_request((*request));

#define param_request(plist, pkey)\
  ((plist)->procs->request(plist, pkey))

	 /*
	  * Determine whether a given key has been requested.  (Only used
	  * when writing.)  A return value of -1 means that no specific
	  * parameters have been requested; 0 means specific parameters have
	  * been requested, but not this one; 1 means this parameter has
	  * been requested specifically.
	  */

#define param_proc_requested(proc)\
	 int proc(const gs_param_list *, gs_param_name)
	 param_proc_requested((*requested));
#define param_requested(plist, pkey)\
	 (*(plist)->procs->requested)(plist, pkey)

	 /* Get the 'policy' associated with an out-of-range parameter value. */
	 /* (Only used when reading.) */

#define param_proc_get_policy(proc)\
	 int proc(gs_param_list *, gs_param_name)
	 param_proc_get_policy((*get_policy));
#define param_get_policy(plist, pkey)\
	 (*(plist)->procs->get_policy)(plist, pkey)

	 /*
	  * Signal an error.  (Only used when reading.)
	  * The procedure may return a different error code,
	  * or may return 0 indicating that the error is to be ignored.
	  */

#define param_proc_signal_error(proc)\
	 int proc(gs_param_list *, gs_param_name, int)
	 param_proc_signal_error((*signal_error));
#define param_signal_error(plist, pkey, code)\
	 (*(plist)->procs->signal_error)(plist, pkey, code)
#define param_return_error(plist, pkey, code)\
	 return_error(param_signal_error(plist, pkey, code))

	 /*
	  * "Commit" a set of changes.  (Only used when reading.)
	  * This is called at the end of the first phase.
	  */

#define param_proc_commit(proc)\
	 int proc(gs_param_list *)
	 param_proc_commit((*commit));
#define param_commit(plist)\
	 (*(plist)->procs->commit)(plist)

} gs_param_list_procs;

/* Transmit typed parameters. */
int param_read_requested_typed(gs_param_list *, gs_param_name,
				  gs_param_typed_value *);

#define param_read_typed(plist, pkey, pvalue)\
  ((pvalue)->type = gs_param_type_any,\
   param_read_requested_typed(plist, pkey, pvalue))

/* Transmit parameters of specific types. */
int param_read_null(gs_param_list *, gs_param_name);
int param_write_null(gs_param_list *, gs_param_name);
int param_read_bool(gs_param_list *, gs_param_name, bool *);
int param_write_bool(gs_param_list *, gs_param_name, const bool *);
int param_read_int(gs_param_list *, gs_param_name, int *);
int param_write_int(gs_param_list *, gs_param_name, const int *);
int param_read_long(gs_param_list *, gs_param_name, long *);
int param_write_long(gs_param_list *, gs_param_name, const long *);
int param_read_float(gs_param_list *, gs_param_name, float *);
int param_write_float(gs_param_list *, gs_param_name, const float *);
int param_read_string(gs_param_list *, gs_param_name, gs_param_string *);
int param_write_string(gs_param_list *, gs_param_name,
		       const gs_param_string *);
int param_read_name(gs_param_list *, gs_param_name, gs_param_string *);
int param_write_name(gs_param_list *, gs_param_name,
		     const gs_param_string *);
int param_read_int_array(gs_param_list *, gs_param_name,
			 gs_param_int_array *);
int param_write_int_array(gs_param_list *, gs_param_name,
			  const gs_param_int_array *);
int param_write_int_values(gs_param_list *, gs_param_name,
			   const int *, uint, bool);
int param_read_float_array(gs_param_list *, gs_param_name,
			   gs_param_float_array *);
int param_write_float_array(gs_param_list *, gs_param_name,
			    const gs_param_float_array *);
int param_write_float_values(gs_param_list *, gs_param_name,
			     const float *, uint, bool);
int param_read_string_array(gs_param_list *, gs_param_name,
			    gs_param_string_array *);
int param_write_string_array(gs_param_list *, gs_param_name,
			     const gs_param_string_array *);
int param_read_name_array(gs_param_list *, gs_param_name,
			  gs_param_string_array *);
int param_write_name_array(gs_param_list *, gs_param_name,
			   const gs_param_string_array *);

/*
 * Define an abstract parameter list.  Implementations are concrete
 * subclasses.
 *
 * The persisent_keys flag allows for both statically and dynamically
 * allocated keys.  The default is static (the keys are normally C string
 * literals).
 */
#define gs_param_list_common\
    const gs_param_list_procs *procs;\
    gs_memory_t *memory;	/* for allocating coerced arrays */\
    bool persistent_keys
struct gs_param_list_s {
    gs_param_list_common;
};

/* Set whether the keys for param_write_XXX are persistent. */
/* VMS limits procedure names to 31 characters. */
#define gs_param_list_set_persistent_keys gs_param_list_set_persist_keys
void gs_param_list_set_persistent_keys(gs_param_list *, bool);

/* Initialize a parameter list key enumerator. */
void param_init_enumerator(gs_param_enumerator_t * penum);

/*
 * The following interface provides a convenient way to read and set
 * collections of parameters of any type other than dictionaries.
 */

typedef struct gs_param_item_s {
    const char *key;
    byte /*gs_param_type */ type;
    short offset;		/* offset of value in structure */
} gs_param_item_t;
#define gs_param_item_end { 0 }	/* list terminator */
/*
 * Transfer a collection of parameters.
 * For param_write_items, if a parameter value is equal to the value in
 * the optional default_obj, the item isn't transferred.
 */
int gs_param_read_items(gs_param_list * plist, void *obj,
			const gs_param_item_t * items);
int gs_param_write_items(gs_param_list * plist, const void *obj,
			 const void *default_obj,
			 const gs_param_item_t * items);

/* Internal procedure to initialize the common part of a parameter list. */
void gs_param_list_init(gs_param_list *, const gs_param_list_procs *,
			gs_memory_t *);

/*
 * Internal procedure to read a value, with coercion if requested, needed,
 * and possible.  If mem != 0, we can coerce int arrays to float arrays, and
 * possibly do other coercions later.
 */
int param_coerce_typed(gs_param_typed_value * pvalue,
		       gs_param_type req_type, gs_memory_t * mem);

/* ---------------- Default implementation ---------------- */

/*
 * Provide default generic implementations of param_request and
 * param_requested.
 */
param_proc_request(gs_param_request_default);  /* does nothing */
param_proc_requested(gs_param_requested_default);  /* always returns true */

/*
 * Define a default implementation, intended to be usable easily
 * from C code.  The intended usage pattern is:
	gs_c_param_list list;
	[... other code here ...]
	gs_c_param_list_write(&list, mem);
	[As many as needed:]
	code = param_write_XXX(&list, "ParamName", &param_value);
	[Check code for <0]
	gs_c_param_list_read(&list);
	code = gs_putdeviceparams(dev, &list);
	gs_c_param_list_release(&list);
	[Check code for <0]
	if ( code == 1 )
	{
	    code = (*dev_proc(dev, open_device))(dev);
	    [Check code for <0]
	}
 *
 * This implementation also has the special property that it can forward
 * unrecognized param_read_ calls to another parameter list, called the
 * target.  This allows constructing incrementally modified parameter lists.
 * Note that this is only relevant for put_params (reading from the
 * parameter list).
 */

typedef struct gs_c_param_s gs_c_param;	/* opaque here */
typedef struct gs_c_param_list_s {
    gs_param_list_common;
    gs_c_param *head;
    gs_param_list *target;
    uint count;
    bool any_requested;
    gs_param_collection_type_t coll_type;
} gs_c_param_list;
#define private_st_c_param_list()	/* in gsparam.c */\
  gs_private_st_ptrs2(st_c_param_list, gs_c_param_list, "c_param_list",\
    c_param_list_enum_ptrs, c_param_list_reloc_ptrs, head, target)

/* Define a GC descriptor for gs_param_string. */
/* This structure descriptor is only for non persistent gs_param_strings. */
#define private_st_gs_param_string()	/* in gdevdevn.c */\
  gs_private_st_composite(st_gs_param_string, gs_param_string, "gs_param_string",\
			param_string_enum_ptrs, param_string_reloc_ptrs)

/* Set the target of a C parameter list. */
void gs_c_param_list_set_target(gs_c_param_list *, gs_param_list *);

/*
 * Clients normally allocate the gs_c_param_list on the stack, but we
 * provide a procedure for allocating one in memory.
 */
gs_c_param_list *gs_c_param_list_alloc(gs_memory_t *, client_name_t);
void gs_c_param_list_write(gs_c_param_list *, gs_memory_t *);
void gs_c_param_list_write_more(gs_c_param_list *); /* switch back to writing, no init */
void gs_c_param_list_read(gs_c_param_list *);	/* switch to reading */
void gs_c_param_list_release(gs_c_param_list *);

#endif /* gsparam_INCLUDED */
