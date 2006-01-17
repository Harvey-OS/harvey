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

/* $Id: iparam.c,v 1.10 2004/08/04 19:36:13 stefan Exp $ */
/* Interpreter implementations of parameter dictionaries */
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "ierrors.h"
#include "oper.h"		/* for check_type */
#include "opcheck.h"
#include "ialloc.h"
#include "idict.h"
#include "imemory.h"		/* for iutil.h */
#include "iname.h"
#include "istack.h"
#include "iparam.h"
#include "iutil.h"		/* for num_params */
#include "ivmspace.h"
#include "store.h"

/* ================ Utilities ================ */

/* Convert a key to a ref. */
private int
ref_param_key(const iparam_list * plist, gs_param_name pkey, ref * pkref)
{
    if (plist->int_keys) {
	long key;

	if (sscanf(pkey, "%ld", &key) != 1)
	    return_error(e_rangecheck);
	make_int(pkref, key);
	return 0;
    } else
        return name_ref(plist->memory, (const byte *)pkey, strlen(pkey), pkref, 0);
}

/* Fill in a gs_param_key_t from a name or int ref. */
private int
ref_to_key(const ref * pref, gs_param_key_t * key, iparam_list *plist)
{
    if (r_has_type(pref, t_name)) {
	ref nref;

	name_string_ref(plist->memory, pref, &nref);
	key->data = nref.value.const_bytes;
	key->size = r_size(&nref);
	key->persistent = false; /* names may be freed */
    } else if (r_has_type(pref, t_integer)) {
	char istr[sizeof(long) * 8 / 3 + 2];
	int len;
	byte *buf;

	sprintf(istr, "%ld", pref->value.intval);
	len = strlen(istr);
	/* GC will take care of freeing this: */
	buf = gs_alloc_string(plist->memory, len, "ref_to_key");
	if (!buf)
	    return_error(e_VMerror);
	key->data = buf;
	key->size = len;
	key->persistent = true;
    } else
	return_error(e_typecheck);
    return 0;
}

/* ================ Writing parameters to refs ================ */

/* Forward references */
private int array_new_indexed_plist_write(dict_param_list *plist,
					  ref *parray, const ref *pwanted,
					  gs_ref_memory_t *imem);

/* ---------------- Generic writing procedures ---------------- */

private param_proc_begin_xmit_collection(ref_param_begin_write_collection);
private param_proc_end_xmit_collection(ref_param_end_write_collection);
private param_proc_xmit_typed(ref_param_write_typed);
private param_proc_next_key(ref_param_get_next_key);
private param_proc_requested(ref_param_requested);
private const gs_param_list_procs ref_write_procs =
{
    ref_param_write_typed,
    ref_param_begin_write_collection,
    ref_param_end_write_collection,
    ref_param_get_next_key,
    NULL,			/* request */
    ref_param_requested
};
private int ref_array_param_requested(const iparam_list *, gs_param_name,
				      ref *, uint, client_name_t);
private int ref_param_write(iparam_list *, gs_param_name, const ref *);
private int ref_param_write_string_value(ref *, const gs_param_string *,
					 gs_ref_memory_t *);
private int ref_param_write_name_value(const gs_memory_t *mem, ref *, const gs_param_string *);
private int
ref_param_make_int(ref *pe, const void *pvalue, uint i, gs_ref_memory_t *imem)
{
    make_tav(pe, t_integer, imemory_new_mask(imem), intval,
	     ((const gs_param_int_array *)pvalue)->data[i]);
    return 0;
}
private int
ref_param_make_float(ref *pe, const void *pvalue, uint i, gs_ref_memory_t *imem)
{
    make_tav(pe, t_real, imemory_new_mask(imem), realval,
	     ((const gs_param_float_array *)pvalue)->data[i]);
    return 0;
}
private int
ref_param_make_string(ref *pe, const void *pvalue, uint i, gs_ref_memory_t *imem)
{
    return ref_param_write_string_value(pe,
			 &((const gs_param_string_array *)pvalue)->data[i],
					imem);
}
private int
ref_param_make_name(ref * pe, const void *pvalue, uint i, gs_ref_memory_t *imem)
{
    return ref_param_write_name_value((const gs_memory_t *)imem, pe,
			 &((const gs_param_string_array *)pvalue)->data[i]);
}
private int
ref_param_write_typed_array(gs_param_list * plist, gs_param_name pkey,
			    void *pvalue, uint count,
			    int (*make)(ref *, const void *, uint,
					gs_ref_memory_t *))
{
    iparam_list *const iplist = (iparam_list *) plist;
    ref value;
    uint i;
    ref *pe;
    int code;

    if ((code = ref_array_param_requested(iplist, pkey, &value, count,
				       "ref_param_write_typed_array")) <= 0)
	return code;
    for (i = 0, pe = value.value.refs; i < count; ++i, ++pe)
	if ((code = (*make) (pe, pvalue, i, iplist->ref_memory)) < 0)
	    return code;
    return ref_param_write(iplist, pkey, &value);
}
private int
ref_param_begin_write_collection(gs_param_list * plist, gs_param_name pkey,
				 gs_param_dict * pvalue,
				 gs_param_collection_type_t coll_type)
{
    iparam_list *const iplist = (iparam_list *) plist;
    gs_ref_memory_t *imem = iplist->ref_memory;
    dict_param_list *dlist = (dict_param_list *)
	gs_alloc_bytes(plist->memory, size_of(dict_param_list),
		       "ref_param_begin_write_collection");
    int code;

    if (dlist == 0)
	return_error(e_VMerror);
    if (coll_type != gs_param_collection_array) {
	ref dref;

	code = dict_alloc(imem, pvalue->size, &dref);
	if (code >= 0) {
	    code = dict_param_list_write(dlist, &dref, NULL, imem);
	    dlist->int_keys = coll_type == gs_param_collection_dict_int_keys;
	}
    } else {
	ref aref;

	code = gs_alloc_ref_array(imem, &aref, a_all, pvalue->size,
				  "ref_param_begin_write_collection");
	if (code >= 0)
	    code = array_new_indexed_plist_write(dlist, &aref, NULL, imem);
    }
    if (code < 0)
	gs_free_object(plist->memory, dlist, "ref_param_begin_write_collection");
    else
	pvalue->list = (gs_param_list *) dlist;
    return code;
}
private int
ref_param_end_write_collection(gs_param_list * plist, gs_param_name pkey,
			       gs_param_dict * pvalue)
{
    iparam_list *const iplist = (iparam_list *) plist;
    int code = ref_param_write(iplist, pkey,
			       &((dict_param_list *) pvalue->list)->dict);

    gs_free_object(plist->memory, pvalue->list, "ref_param_end_write_collection");
    return code;
}
private int
ref_param_write_typed(gs_param_list * plist, gs_param_name pkey,
		      gs_param_typed_value * pvalue)
{
    iparam_list *const iplist = (iparam_list *) plist;
    ref value;
    int code = 0;

    switch (pvalue->type) {
	case gs_param_type_null:
	    make_null(&value);
	    break;
	case gs_param_type_bool:
	    make_bool(&value, pvalue->value.b);
	    break;
	case gs_param_type_int:
	    make_int(&value, pvalue->value.i);
	    break;
	case gs_param_type_long:
	    make_int(&value, pvalue->value.l);
	    break;
	case gs_param_type_float:
	    make_real(&value, pvalue->value.f);
	    break;
	case gs_param_type_string:
	    if (!ref_param_requested(plist, pkey))
		return 0;
	    code = ref_param_write_string_value(&value, &pvalue->value.s,
						iplist->ref_memory);
	    break;
	case gs_param_type_name:
	    if (!ref_param_requested(plist, pkey))
		return 0;
	    code = ref_param_write_name_value(iplist->memory, &value, &pvalue->value.n);
	    break;
	case gs_param_type_int_array:
	    return ref_param_write_typed_array(plist, pkey, &pvalue->value.ia,
					       pvalue->value.ia.size,
					       ref_param_make_int);
	case gs_param_type_float_array:
	    return ref_param_write_typed_array(plist, pkey, &pvalue->value.fa,
					       pvalue->value.fa.size,
					       ref_param_make_float);
	case gs_param_type_string_array:
	    return ref_param_write_typed_array(plist, pkey, &pvalue->value.sa,
					       pvalue->value.sa.size,
					       ref_param_make_string);
	case gs_param_type_name_array:
	    return ref_param_write_typed_array(plist, pkey, &pvalue->value.na,
					       pvalue->value.na.size,
					       ref_param_make_name);
	case gs_param_type_dict:
	case gs_param_type_dict_int_keys:
	case gs_param_type_array:
	    return ref_param_begin_write_collection(plist, pkey,
						    &pvalue->value.d,
	      (gs_param_collection_type_t)(pvalue->type - gs_param_type_dict));
	default:
	    return_error(e_typecheck);
    }
    if (code < 0)
	return code;
    return ref_param_write(iplist, pkey, &value);
}

/* Check whether a given parameter was requested. */
private int
ref_param_requested(const gs_param_list * plist, gs_param_name pkey)
{
    const iparam_list *const ciplist = (const iparam_list *)plist;
    ref kref;
    ref *ignore_value;

    if (!r_has_type(&ciplist->u.w.wanted, t_dictionary))
	return -1;
    if (ref_param_key(ciplist, pkey, &kref) < 0)
	return -1;		/* catch it later */
    return (dict_find(&ciplist->u.w.wanted, &kref, &ignore_value) > 0);
}

/* Check whether an array parameter is wanted, and allocate it if so. */
/* Return <0 on error, 0 if not wanted, 1 if wanted. */
private int
ref_array_param_requested(const iparam_list *iplist, gs_param_name pkey,
			  ref *pvalue, uint size, client_name_t cname)
{
    int code;

    if (!ref_param_requested((const gs_param_list *)iplist, pkey))
	return 0;
    code = gs_alloc_ref_array(iplist->ref_memory, pvalue, a_all, size, cname);
    return (code < 0 ? code : 1);
}

/* ---------------- Internal routines ---------------- */

/* Prepare to write a string value. */
private int
ref_param_write_string_value(ref * pref, const gs_param_string * pvalue,
			     gs_ref_memory_t *imem)
{
    const byte *pdata = pvalue->data;
    uint n = pvalue->size;

    if (pvalue->persistent)
	make_const_string(pref, a_readonly | avm_foreign, n, pdata);
    else {
	byte *pstr = gs_alloc_string((gs_memory_t *)imem, n,
				     "ref_param_write_string");

	if (pstr == 0)
	    return_error(e_VMerror);
	memcpy(pstr, pdata, n);
	make_string(pref, a_readonly | imemory_space(imem), n, pstr);
    }
    return 0;
}

/* Prepare to write a name value. */
private int
ref_param_write_name_value(const gs_memory_t *mem, ref * pref, const gs_param_string * pvalue)
{
    return name_ref(mem, pvalue->data, pvalue->size, pref,
		    (pvalue->persistent ? 0 : 1));
}

/* Generic routine for writing a ref parameter. */
private int
ref_param_write(iparam_list * plist, gs_param_name pkey, const ref * pvalue)
{
    ref kref;
    int code;

    if (!ref_param_requested((gs_param_list *) plist, pkey))
	return 0;
    code = ref_param_key(plist, pkey, &kref);
    if (code < 0)
	return code;
    return (*plist->u.w.write) (plist, &kref, pvalue);
}

/* ---------------- Implementations ---------------- */

/* Initialize for writing parameters. */
private void
ref_param_write_init(iparam_list * plist, const ref * pwanted,
		     gs_ref_memory_t *imem)
{
    gs_param_list_init((gs_param_list *)plist, &ref_write_procs,
		       (gs_memory_t *)imem);
    plist->ref_memory = imem;
    if (pwanted == 0)
	make_null(&plist->u.w.wanted);
    else
	plist->u.w.wanted = *pwanted;
    plist->results = 0;
    plist->int_keys = false;
}

/* Implementation for getting parameters to a stack. */
private int
stack_param_write(iparam_list * plist, const ref * pkey, const ref * pvalue)
{
    stack_param_list *const splist = (stack_param_list *) plist;
    ref_stack_t *pstack = splist->pstack;
    s_ptr p = pstack->p;

    if (pstack->top - p < 2) {
	int code = ref_stack_push(pstack, 2);

	if (code < 0)
	    return code;
	*ref_stack_index(pstack, 1) = *pkey;
	p = pstack->p;
    } else {
	pstack->p = p += 2;
	p[-1] = *pkey;
    }
    *p = *pvalue;
    splist->count++;
    return 0;
}

/* Implementation for enumerating parameters on a stack */
private int			/* ret 0 ok, 1 if EOF, or -ve err */
stack_param_enumerate(iparam_list * plist, gs_param_enumerator_t * penum,
		      gs_param_key_t * key, ref_type * type)
{
    int code;
    stack_param_list *const splist = (stack_param_list *) plist;
    long index = penum->intval;
    ref *stack_element;

    do {
	stack_element =
	    ref_stack_index(splist->pstack, index + 1 + splist->skip);
	if (!stack_element)
	    return 1;
    } while (index += 2, !r_has_type(stack_element, t_name));
    *type = r_type(stack_element);
    code = ref_to_key(stack_element, key, plist);
    penum->intval = index;
    return code;
}

int
stack_param_list_write(stack_param_list * plist, ref_stack_t * pstack,
		       const ref * pwanted, gs_ref_memory_t *imem)
{
    plist->u.w.write = stack_param_write;
    ref_param_write_init((iparam_list *) plist, pwanted, imem);
    plist->enumerate = stack_param_enumerate;
    plist->pstack = pstack;
    plist->skip = 0;
    plist->count = 0;
    return 0;
}

/* Implementation for getting parameters to a dictionary. */
private int
dict_param_write(iparam_list * plist, const ref * pkey, const ref * pvalue)
{
    int code =
	dict_put(&((dict_param_list *) plist)->dict, pkey, pvalue, NULL);

    return min(code, 0);
}

/* Implementation for enumerating parameters in a dictionary */
private int			/* ret 0 ok, 1 if EOF, or -ve err */
dict_param_enumerate(iparam_list * plist, gs_param_enumerator_t * penum,
		     gs_param_key_t * key, ref_type * type)
{
    ref elt[2];
    int code;
    dict_param_list *const pdlist = (dict_param_list *) plist;
    int index =
    (penum->intval != 0 ? penum->intval : dict_first(&pdlist->dict));

    index = dict_next(&pdlist->dict, index, elt);
    if (index < 0)
	return 1;
    *type = r_type(&elt[1]);
    code = ref_to_key(&elt[0], key, plist);
    penum->intval = index;
    return code;
}

int
dict_param_list_write(dict_param_list *plist, ref *pdict, const ref *pwanted,
		      gs_ref_memory_t *imem)
{
    check_dict_write(*pdict);
    plist->u.w.write = dict_param_write;
    plist->enumerate = dict_param_enumerate;
    ref_param_write_init((iparam_list *) plist, pwanted, imem);
    plist->dict = *pdict;
    return 0;
}

/* Implementation for getting parameters to an indexed array. */
/* Note that this is now internal, since it only handles "new" arrays. */
private int
array_new_indexed_param_write(iparam_list * iplist, const ref * pkey,
			  const ref * pvalue)
{
    const ref *const arr = &((dict_param_list *)iplist)->dict;
    ref *eltp;

    if (!r_has_type(pkey, t_integer))
	return_error(e_typecheck);
    check_int_ltu(*pkey, r_size(arr));
    store_check_dest(arr, pvalue);
    eltp = arr->value.refs + pkey->value.intval;
    /* ref_assign_new(eltp, pvalue); */
    ref_assign(eltp, pvalue);
    r_set_attrs(eltp, imemory_new_mask(iplist->ref_memory));
    return 0;
}
private int
array_new_indexed_plist_write(dict_param_list * plist, ref * parray,
			      const ref * pwanted, gs_ref_memory_t *imem)
{
    check_array(*parray);
    check_write(*parray);
    plist->u.w.write = array_new_indexed_param_write;
    ref_param_write_init((iparam_list *) plist, pwanted, imem);
    plist->dict = *parray;
    plist->int_keys = true;
    return 0;
}

/* ================ Reading refs to parameters ================ */

/* ---------------- Generic reading procedures ---------------- */

private param_proc_begin_xmit_collection(ref_param_begin_read_collection);
private param_proc_end_xmit_collection(ref_param_end_read_collection);
private param_proc_xmit_typed(ref_param_read_typed);

/*private param_proc_next_key(ref_param_get_next_key); already dec'ld above */
private param_proc_get_policy(ref_param_read_get_policy);
private param_proc_signal_error(ref_param_read_signal_error);
private param_proc_commit(ref_param_read_commit);
private const gs_param_list_procs ref_read_procs =
{
    ref_param_read_typed,
    ref_param_begin_read_collection,
    ref_param_end_read_collection,
    ref_param_get_next_key,
    NULL,			/* request */
    NULL,			/* requested */
    ref_param_read_get_policy,
    ref_param_read_signal_error,
    ref_param_read_commit
};
private int ref_param_read(iparam_list *, gs_param_name,
			   iparam_loc *, int);
private int ref_param_read_string_value(const gs_memory_t *mem,
					const iparam_loc *,
					gs_param_string *);
private int ref_param_read_array(iparam_list *, gs_param_name,
				 iparam_loc *);

#define iparam_note_error(loc, code)\
  gs_note_error(*(loc).presult = code)
#define iparam_check_type(loc, typ)\
  if ( !r_has_type((loc).pvalue, typ) )\
    return iparam_note_error(loc, e_typecheck)
#define iparam_check_read(loc)\
  if ( !r_has_attr((loc).pvalue, a_read) )\
    return iparam_note_error(loc, e_invalidaccess)

private int
ref_param_read_int_array(gs_param_list * plist, gs_param_name pkey,
			 gs_param_int_array * pvalue)
{
    iparam_list *const iplist = (iparam_list *) plist;
    iparam_loc loc;
    int code = ref_param_read_array(iplist, pkey, &loc);
    int *piv;
    uint size;
    long i;

    if (code != 0)
	return code;
    size = r_size(loc.pvalue);
    piv = (int *)gs_alloc_byte_array(plist->memory, size, sizeof(int),
				     "ref_param_read_int_array");

    if (piv == 0)
	return_error(e_VMerror);
    for (i = 0; i < size; i++) {
	ref elt;

	array_get(plist->memory, loc.pvalue, i, &elt);
	if (!r_has_type(&elt, t_integer)) {
	    code = gs_note_error(e_typecheck);
	    break;
	}
#if arch_sizeof_int < arch_sizeof_long
	if (elt.value.intval != (int)elt.value.intval) {
	    code = gs_note_error(e_rangecheck);
	    break;
	}
#endif
	piv[i] = (int)elt.value.intval;
    }
    if (code < 0) {
	gs_free_object(plist->memory, piv, "ref_param_read_int_array");
	return (*loc.presult = code);
    }
    pvalue->data = piv;
    pvalue->size = size;
    pvalue->persistent = true;
    return 0;
}
private int
ref_param_read_float_array(gs_param_list * plist, gs_param_name pkey,
			   gs_param_float_array * pvalue)
{
    iparam_list *const iplist = (iparam_list *) plist;
    iparam_loc loc;
    ref aref, elt;
    int code = ref_param_read_array(iplist, pkey, &loc);
    float *pfv;
    uint size;
    long i;

    if (code != 0)
	return code;
    size = r_size(loc.pvalue);
    pfv = (float *)gs_alloc_byte_array(plist->memory, size, sizeof(float),
				       "ref_param_read_float_array");

    if (pfv == 0)
	return_error(e_VMerror);
    aref = *loc.pvalue;
    loc.pvalue = &elt;
    for (i = 0; code >= 0 && i < size; i++) {
        array_get(plist->memory, &aref, i, &elt);
	code = float_param(&elt, pfv + i);
    }
    if (code < 0) {
	gs_free_object(plist->memory, pfv, "ref_read_float_array_param");
	return (*loc.presult = code);
    }
    pvalue->data = pfv;
    pvalue->size = size;
    pvalue->persistent = true;
    return 0;
}
private int
ref_param_read_string_array(gs_param_list * plist, gs_param_name pkey,
			    gs_param_string_array * pvalue)
{
    iparam_list *const iplist = (iparam_list *) plist;
    iparam_loc loc;
    ref aref;
    int code = ref_param_read_array(iplist, pkey, &loc);
    gs_param_string *psv;
    uint size;
    long i;

    if (code != 0)
	return code;
    size = r_size(loc.pvalue);
    psv = (gs_param_string *)
	gs_alloc_byte_array(plist->memory, size, sizeof(gs_param_string),
			    "ref_param_read_string_array");
    if (psv == 0)
	return_error(e_VMerror);
    aref = *loc.pvalue;
    if (r_has_type(&aref, t_array)) {
	for (i = 0; code >= 0 && i < size; i++) {
	    loc.pvalue = aref.value.refs + i;
	    code = ref_param_read_string_value(plist->memory, &loc, psv + i);
	}
    } else {
	ref elt;

	loc.pvalue = &elt;
	for (i = 0; code >= 0 && i < size; i++) {
	    array_get(plist->memory, &aref, i, &elt);
	    code = ref_param_read_string_value(plist->memory, &loc, psv + i);
	}
    }
    if (code < 0) {
	gs_free_object(plist->memory, psv, "ref_param_read_string_array");
	return (*loc.presult = code);
    }
    pvalue->data = psv;
    pvalue->size = size;
    pvalue->persistent = true;
    return 0;
}
private int
ref_param_begin_read_collection(gs_param_list * plist, gs_param_name pkey,
				gs_param_dict * pvalue,
				gs_param_collection_type_t coll_type)
{
    iparam_list *const iplist = (iparam_list *) plist;
    iparam_loc loc;
    bool int_keys = coll_type != 0;
    int code = ref_param_read(iplist, pkey, &loc, -1);
    dict_param_list *dlist;

    if (code != 0)
	return code;
    dlist = (dict_param_list *)
	gs_alloc_bytes(plist->memory, size_of(dict_param_list),
		       "ref_param_begin_read_collection");
    if (dlist == 0)
	return_error(e_VMerror);
    if (r_has_type(loc.pvalue, t_dictionary)) {
	code = dict_param_list_read(dlist, loc.pvalue, NULL, false,
				    iplist->ref_memory);
	dlist->int_keys = int_keys;
	if (code >= 0)
	    pvalue->size = dict_length(loc.pvalue);
    } else if (int_keys && r_is_array(loc.pvalue)) {
	code = array_indexed_param_list_read(dlist, loc.pvalue, NULL, false,
					     iplist->ref_memory);
	if (code >= 0)
	    pvalue->size = r_size(loc.pvalue);
    } else
	code = gs_note_error(e_typecheck);
    if (code < 0) {
	gs_free_object(plist->memory, dlist, "ref_param_begin_write_collection");
	return iparam_note_error(loc, code);
    }
    pvalue->list = (gs_param_list *) dlist;
    return 0;
}
private int
ref_param_end_read_collection(gs_param_list * plist, gs_param_name pkey,
			      gs_param_dict * pvalue)
{
    iparam_list_release((dict_param_list *) pvalue->list);
    gs_free_object(plist->memory, pvalue->list,
		   "ref_param_end_read_collection");
    return 0;
}
private int
ref_param_read_typed(gs_param_list * plist, gs_param_name pkey,
		     gs_param_typed_value * pvalue)
{
    iparam_list *const iplist = (iparam_list *) plist;
    iparam_loc loc;
    ref elt;
    int code = ref_param_read(iplist, pkey, &loc, -1);

    if (code != 0)
	return code;
    switch (r_type(loc.pvalue)) {
	case t_array:
	case t_mixedarray:
	case t_shortarray:
	    iparam_check_read(loc);
	    if (r_size(loc.pvalue) <= 0) {
		/* 0-length array; can't get type info */
		pvalue->type = gs_param_type_array;
		pvalue->value.d.list = 0;
		pvalue->value.d.size = 0;
		return 0;
	    }
	    /*
	     * We have to guess at the array type.  First we guess based
	     * on the type of the first element of the array.  If that
	     * fails, we try again with more general types.
	     */
	    array_get(plist->memory, loc.pvalue, 0, &elt);
	    switch (r_type(&elt)) {
		case t_integer:
		    pvalue->type = gs_param_type_int_array;
		    code = ref_param_read_int_array(plist, pkey,
						    &pvalue->value.ia);
		    if (code != e_typecheck)
			return code;
		    /* This might be a float array.  Fall through. */
		    *loc.presult = 0;  /* reset error */
		case t_real:
		    pvalue->type = gs_param_type_float_array;
		    return ref_param_read_float_array(plist, pkey,
						      &pvalue->value.fa);
		case t_string:
		    pvalue->type = gs_param_type_string_array;
		    return ref_param_read_string_array(plist, pkey,
						       &pvalue->value.sa);
		case t_name:
		    pvalue->type = gs_param_type_name_array;
		    return ref_param_read_string_array(plist, pkey,
						       &pvalue->value.na);
		default:
		    break;
	    }
	    return gs_note_error(e_typecheck);
	case t_boolean:
	    pvalue->type = gs_param_type_bool;
	    pvalue->value.b = loc.pvalue->value.boolval;
	    return 0;
	case t_dictionary:
	    code = ref_param_begin_read_collection(plist, pkey,
			    &pvalue->value.d, gs_param_collection_dict_any);
	    if (code < 0)
		return code;
	    pvalue->type = gs_param_type_dict;

	    /* fixup new dict's type & int_keys field if contents have int keys */
	    {
		gs_param_enumerator_t enumr;
		gs_param_key_t key;
		ref_type keytype;

		param_init_enumerator(&enumr);
		if (!(*((iparam_list *) plist)->enumerate)
		    ((iparam_list *) pvalue->value.d.list, &enumr, &key, &keytype)
		    && keytype == t_integer) {
		    ((dict_param_list *) pvalue->value.d.list)->int_keys = 1;
		    pvalue->type = gs_param_type_dict_int_keys;
		}
	    }
	    return 0;
	case t_integer:
	    pvalue->type = gs_param_type_long;
	    pvalue->value.l = loc.pvalue->value.intval;
	    return 0;
	case t_name:
	    pvalue->type = gs_param_type_name;
	    return ref_param_read_string_value(plist->memory, &loc, &pvalue->value.n);
	case t_null:
	    pvalue->type = gs_param_type_null;
	    return 0;
	case t_real:
	    pvalue->value.f = loc.pvalue->value.realval;
	    pvalue->type = gs_param_type_float;
	    return 0;
	case t_string:
	    pvalue->type = gs_param_type_string;
	    return ref_param_read_string_value(plist->memory, &loc, &pvalue->value.s);
	default:
	    break;
    }
    return gs_note_error(e_typecheck);
}

private int
ref_param_read_get_policy(gs_param_list * plist, gs_param_name pkey)
{
    iparam_list *const iplist = (iparam_list *) plist;
    ref *pvalue;

    if (!(r_has_type(&iplist->u.r.policies, t_dictionary) &&
	  dict_find_string(&iplist->u.r.policies, pkey, &pvalue) > 0 &&
	  r_has_type(pvalue, t_integer))
	)
	return gs_param_policy_ignore;
    return (int)pvalue->value.intval;
}
private int
ref_param_read_signal_error(gs_param_list * plist, gs_param_name pkey, int code)
{
    iparam_list *const iplist = (iparam_list *) plist;
    iparam_loc loc;

    ref_param_read(iplist, pkey, &loc, -1);	/* can't fail */
    *loc.presult = code;
    switch (ref_param_read_get_policy(plist, pkey)) {
	case gs_param_policy_ignore:
	    return 0;
	case gs_param_policy_consult_user:
	    return_error(e_configurationerror);
	default:
	    return code;
    }
}
private int
ref_param_read_commit(gs_param_list * plist)
{
    iparam_list *const iplist = (iparam_list *) plist;
    int i;
    int ecode = 0;

    if (!iplist->u.r.require_all)
	return 0;
    /* Check to make sure that all parameters were actually read. */
    for (i = 0; i < iplist->count; ++i)
	if (iplist->results[i] == 0)
	    iplist->results[i] = ecode = gs_note_error(e_undefined);
    return ecode;
}
private int
ref_param_get_next_key(gs_param_list * plist, gs_param_enumerator_t * penum,
		       gs_param_key_t * key)
{
    ref_type keytype;		/* result not needed here */
    iparam_list *const pilist = (iparam_list *) plist;

    return (*pilist->enumerate) (pilist, penum, key, &keytype);
}

/* ---------------- Internal routines ---------------- */

/* Read a string value. */
private int
ref_param_read_string_value(const gs_memory_t *mem, const iparam_loc * ploc, gs_param_string * pvalue)
{
    const ref *pref = ploc->pvalue;

    switch (r_type(pref)) {
	case t_name: {
	    ref nref;

	    name_string_ref(mem, pref, &nref);
	    pvalue->data = nref.value.const_bytes;
	    pvalue->size = r_size(&nref);
	    pvalue->persistent = true;
	}
	    break;
	case t_string:
	    iparam_check_read(*ploc);
	    pvalue->data = pref->value.const_bytes;
	    pvalue->size = r_size(pref);
	    pvalue->persistent = false;
	    break;
	default:
	    return iparam_note_error(*ploc, e_typecheck);
    }
    return 0;
}

/* Read an array (or packed array) parameter. */
private int
ref_param_read_array(iparam_list * plist, gs_param_name pkey, iparam_loc * ploc)
{
    int code = ref_param_read(plist, pkey, ploc, -1);

    if (code != 0)
	return code;
    if (!r_is_array(ploc->pvalue))
	return iparam_note_error(*ploc, e_typecheck);
    iparam_check_read(*ploc);
    return 0;
}

/* Generic routine for reading a ref parameter. */
private int
ref_param_read(iparam_list * plist, gs_param_name pkey, iparam_loc * ploc,
	       int type)
{
    iparam_list *const iplist = (iparam_list *) plist;
    ref kref;
    int code = ref_param_key(plist, pkey, &kref);

    if (code < 0)
	return code;
    code = (*plist->u.r.read) (iplist, &kref, ploc);
    if (code != 0)
	return code;
    if (type >= 0)
	iparam_check_type(*ploc, type);
    return 0;
}

/* ---------------- Implementations ---------------- */

/* Implementation for putting parameters from an empty collection. */
private int
empty_param_read(iparam_list * plist, const ref * pkey, iparam_loc * ploc)
{
    return 1;
}

/* Initialize for reading parameters. */
private int
ref_param_read_init(iparam_list * plist, uint count, const ref * ppolicies,
		    bool require_all, gs_ref_memory_t *imem)
{
    gs_param_list_init((gs_param_list *)plist, &ref_read_procs,
		       (gs_memory_t *)imem);
    plist->ref_memory = imem;
    if (ppolicies == 0)
	make_null(&plist->u.r.policies);
    else
	plist->u.r.policies = *ppolicies;
    plist->u.r.require_all = require_all;
    plist->count = count;
    plist->results = (int *)
	gs_alloc_byte_array(plist->memory, count, sizeof(int),
			    "ref_param_read_init");

    if (plist->results == 0)
	return_error(e_VMerror);
    memset(plist->results, 0, count * sizeof(int));

    plist->int_keys = false;
    return 0;
}

/* Implementation for putting parameters from an indexed array. */
private int
array_indexed_param_read(iparam_list * plist, const ref * pkey, iparam_loc * ploc)
{
    ref *const arr = &((dict_param_list *) plist)->dict;

    check_type(*pkey, t_integer);
    if (pkey->value.intval < 0 || pkey->value.intval >= r_size(arr))
	return 1;
    ploc->pvalue = arr->value.refs + pkey->value.intval;
    ploc->presult = &plist->results[pkey->value.intval];
    *ploc->presult = 1;
    return 0;
}
int
array_indexed_param_list_read(dict_param_list * plist, const ref * parray,
			      const ref * ppolicies, bool require_all,
			      gs_ref_memory_t *ref_memory)
{
    iparam_list *const iplist = (iparam_list *) plist;
    int code;

    check_read_type(*parray, t_array);
    plist->u.r.read = array_indexed_param_read;
    plist->dict = *parray;
    code = ref_param_read_init(iplist, r_size(parray), ppolicies,
			       require_all, ref_memory);
    plist->int_keys = true;
    return code;
}

/* Implementation for putting parameters from an array. */
private int
array_param_read(iparam_list * plist, const ref * pkey, iparam_loc * ploc)
{
    ref *bot = ((array_param_list *) plist)->bot;
    ref *ptr = bot;
    ref *top = ((array_param_list *) plist)->top;

    for (; ptr < top; ptr += 2) {
	if (r_has_type(ptr, t_name) && name_eq(ptr, pkey)) {
	    ploc->pvalue = ptr + 1;
	    ploc->presult = &plist->results[ptr - bot];
	    *ploc->presult = 1;
	    return 0;
	}
    }
    return 1;
}

/* Implementation for enumerating parameters in an array */
private int			/* ret 0 ok, 1 if EOF, or -ve err */
array_param_enumerate(iparam_list * plist, gs_param_enumerator_t * penum,
		      gs_param_key_t * key, ref_type * type)
{
    int index = penum->intval;
    ref *bot = ((array_param_list *) plist)->bot;
    ref *ptr = bot + index;
    ref *top = ((array_param_list *) plist)->top;

    for (; ptr < top; ptr += 2) {
	index += 2;

	if (r_has_type(ptr, t_name)) {
	    int code = ref_to_key(ptr, key, plist);

	    *type = r_type(ptr);
	    penum->intval = index;
	    return code;
	}
    }
    return 1;
}

int
array_param_list_read(array_param_list * plist, ref * bot, uint count,
		      const ref * ppolicies, bool require_all,
		      gs_ref_memory_t *imem)
{
    iparam_list *const iplist = (iparam_list *) plist;

    if (count & 1)
	return_error(e_rangecheck);
    plist->u.r.read = array_param_read;
    plist->enumerate = array_param_enumerate;
    plist->bot = bot;
    plist->top = bot + count;
    return ref_param_read_init(iplist, count, ppolicies, require_all, imem);
}

/* Implementation for putting parameters from a stack. */
private int
stack_param_read(iparam_list * plist, const ref * pkey, iparam_loc * ploc)
{
    stack_param_list *const splist = (stack_param_list *) plist;
    ref_stack_t *pstack = splist->pstack;

    /* This implementation is slow, but it probably doesn't matter. */
    uint index = splist->skip + 1;
    uint count = splist->count;

    for (; count; count--, index += 2) {
	const ref *p = ref_stack_index(pstack, index);

	if (r_has_type(p, t_name) && name_eq(p, pkey)) {
	    ploc->pvalue = ref_stack_index(pstack, index - 1);
	    ploc->presult = &plist->results[count - 1];
	    *ploc->presult = 1;
	    return 0;
	}
    }
    return 1;
}
int
stack_param_list_read(stack_param_list * plist, ref_stack_t * pstack,
		      uint skip, const ref * ppolicies, bool require_all,
		      gs_ref_memory_t *imem)
{
    iparam_list *const iplist = (iparam_list *) plist;
    uint count = ref_stack_counttomark(pstack);

    if (count == 0)
	return_error(e_unmatchedmark);
    count -= skip + 1;
    if (count & 1)
	return_error(e_rangecheck);
    plist->u.r.read = stack_param_read;
    plist->enumerate = stack_param_enumerate;
    plist->pstack = pstack;
    plist->skip = skip;
    return ref_param_read_init(iplist, count >> 1, ppolicies, require_all, imem);
}

/* Implementation for putting parameters from a dictionary. */
private int
dict_param_read(iparam_list * plist, const ref * pkey, iparam_loc * ploc)
{
    ref const *spdict = &((dict_param_list *) plist)->dict;
    int code = dict_find(spdict, pkey, &ploc->pvalue);

    if (code != 1)
	return 1;
    ploc->presult =
	&plist->results[dict_value_index(spdict, ploc->pvalue)];
    *ploc->presult = 1;
    return 0;
}
int
dict_param_list_read(dict_param_list * plist, const ref * pdict,
		     const ref * ppolicies, bool require_all,
		     gs_ref_memory_t *imem)
{
    iparam_list *const iplist = (iparam_list *) plist;
    uint count;

    if (pdict == 0) {
	plist->u.r.read = empty_param_read;
	count = 0;
    } else {
	check_dict_read(*pdict);
	plist->u.r.read = dict_param_read;
	plist->dict = *pdict;
	count = dict_max_index(pdict) + 1;
    }
    plist->enumerate = dict_param_enumerate;
    return ref_param_read_init(iplist, count, ppolicies, require_all, imem);
}
