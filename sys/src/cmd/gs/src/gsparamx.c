/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gsparamx.c,v 1.3 2000/09/19 19:00:30 lpd Exp $ */
/* Extended parameter dictionary utilities */
#include "string_.h"
#include "gserror.h"
#include "gserrors.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gsparam.h"
#include "gsparamx.h"

/* Compare a C string and a gs_param_string. */
bool
gs_param_string_eq(const gs_param_string * pcs, const char *str)
{
    return (strlen(str) == pcs->size &&
	    !strncmp(str, (const char *)pcs->data, pcs->size));
}

/* Put an enumerated value. */
int
param_put_enum(gs_param_list * plist, gs_param_name param_name,
	       int *pvalue, const char *const pnames[], int ecode)
{
    gs_param_string ens;
    int code = param_read_name(plist, param_name, &ens);

    switch (code) {
	case 1:
	    return ecode;
	case 0:
	    {
		int i;

		for (i = 0; pnames[i] != 0; ++i)
		    if (gs_param_string_eq(&ens, pnames[i])) {
			*pvalue = i;
			return 0;
		    }
	    }
	    code = gs_error_rangecheck;
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, code);
    }
    return code;
}

/* Put a Boolean value. */
int
param_put_bool(gs_param_list * plist, gs_param_name param_name,
	       bool * pval, int ecode)
{
    int code;

    switch (code = param_read_bool(plist, param_name, pval)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }
    return ecode;
}

/* Put an integer value. */
int
param_put_int(gs_param_list * plist, gs_param_name param_name,
	      int *pval, int ecode)
{
    int code;

    switch (code = param_read_int(plist, param_name, pval)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }
    return ecode;
}

/* Put a long value. */
int
param_put_long(gs_param_list * plist, gs_param_name param_name,
	       long *pval, int ecode)
{
    int code;

    switch (code = param_read_long(plist, param_name, pval)) {
	default:
	    ecode = code;
	    param_signal_error(plist, param_name, ecode);
	case 0:
	case 1:
	    break;
    }
    return ecode;
}

/* Copy one parameter list to another, recursively if necessary. */
int
param_list_copy(gs_param_list *plto, gs_param_list *plfrom)
{
    gs_param_enumerator_t key_enum;
    gs_param_key_t key;
    int code;

    param_init_enumerator(&key_enum);
    while ((code = param_get_next_key(plfrom, &key_enum, &key)) == 0) {
	char string_key[256];	/* big enough for any reasonable key */
	gs_param_typed_value value;
	gs_param_collection_type_t coll_type;
	gs_param_typed_value copy;

	if (key.size > sizeof(string_key) - 1) {
	    code = gs_note_error(gs_error_rangecheck);
	    break;
	}
	memcpy(string_key, key.data, key.size);
	string_key[key.size] = 0;
	if ((code = param_read_typed(plfrom, string_key, &value)) != 0) {
	    code = (code > 0 ? gs_note_error(gs_error_unknownerror) : code);
	    break;
	}
	gs_param_list_set_persistent_keys(plto, key.persistent);
	switch (value.type) {
	case gs_param_type_dict:
	    coll_type = gs_param_collection_dict_any;
	    goto cc;
	case gs_param_type_dict_int_keys:
	    coll_type = gs_param_collection_dict_int_keys;
	    goto cc;
	case gs_param_type_array:
	    coll_type = gs_param_collection_array;
	cc:
	    copy.value.d.size = value.value.d.size;
	    if ((code = param_begin_write_collection(plto, string_key,
						     &copy.value.d,
						     coll_type)) < 0 ||
		(code = param_list_copy(copy.value.d.list,
					value.value.d.list)) < 0 ||
		(code = param_end_write_collection(plto, string_key,
						   &copy.value.d)) < 0)
		break;
	    code = param_end_read_collection(plfrom, string_key,
					     &value.value.d);
	    break;
	default:
	    code = param_write_typed(plto, string_key, &value);
	}
	if (code < 0)
	    break;
    }
    return code;
}
