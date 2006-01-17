/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: idparam.h,v 1.7 2004/08/04 19:36:12 stefan Exp $ */
/* Interface to idparam.c */

#ifndef idparam_INCLUDED
#  define idparam_INCLUDED

#ifndef gs_matrix_DEFINED
#  define gs_matrix_DEFINED
typedef struct gs_matrix_s gs_matrix;
#endif

#ifndef gs_uid_DEFINED
#  define gs_uid_DEFINED
typedef struct gs_uid_s gs_uid;
#endif

/*
 * Unless otherwise noted, all the following routines return 0 for
 * a valid parameter, 1 for a defaulted parameter, or <0 on error.
 *
 * Note that all the dictionary parameter routines take a C string,
 * not a t_name ref *.  Even though this is slower, it means that
 * the GC doesn't have to worry about finding statically declared
 * name refs, and we have that many fewer static variables.
 *
 * All these routines allow pdict == NULL, which they treat the same as
 * pdict referring to an empty dictionary.  Routines with "null" in their
 * name return 2 if the parameter is null, without setting *pvalue.
 */
int dict_bool_param(const ref * pdict, const char *kstr,
		    bool defaultval, bool * pvalue);
int dict_int_param(const ref * pdict, const char *kstr,
		   int minval, int maxval, int defaultval, int *pvalue);
int dict_int_null_param(const ref * pdict, const char *kstr,
			int minval, int maxval, int defaultval,
			int *pvalue);
int dict_uint_param(const ref * pdict, const char *kstr,
		    uint minval, uint maxval, uint defaultval,
		    uint * pvalue);
int dict_float_param(const ref * pdict, const char *kstr,
		     floatp defaultval, float *pvalue);
/*
 * There are 3 variants of the procedures for getting array parameters.
 * All return the element count if the parameter is present and of the
 * correct size, 0 if the key is missing.
 *	_xxx_check_param return over_error if the array size > len,
 *	  (under_error < 0 ? under_error : the element count) if the array
 *	  size < len.
 *	_xxx_param return limitcheck if the array size > maxlen.
 *	  Equivalent to _xxx_check_param(..., 0, limitcheck).
 *	_xxxs return rangecheck if the array size != len.
 *	  Equivalent to _xxx_check_param(..., rangecheck, rangecheck).
 * All can return other error codes (e.g., typecheck).
 */
int dict_int_array_check_param(const ref * pdict, const char *kstr,
			       uint len, int *ivec,
			       int under_error, int over_error);
int dict_int_array_param(const ref * pdict, const char *kstr,
			 uint maxlen, int *ivec);
int dict_ints_param(const ref * pdict, const char *kstr,
		    uint len, int *ivec);
/*
 * For _float_array_param, if the parameter is missing and defaultvec is
 * not NULL, copy (max)len elements from defaultvec to fvec and return
 * (max)len.
 */
int dict_float_array_check_param(const gs_memory_t *mem,
				 const ref * pdict, const char *kstr,
				 uint len, float *fvec,
				 const float *defaultvec,
				 int under_error, int over_error);
int dict_float_array_param(const gs_memory_t *mem,
			   const ref * pdict, const char *kstr,
			   uint maxlen, float *fvec,
			   const float *defaultvec);
int dict_floats_param(const gs_memory_t *mem,
		      const ref * pdict, const char *kstr,
		      uint len, float *fvec,
		      const float *defaultvec);

/*
 * For dict_proc_param,
 *      defaultval = false means substitute t__invalid;
 *      defaultval = true means substitute an empty procedure.
 * In either case, return 1.
 */
int dict_proc_param(const ref * pdict, const char *kstr, ref * pproc,
		    bool defaultval);
int dict_matrix_param(const gs_memory_t *mem, 
		      const ref * pdict, const char *kstr,
		      gs_matrix * pmat);
int dict_uid_param(const ref * pdict, gs_uid * puid, int defaultval,
		   gs_memory_t * mem, const i_ctx_t *i_ctx_p);

/* Check that a UID in a dictionary is equal to an existing, valid UID. */
bool dict_check_uid_param(const ref * pdict, const gs_uid * puid);

#endif /* idparam_INCLUDED */
