/* Copyright (C) 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsparam.c,v 1.7 2002/10/07 08:28:56 ghostgum Exp $ */
/* Support for parameter lists */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsparam.h"
#include "gsstruct.h"

/* GC procedures */
ENUM_PTRS_WITH(gs_param_typed_value_enum_ptrs, gs_param_typed_value *pvalue) return 0;
    case 0:
    switch (pvalue->type) {
    case gs_param_type_string:
	return ENUM_STRING(&pvalue->value.s);
    case gs_param_type_name:
	return ENUM_STRING(&pvalue->value.n);
    case gs_param_type_int_array:
	return ENUM_OBJ(pvalue->value.ia.data);
    case gs_param_type_float_array:
	return ENUM_OBJ(pvalue->value.fa.data);
    case gs_param_type_string_array:
	return ENUM_OBJ(pvalue->value.sa.data);
    case gs_param_type_name_array:
	return ENUM_OBJ(pvalue->value.na.data);
    default:
	return ENUM_OBJ(0);	/* don't stop early */
    }
ENUM_PTRS_END
RELOC_PTRS_WITH(gs_param_typed_value_reloc_ptrs, gs_param_typed_value *pvalue) {
    switch (pvalue->type) {
    case gs_param_type_string:
    case gs_param_type_name: {
	gs_const_string str;

	str.data = pvalue->value.s.data; /* n == s */
	str.size = pvalue->value.s.size;
	RELOC_CONST_STRING_VAR(str);
	pvalue->value.s.data = str.data;
	break;
    }
    case gs_param_type_int_array:
	RELOC_VAR(pvalue->value.ia.data);
	break;
    case gs_param_type_float_array:
	RELOC_VAR(pvalue->value.fa.data);
	break;
    case gs_param_type_string_array:
	RELOC_VAR(pvalue->value.sa.data);
	break;
    case gs_param_type_name_array:
	RELOC_VAR(pvalue->value.na.data);
	break;
    default:
	break;
    }
}
RELOC_PTRS_END

/* Internal procedure to initialize the common part of a parameter list. */
void
gs_param_list_init(gs_param_list *plist, const gs_param_list_procs *procs,
		   gs_memory_t *mem)
{
    plist->procs = procs;
    plist->memory = mem;
    plist->persistent_keys = true;
}

/* Set whether the keys for param_write_XXX are persistent. */
void
gs_param_list_set_persistent_keys(gs_param_list *plist, bool persistent)
{
    plist->persistent_keys = persistent;
}

/* Reset a gs_param_key_t enumerator to its initial state */
void
param_init_enumerator(gs_param_enumerator_t * enumerator)
{
    memset(enumerator, 0, sizeof(*enumerator));
}

/* Transfer a collection of parameters. */
private const byte xfer_item_sizes[] = {
    GS_PARAM_TYPE_SIZES(0)
};
int
gs_param_read_items(gs_param_list * plist, void *obj,
		    const gs_param_item_t * items)
{
    const gs_param_item_t *pi;
    int ecode = 0;

    for (pi = items; pi->key != 0; ++pi) {
	const char *key = pi->key;
	void *pvalue = (void *)((char *)obj + pi->offset);
	gs_param_typed_value typed;
	int code;

	typed.type = pi->type;
	code = param_read_requested_typed(plist, key, &typed);
	switch (code) {
	    default:		/* < 0 */
		ecode = code;
	    case 1:
		break;
	    case 0:
		if (typed.type != pi->type)	/* shouldn't happen! */
		    ecode = gs_note_error(gs_error_typecheck);
		else
		    memcpy(pvalue, &typed.value, xfer_item_sizes[pi->type]);
	}
    }
    return ecode;
}
int
gs_param_write_items(gs_param_list * plist, const void *obj,
		     const void *default_obj, const gs_param_item_t * items)
{
    const gs_param_item_t *pi;
    int ecode = 0;

    for (pi = items; pi->key != 0; ++pi) {
	const char *key = pi->key;
	const void *pvalue = (const void *)((const char *)obj + pi->offset);
	int size = xfer_item_sizes[pi->type];
	gs_param_typed_value typed;
	int code;

	if (default_obj != 0 &&
	    !memcmp((const void *)((const char *)default_obj + pi->offset),
		    pvalue, size)
	    )
	    continue;
	memcpy(&typed.value, pvalue, size);
	typed.type = pi->type;
	code = (*plist->procs->xmit_typed) (plist, key, &typed);
	if (code < 0)
	    ecode = code;
    }
    return ecode;
}

/* Read a value, with coercion if requested, needed, and possible. */
/* If mem != 0, we can coerce int arrays to float arrays. */
int
param_coerce_typed(gs_param_typed_value * pvalue, gs_param_type req_type,
		   gs_memory_t * mem)
{
    if (req_type == gs_param_type_any || pvalue->type == req_type)
	return 0;
    /*
     * Look for coercion opportunities.  It would be wonderful if we
     * could convert int/float arrays and name/string arrays, but
     * right now we can't.  However, a 0-length heterogenous array
     * will satisfy a request for any specific type.
     */
    switch (pvalue->type /* actual type */ ) {
	case gs_param_type_int:
	    switch (req_type) {
		case gs_param_type_long:
		    pvalue->value.l = pvalue->value.i;
		    goto ok;
		case gs_param_type_float:
		    pvalue->value.f = (float)pvalue->value.l;
		    goto ok;
		default:
		    break;
	    }
	    break;
	case gs_param_type_long:
	    switch (req_type) {
		case gs_param_type_int:
#if arch_sizeof_int < arch_sizeof_long
		    if (pvalue->value.l != (int)pvalue->value.l)
			return_error(gs_error_rangecheck);
#endif
		    pvalue->value.i = (int)pvalue->value.l;
		    goto ok;
		case gs_param_type_float:
		    pvalue->value.f = (float)pvalue->value.l;
		    goto ok;
		default:
		    break;
	    }
	    break;
	case gs_param_type_string:
	    if (req_type == gs_param_type_name)
		goto ok;
	    break;
	case gs_param_type_name:
	    if (req_type == gs_param_type_string)
		goto ok;
	    break;
	case gs_param_type_int_array:
	    switch (req_type) {
		case gs_param_type_float_array:{
			uint size = pvalue->value.ia.size;
			float *fv;
			uint i;

			if (mem == 0)
			    break;
			fv = (float *)gs_alloc_byte_array(mem, size, sizeof(float),
						"int array => float array");

			if (fv == 0)
			    return_error(gs_error_VMerror);
			for (i = 0; i < size; ++i)
			    fv[i] = (float)pvalue->value.ia.data[i];
			pvalue->value.fa.data = fv;
			pvalue->value.fa.persistent = false;
			goto ok;
		    }
		default:
		    break;
	    }
	    break;
	case gs_param_type_string_array:
	    if (req_type == gs_param_type_name_array)
		goto ok;
	    break;
	case gs_param_type_name_array:
	    if (req_type == gs_param_type_string_array)
		goto ok;
	    break;
	case gs_param_type_array:
	    if (pvalue->value.d.size == 0 &&
		(req_type == gs_param_type_int_array ||
		 req_type == gs_param_type_float_array ||
		 req_type == gs_param_type_string_array ||
		 req_type == gs_param_type_name_array)
		)
		goto ok;
	    break;
	default:
	    break;
    }
    return_error(gs_error_typecheck);
  ok:pvalue->type = req_type;
    return 0;
}
int
param_read_requested_typed(gs_param_list * plist, gs_param_name pkey,
			   gs_param_typed_value * pvalue)
{
    gs_param_type req_type = pvalue->type;
    int code = (*plist->procs->xmit_typed) (plist, pkey, pvalue);

    if (code != 0)
	return code;
    return param_coerce_typed(pvalue, req_type, plist->memory);
}


/* ---------------- Fixed-type reading procedures ---------------- */

#define RETURN_READ_TYPED(alt, ptype)\
  gs_param_typed_value typed;\
  int code;\
\
  typed.type = ptype;\
  code = param_read_requested_typed(plist, pkey, &typed);\
  if ( code == 0 )\
    *pvalue = typed.value.alt;\
  return code

int
param_read_null(gs_param_list * plist, gs_param_name pkey)
{
    gs_param_typed_value typed;

    typed.type = gs_param_type_null;
    return param_read_requested_typed(plist, pkey, &typed);
}
int
param_read_bool(gs_param_list * plist, gs_param_name pkey, bool * pvalue)
{
    RETURN_READ_TYPED(b, gs_param_type_bool);
}
int
param_read_int(gs_param_list * plist, gs_param_name pkey, int *pvalue)
{
    RETURN_READ_TYPED(i, gs_param_type_int);
}
int
param_read_long(gs_param_list * plist, gs_param_name pkey, long *pvalue)
{
    RETURN_READ_TYPED(l, gs_param_type_long);
}
int
param_read_float(gs_param_list * plist, gs_param_name pkey, float *pvalue)
{
    RETURN_READ_TYPED(f, gs_param_type_float);
}
int
param_read_string(gs_param_list * plist, gs_param_name pkey,
		  gs_param_string * pvalue)
{
    RETURN_READ_TYPED(s, gs_param_type_string);
}
int
param_read_name(gs_param_list * plist, gs_param_name pkey,
		gs_param_string * pvalue)
{
    RETURN_READ_TYPED(n, gs_param_type_string);
}
int
param_read_int_array(gs_param_list * plist, gs_param_name pkey,
		     gs_param_int_array * pvalue)
{
    RETURN_READ_TYPED(ia, gs_param_type_int_array);
}
int
param_read_float_array(gs_param_list * plist, gs_param_name pkey,
		       gs_param_float_array * pvalue)
{
    RETURN_READ_TYPED(fa, gs_param_type_float_array);
}
int
param_read_string_array(gs_param_list * plist, gs_param_name pkey,
			gs_param_string_array * pvalue)
{
    RETURN_READ_TYPED(sa, gs_param_type_string_array);
}
int
param_read_name_array(gs_param_list * plist, gs_param_name pkey,
		      gs_param_string_array * pvalue)
{
    RETURN_READ_TYPED(na, gs_param_type_name_array);
}

#undef RETURN_READ_TYPED

/* ---------------- Default writing procedures ---------------- */

#define RETURN_WRITE_TYPED(alt, ptype)\
  gs_param_typed_value typed;\
\
  typed.value.alt = *pvalue;\
  typed.type = ptype;\
  return param_write_typed(plist, pkey, &typed)

int
param_write_null(gs_param_list * plist, gs_param_name pkey)
{
    gs_param_typed_value typed;

    typed.type = gs_param_type_null;
    return param_write_typed(plist, pkey, &typed);
}
int
param_write_bool(gs_param_list * plist, gs_param_name pkey, const bool * pvalue)
{
    RETURN_WRITE_TYPED(b, gs_param_type_bool);
}
int
param_write_int(gs_param_list * plist, gs_param_name pkey, const int *pvalue)
{
    RETURN_WRITE_TYPED(i, gs_param_type_int);
}
int
param_write_long(gs_param_list * plist, gs_param_name pkey, const long *pvalue)
{
    RETURN_WRITE_TYPED(l, gs_param_type_long);
}
int
param_write_float(gs_param_list * plist, gs_param_name pkey,
		  const float *pvalue)
{
    RETURN_WRITE_TYPED(f, gs_param_type_float);
}
int
param_write_string(gs_param_list * plist, gs_param_name pkey,
		   const gs_param_string * pvalue)
{
    RETURN_WRITE_TYPED(s, gs_param_type_string);
}
int
param_write_name(gs_param_list * plist, gs_param_name pkey,
		 const gs_param_string * pvalue)
{
    RETURN_WRITE_TYPED(n, gs_param_type_name);
}
int
param_write_int_array(gs_param_list * plist, gs_param_name pkey,
		      const gs_param_int_array * pvalue)
{
    RETURN_WRITE_TYPED(ia, gs_param_type_int_array);
}
int
param_write_int_values(gs_param_list * plist, gs_param_name pkey,
		       const int *values, uint size, bool persistent)
{
    gs_param_int_array ia;

    ia.data = values, ia.size = size, ia.persistent = persistent;
    return param_write_int_array(plist, pkey, &ia);
}
int
param_write_float_array(gs_param_list * plist, gs_param_name pkey,
			const gs_param_float_array * pvalue)
{
    RETURN_WRITE_TYPED(fa, gs_param_type_float_array);
}
int
param_write_float_values(gs_param_list * plist, gs_param_name pkey,
			 const float *values, uint size, bool persistent)
{
    gs_param_float_array fa;

    fa.data = values, fa.size = size, fa.persistent = persistent;
    return param_write_float_array(plist, pkey, &fa);
}
int
param_write_string_array(gs_param_list * plist, gs_param_name pkey,
			 const gs_param_string_array * pvalue)
{
    RETURN_WRITE_TYPED(sa, gs_param_type_string_array);
}
int
param_write_name_array(gs_param_list * plist, gs_param_name pkey,
		       const gs_param_string_array * pvalue)
{
    RETURN_WRITE_TYPED(na, gs_param_type_name_array);
}

#undef RETURN_WRITE_TYPED

/* ---------------- Default request implementation ---------------- */

int
gs_param_request_default(gs_param_list * plist, gs_param_name pkey)
{
    return 0;
}

int
gs_param_requested_default(const gs_param_list * plist, gs_param_name pkey)
{
    return -1;			/* requested by default */
}
