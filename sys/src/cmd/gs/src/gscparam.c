/* Copyright (C) 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gscparam.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Default implementation of parameter lists */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gsstruct.h"

/* Forward references */
typedef union c_param_value_s {
    GS_PARAM_VALUE_UNION(gs_c_param_list);
} gs_c_param_value;
/*typedef struct gs_c_param_s gs_c_param; *//* in gsparam.h */

/* Define the GC type for a parameter list. */
private_st_c_param_list();

/* Lengths corresponding to various gs_param_type_xxx types */
const byte gs_param_type_sizes[] = {
    GS_PARAM_TYPE_SIZES(sizeof(gs_c_param_list))
};

/* Lengths of *actual* data-containing type pointed to or contained by gs_param_type_xxx's */
const byte gs_param_type_base_sizes[] = {
    GS_PARAM_TYPE_BASE_SIZES(0)
};

/*
 * Define a parameter list element.  We use gs_param_type_any to identify
 * elements that have been requested but not yet written.  The reading
 * procedures must recognize such elements as undefined, and ignore them.
 */
struct gs_c_param_s {
    gs_c_param *next;
    gs_param_name key;
    gs_c_param_value value;
    gs_param_type type;
    void *alternate_typed_data;
};

/* Parameter values aren't really simple, */
/* but since parameter lists are transient, it doesn't matter. */
gs_private_st_ptrs2(st_c_param, gs_c_param, "gs_c_param",
	 c_param_enum_ptrs, c_param_reloc_ptrs, next, alternate_typed_data);

/* ---------------- Utilities ---------------- */

private gs_c_param *
c_param_find(const gs_c_param_list *plist, gs_param_name pkey, bool any)
{
    gs_c_param *pparam = plist->head;

    for (; pparam != 0; pparam = pparam->next)
	if (!strcmp(pparam->key, pkey))
	    return (pparam->type != gs_param_type_any || any ? pparam : 0);
    return 0;
}

/* ---------------- Writing parameters to a list ---------------- */

private param_proc_begin_xmit_collection(c_param_begin_write_collection);
private param_proc_end_xmit_collection(c_param_end_write_collection);
private param_proc_xmit_typed(c_param_write_typed);
private param_proc_request(c_param_request);
private param_proc_requested(c_param_requested);
private const gs_param_list_procs c_write_procs =
{
    c_param_write_typed,
    c_param_begin_write_collection,
    c_param_end_write_collection,
    NULL,			/* get_next_key */
    c_param_request,
    c_param_requested
};

/* Initialize a list for writing. */
void
gs_c_param_list_write(gs_c_param_list * plist, gs_memory_t * mem)
{
    plist->memory = mem;
    plist->head = 0;
    plist->target = 0;		/* not used for writing */
    plist->count = 0;
    plist->any_requested = false;
    gs_c_param_list_write_more(plist);
}

/* Set the target of a list.  Only relevant for reading. */
void
gs_c_param_list_set_target(gs_c_param_list *plist, gs_param_list *target)
{
    plist->target = target;
}

/* Re-enable a list for writing, without clearing it. */
/* gs_c_param_list_write must have been called previously. */
void
gs_c_param_list_write_more(gs_c_param_list * plist)
{
    plist->procs = &c_write_procs;
    plist->coll_type = gs_param_collection_dict_any;
}

/* Release a list. */
void
gs_c_param_list_release(gs_c_param_list * plist)
{
    gs_c_param *pparam;

    while ((pparam = plist->head) != 0) {
	gs_c_param *next = pparam->next;

	switch (pparam->type) {
	    case gs_param_type_dict:
	    case gs_param_type_dict_int_keys:
	    case gs_param_type_array:
		gs_c_param_list_release(&pparam->value.d);
		break;
	    case gs_param_type_string:
	    case gs_param_type_name:
	    case gs_param_type_int_array:
	    case gs_param_type_float_array:
	    case gs_param_type_string_array:
	    case gs_param_type_name_array:
		if (!pparam->value.s.persistent)
		    gs_free_const_object(plist->memory,
					 pparam->value.s.data,
					 "gs_c_param_list_release data");
		break;
	    default:
		break;
	}
	gs_free_object(plist->memory, pparam->alternate_typed_data,
		       "gs_c_param_list_release alternate data");
	gs_free_object(plist->memory, pparam,
		       "gs_c_param_list_release entry");
	plist->head = next;
	plist->count--;
    }
}

/* Add an entry to a list.  Doesn't set: value, type, plist->head. */
private gs_c_param *
c_param_add(gs_c_param_list * plist, gs_param_name pkey)
{
    gs_c_param *pparam =
	gs_alloc_struct(plist->memory, gs_c_param, &st_c_param,
			"c_param_write entry");

    if (pparam == 0)
	return 0;
    pparam->next = plist->head;
    pparam->key = pkey;
    pparam->alternate_typed_data = 0;
    return pparam;
}

/*  Write a dynamically typed parameter to a list. */
private int
c_param_write(gs_c_param_list * plist, gs_param_name pkey, void *pvalue,
	      gs_param_type type)
{
    unsigned top_level_sizeof = 0;
    unsigned second_level_sizeof = 0;
    gs_c_param *pparam = c_param_add(plist, pkey);

    if (pparam == 0)
	return_error(gs_error_VMerror);
    memcpy(&pparam->value, pvalue, gs_param_type_sizes[(int)type]);
    pparam->type = type;

    /* Need deeper copies of data if it's not persistent */
    switch (type) {
	    gs_param_string const *curr_string;
	    gs_param_string const *end_string;

	case gs_param_type_string_array:
	case gs_param_type_name_array:
	    /* Determine how much mem needed to hold actual string data */
	    curr_string = pparam->value.sa.data;
	    end_string = curr_string + pparam->value.sa.size;
	    for (; curr_string < end_string; ++curr_string)
		if (!curr_string->persistent)
		    second_level_sizeof += curr_string->size;
	    /* fall thru */

	case gs_param_type_string:
	case gs_param_type_name:
	case gs_param_type_int_array:
	case gs_param_type_float_array:
	    if (!pparam->value.s.persistent) {	/* Allocate & copy object pointed to by array or string */
		byte *top_level_memory;

		top_level_sizeof =
		    pparam->value.s.size * gs_param_type_base_sizes[type];
		top_level_memory =
		    gs_alloc_bytes_immovable(plist->memory,
				     top_level_sizeof + second_level_sizeof,
					     "c_param_write data");
		if (top_level_memory == 0) {
		    gs_free_object(plist->memory, pparam, "c_param_write entry");
		    return_error(gs_error_VMerror);
		}
		memcpy(top_level_memory, pparam->value.s.data, top_level_sizeof);
		pparam->value.s.data = top_level_memory;

		/* String/name arrays need to copy actual str data */

		if (second_level_sizeof > 0) {
		    byte *second_level_memory =
		    top_level_memory + top_level_sizeof;

		    curr_string = pparam->value.sa.data;
		    end_string = curr_string + pparam->value.sa.size;
		    for (; curr_string < end_string; ++curr_string)
			if (!curr_string->persistent) {
			    memcpy(second_level_memory,
				   curr_string->data, curr_string->size);
			    ((gs_param_string *) curr_string)->data
				= second_level_memory;
			    second_level_memory += curr_string->size;
			}
		}
	    }
	    break;
	default:
	    break;
    }

    plist->head = pparam;
    plist->count++;
    return 0;
}

/* Individual writing routines. */
private int
c_param_begin_write_collection(gs_param_list * plist, gs_param_name pkey,
	       gs_param_dict * pvalue, gs_param_collection_type_t coll_type)
{
    gs_c_param_list *const cplist = (gs_c_param_list *)plist;
    gs_c_param_list *dlist =
	gs_alloc_struct(cplist->memory, gs_c_param_list, &st_c_param_list,
			"c_param_begin_write_collection");

    if (dlist == 0)
	return_error(gs_error_VMerror);
    gs_c_param_list_write(dlist, cplist->memory);
    dlist->coll_type = coll_type;
    pvalue->list = (gs_param_list *) dlist;
    return 0;
}
private int
c_param_end_write_collection(gs_param_list * plist, gs_param_name pkey,
			     gs_param_dict * pvalue)
{
    gs_c_param_list *const cplist = (gs_c_param_list *)plist;
    gs_c_param_list *dlist = (gs_c_param_list *) pvalue->list;

    return c_param_write(cplist, pkey, pvalue->list,
		    (dlist->coll_type == gs_param_collection_dict_int_keys ?
		     gs_param_type_dict_int_keys :
		     dlist->coll_type == gs_param_collection_array ?
		     gs_param_type_array : gs_param_type_dict));
}
private int
c_param_write_typed(gs_param_list * plist, gs_param_name pkey,
		    gs_param_typed_value * pvalue)
{
    gs_c_param_list *const cplist = (gs_c_param_list *)plist;
    gs_param_collection_type_t coll_type;

    switch (pvalue->type) {
	case gs_param_type_dict:
	    coll_type = gs_param_collection_dict_any;
	    break;
	case gs_param_type_dict_int_keys:
	    coll_type = gs_param_collection_dict_int_keys;
	    break;
	case gs_param_type_array:
	    coll_type = gs_param_collection_array;
	    break;
	default:
	    return c_param_write(cplist, pkey, &pvalue->value, pvalue->type);
    }
    return c_param_begin_write_collection
	(plist, pkey, &pvalue->value.d, coll_type);
}

/* Other procedures */

private int
c_param_request(gs_param_list * plist, gs_param_name pkey)
{
    gs_c_param_list *const cplist = (gs_c_param_list *)plist;
    gs_c_param *pparam;

    cplist->any_requested = true;
    if (c_param_find(cplist, pkey, true))
	return 0;
    pparam = c_param_add(cplist, pkey);
    if (pparam == 0)
	return_error(gs_error_VMerror);
    pparam->type = gs_param_type_any; /* mark as undefined */
    cplist->head = pparam;
    return 0;
}

private int
c_param_requested(const gs_param_list * plist, gs_param_name pkey)
{
    const gs_c_param_list *const cplist = (const gs_c_param_list *)plist;
    gs_param_list *target = cplist->target;
    int code;

    if (!cplist->any_requested)
	return (target ? param_requested(target, pkey) : -1);
    if (c_param_find(cplist, pkey, true) != 0)
	return 1;
    if (!target)
	return 0;
    code = param_requested(target, pkey);
    return (code < 0 ? 0 : 1);
}

/* ---------------- Reading from a list to parameters ---------------- */

private param_proc_begin_xmit_collection(c_param_begin_read_collection);
private param_proc_end_xmit_collection(c_param_end_read_collection);
private param_proc_xmit_typed(c_param_read_typed);
private param_proc_next_key(c_param_get_next_key);
private param_proc_get_policy(c_param_read_get_policy);
private param_proc_signal_error(c_param_read_signal_error);
private param_proc_commit(c_param_read_commit);
private const gs_param_list_procs c_read_procs =
{
    c_param_read_typed,
    c_param_begin_read_collection,
    c_param_end_read_collection,
    c_param_get_next_key,
    NULL,			/* request, N/A */
    NULL,			/* requested, N/A */
    c_param_read_get_policy,
    c_param_read_signal_error,
    c_param_read_commit
};

/* Switch a list from writing to reading. */
void
gs_c_param_list_read(gs_c_param_list * plist)
{
    plist->procs = &c_read_procs;
}

/* Generic routine for reading a parameter from a list. */

private int
c_param_read_typed(gs_param_list * plist, gs_param_name pkey,
		   gs_param_typed_value * pvalue)
{
    gs_c_param_list *const cplist = (gs_c_param_list *)plist;
    gs_param_type req_type = pvalue->type;
    gs_c_param *pparam = c_param_find(cplist, pkey, false);
    int code;

    if (pparam == 0)
	return (cplist->target ?
		param_read_typed(cplist->target, pkey, pvalue) : 1);
    pvalue->type = pparam->type;
    switch (pvalue->type) {
	case gs_param_type_dict:
	case gs_param_type_dict_int_keys:
	case gs_param_type_array:
	    gs_c_param_list_read(&pparam->value.d);
	    pvalue->value.d.list = (gs_param_list *) & pparam->value.d;
	    pvalue->value.d.size = pparam->value.d.count;
	    return 0;
	default:
	    break;
    }
    memcpy(&pvalue->value, &pparam->value,
	   gs_param_type_sizes[(int)pparam->type]);
    code = param_coerce_typed(pvalue, req_type, NULL);
/****** SHOULD LET param_coerce_typed DO THIS ******/
    if (code == gs_error_typecheck &&
	req_type == gs_param_type_float_array &&
	pvalue->type == gs_param_type_int_array
	) {
	/* Convert int array to float dest */
	gs_param_float_array fa;
	int element;

	fa.size = pparam->value.ia.size;
	fa.persistent = false;

	if (pparam->alternate_typed_data == 0) {
	    if ((pparam->alternate_typed_data
		 = (void *)gs_alloc_bytes_immovable(cplist->memory,
						    fa.size * sizeof(float),
			     "gs_c_param_read alternate float array")) == 0)
		      return_error(gs_error_VMerror);

	    for (element = 0; element < fa.size; ++element)
		((float *)(pparam->alternate_typed_data))[element]
		    = (float)pparam->value.ia.data[element];
	}
	fa.data = (float *)pparam->alternate_typed_data;

	pvalue->value.fa = fa;
	return 0;
    }
    return code;
}

/* Individual reading routines. */
private int
c_param_begin_read_collection(gs_param_list * plist, gs_param_name pkey,
	       gs_param_dict * pvalue, gs_param_collection_type_t coll_type)
{
    gs_c_param_list *const cplist = (gs_c_param_list *)plist;
    gs_c_param *pparam = c_param_find(cplist, pkey, false);

    if (pparam == 0)
	return
	    (cplist->target ?
	     param_begin_read_collection(cplist->target,
					 pkey, pvalue, coll_type) :
	     1);
    switch (pparam->type) {
	case gs_param_type_dict:
	    if (coll_type != gs_param_collection_dict_any)
		return_error(gs_error_typecheck);
	    break;
	case gs_param_type_dict_int_keys:
	    if (coll_type == gs_param_collection_array)
		return_error(gs_error_typecheck);
	    break;
	case gs_param_type_array:
	    break;
	default:
	    return_error(gs_error_typecheck);
    }
    gs_c_param_list_read(&pparam->value.d);
    pvalue->list = (gs_param_list *) & pparam->value.d;
    pvalue->size = pparam->value.d.count;
    return 0;
}
private int
c_param_end_read_collection(gs_param_list * plist, gs_param_name pkey,
			    gs_param_dict * pvalue)
{
    return 0;
}

/* Other procedures */
private int			/* ret 0 ok, 1 if EOF, or -ve err */
c_param_get_next_key(gs_param_list * plist, gs_param_enumerator_t * penum,
		     gs_param_key_t * key)
{
    gs_c_param_list *const cplist = (gs_c_param_list *)plist;
    gs_c_param *pparam =
    (penum->pvoid ? ((gs_c_param *) (penum->pvoid))->next :
     cplist->head);

    if (pparam == 0)
	return 1;
    penum->pvoid = pparam;
    key->data = (const byte *)pparam->key;	/* was const char * */
    key->size = strlen(pparam->key);
    return 0;
}
private int
c_param_read_get_policy(gs_param_list * plist, gs_param_name pkey)
{
    return gs_param_policy_ignore;
}
private int
c_param_read_signal_error(gs_param_list * plist, gs_param_name pkey, int code)
{
    return code;
}
private int
c_param_read_commit(gs_param_list * plist)
{
    return 0;
}
