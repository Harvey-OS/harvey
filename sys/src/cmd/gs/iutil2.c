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

/* iutil2.c */
/* Level 2 utilities for Ghostscript interpreter */
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "errors.h"
#include "opcheck.h"
#include "gsparam.h"
#include "gsutil.h"		/* bytes_compare prototype */
#include "imemory.h"		/* for iutil.h */
#include "iutil.h"
#include "iutil2.h"

/* ------ Password utilities ------ */

/* Read a password from a parameter list. */
/* Return 0 if present, 1 if absent, or an error code. */
int
param_read_password(gs_param_list *plist, const char _ds *kstr, password *ppass)
{	gs_param_string ps;
	long ipass;
	int code;
	ps.data = (const byte *)ppass->data, ps.size = ppass->size,
	  ps.persistent = false;
	code = param_read_string(plist, kstr, &ps);
	switch ( code )
	{
	case 0:			/* OK */
		if ( ps.size > max_password )
		  return_error(e_limitcheck);
		/* Copy the data back. */
		memcpy(ppass->data, ps.data, ps.size);
		ppass->size = ps.size;
		return 0;
	case 1:			/* key is missing */
		return 1;
	}
	/* We might have gotten a typecheck because */
	/* the supplied password was an integer. */
	if ( code != e_typecheck)
	  return code;
	code = param_read_long(plist, kstr, &ipass);
	if ( code != 0 )	/* error or missing */
	  return code;
	sprintf((char *)ppass->data, "%ld", ipass);
	ppass->size = strlen((char *)ppass->data);
	return 0;
}
/* Write a password to a parameter list. */
int
param_write_password(gs_param_list *plist, const char _ds *kstr, const password *ppass)
{	gs_param_string ps;
	ps.data = (const byte *)ppass->data, ps.size = ppass->size,
	  ps.persistent = false;
	if ( ps.size > max_password )
	  return_error(e_limitcheck);
	return param_write_string(plist, kstr, &ps);
}

/* Check a password from a parameter list. */
/* Return 0 if OK, 1 if not OK, or an error code. */
int
param_check_password(gs_param_list *plist, const password *ppass)
{	if ( ppass->size != 0 )
	{	password pass;
		int code = param_read_password(plist, "Password", &pass);
		if ( code )
		  return code;
		if ( pass.size != ppass->size ||
		     bytes_compare(&pass.data[0], pass.size,
				   &ppass->data[0],
				   ppass->size) != 0
		   )
			return 1;
	}
	return 0;
}
