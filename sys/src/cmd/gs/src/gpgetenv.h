/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gpgetenv.h,v 1.5 2002/06/16 06:59:02 lpd Exp $ */
/* Interface to platform-specific getenv routine */

#ifndef gpgetenv_INCLUDED
#  define gpgetenv_INCLUDED

/*
 * Get a value from the environment (getenv).
 *
 * If the key is missing, set *ptr = 0 (if *plen > 0), set *plen = 1,
 * and return 1.
 *
 * If the key is present and the length len of the value (not counting
 * the terminating \0) is less than *plen, copy the value to ptr, set
 * *plen = len + 1, and return 0.
 *
 * If the key is present and len >= *plen, set *plen = len + 1,
 * don't store anything at ptr, and return -1.
 *
 * Note that *plen is the size of the buffer, not the length of the string:
 * because of the terminating \0, the maximum string length is 1 less than
 * the size of the buffer.
 */
int gp_getenv(const char *key, char *ptr, int *plen);

#endif /* gpgetenv_INCLUDED */
