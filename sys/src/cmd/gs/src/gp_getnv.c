/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gp_getnv.c,v 1.5 2002/06/16 05:48:55 lpd Exp $ */
/* Standard implementation of gp_getenv */
#include "stdio_.h"
#include "string_.h"
#include "gsmemory.h"
#include "gstypes.h"
#include "gp.h"

/* Import the C getenv function. */
extern char *getenv(const char *);

/* Get the value of an environment variable.  See gp.h for details. */
int
gp_getenv(const char *key, char *ptr, int *plen)
{
    const char *str = getenv(key);

    if (str) {
	int len = strlen(str);

	if (len < *plen) {
	    /* string fits */
	    strcpy(ptr, str);
	    *plen = len + 1;
	    return 0;
	}
	/* string doesn't fit */
	*plen = len + 1;
	return -1;
    }
    /* missing key */
    if (*plen > 0)
	*ptr = 0;
    *plen = 1;
    return 1;
}
