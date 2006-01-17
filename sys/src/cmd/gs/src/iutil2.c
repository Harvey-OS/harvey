/* Copyright (C) 1993, 1994, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: iutil2.c,v 1.6 2003/09/03 03:22:59 giles Exp $ */
/* Level 2 utilities for Ghostscript interpreter */
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "ierrors.h"
#include "opcheck.h"
#include "gsparam.h"
#include "gsutil.h"		/* bytes_compare prototype */
#include "idict.h"
#include "imemory.h"		/* for iutil.h */
#include "iutil.h"
#include "iutil2.h"

/* ------ Password utilities ------ */

/* Read a password from a parameter list. */
/* Return 0 if present, 1 if absent, or an error code. */
int
param_read_password(gs_param_list * plist, const char *kstr, password * ppass)
{
    gs_param_string ps;
    long ipass;
    int code;

    ps.data = (const byte *)ppass->data, ps.size = ppass->size,
	ps.persistent = false;
    code = param_read_string(plist, kstr, &ps);
    switch (code) {
	case 0:		/* OK */
	    if (ps.size > MAX_PASSWORD)
		return_error(e_limitcheck);
	    /* Copy the data back. */
	    memcpy(ppass->data, ps.data, ps.size);
	    ppass->size = ps.size;
	    return 0;
	case 1:		/* key is missing */
	    return 1;
    }
    /* We might have gotten a typecheck because */
    /* the supplied password was an integer. */
    if (code != e_typecheck)
	return code;
    code = param_read_long(plist, kstr, &ipass);
    if (code != 0)		/* error or missing */
	return code;
    sprintf((char *)ppass->data, "%ld", ipass);
    ppass->size = strlen((char *)ppass->data);
    return 0;
}
/* Write a password to a parameter list. */
int
param_write_password(gs_param_list * plist, const char *kstr,
		     const password * ppass)
{
    gs_param_string ps;

    ps.data = (const byte *)ppass->data, ps.size = ppass->size,
	ps.persistent = false;
    if (ps.size > MAX_PASSWORD)
	return_error(e_limitcheck);
    return param_write_string(plist, kstr, &ps);
}

/* Check a password from a parameter list. */
/* Return 0 if OK, 1 if not OK, or an error code. */
int
param_check_password(gs_param_list * plist, const password * ppass)
{
    if (ppass->size != 0) {
	password pass;
	int code = param_read_password(plist, "Password", &pass);

	if (code)
	    return code;
	if (pass.size != ppass->size ||
	    bytes_compare(&pass.data[0], pass.size,
			  &ppass->data[0],
			  ppass->size) != 0
	    )
	    return 1;
    }
    return 0;
}

/* Read a password from, or write a password into, a dictionary */
/* (presumably systemdict). */
private int
dict_find_password(ref ** ppvalue, const ref * pdref, const char *kstr)
{
    ref *pvalue;

    if (dict_find_string(pdref, kstr, &pvalue) <= 0)
	return_error(e_undefined);
    if (!r_has_type(pvalue, t_string) ||
	r_has_attrs(pvalue, a_read) ||
	pvalue->value.const_bytes[0] >= r_size(pvalue)
	)
	return_error(e_rangecheck);
    *ppvalue = pvalue;
    return 0;
}
int
dict_read_password(password * ppass, const ref * pdref, const char *pkey)
{
    ref *pvalue;
    int code = dict_find_password(&pvalue, pdref, pkey);

    if (code < 0)
	return code;
    if (pvalue->value.const_bytes[0] > MAX_PASSWORD)
	return_error(e_rangecheck);	/* limitcheck? */
    memcpy(ppass->data, pvalue->value.const_bytes + 1,
	   (ppass->size = pvalue->value.const_bytes[0]));
    return 0;
}
int
dict_write_password(const password * ppass, ref * pdref, const char *pkey,
			bool change_allowed)
{
    ref *pvalue;
    int code = dict_find_password(&pvalue, pdref, pkey);

    if (code < 0)
	return code;
    if (ppass->size >= r_size(pvalue))
	return_error(e_rangecheck);
    if (!change_allowed &&
    	bytes_compare(pvalue->value.bytes + 1, pvalue->value.bytes[0],
	    ppass->data, ppass->size) != 0)
	return_error(e_invalidaccess);
    memcpy(pvalue->value.bytes + 1, ppass->data,
	   (pvalue->value.bytes[0] = ppass->size));
    return 0;
}
