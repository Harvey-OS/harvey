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

/*$Id: gsparamx.h,v 1.3 2000/09/19 19:00:31 lpd Exp $ */
/* Interface to extended parameter dictionary utilities */

#ifndef gsparamx_INCLUDED
#  define gsparamx_INCLUDED

/* Test whether a parameter's string value is equal to a C string. */
bool gs_param_string_eq(P2(const gs_param_string *pcs, const char *str));

/*
 * Put parameters of various types.  These propagate ecode, presumably
 * the previous accumulated error code.
 */
int param_put_enum(P5(gs_param_list * plist, gs_param_name param_name,
		      int *pvalue, const char *const pnames[], int ecode));
int param_put_bool(P4(gs_param_list * plist, gs_param_name param_name,
		      bool * pval, int ecode));
int param_put_int(P4(gs_param_list * plist, gs_param_name param_name,
		     int * pval, int ecode));
int param_put_long(P4(gs_param_list * plist, gs_param_name param_name,
		      long * pval, int ecode));

/* Copy one parameter list to another, recursively if necessary. */
int param_list_copy(P2(gs_param_list *plto, gs_param_list *plfrom));

#endif /* gsparamx_INCLUDED */
