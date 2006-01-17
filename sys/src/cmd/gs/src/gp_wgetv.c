/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gp_wgetv.c,v 1.6 2002/02/21 22:24:52 giles Exp $ */
/* MS Windows implementation of gp_getenv */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>		/* for getenv */
#include <string.h>
#include "gscdefs.h"		/* for gs_productfamily and gs_revision */

/* prototypes */
int gp_getenv_registry(HKEY hkeyroot, const char *key, const char *name, 
    char *ptr, int *plen);

/* ------ Environment variables ------ */

/* Get the value of an environment variable.  See gp.h for details. */
int 
gp_getenv(const char *name, char *ptr, int *plen)
{
    const char *str = getenv(name);

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
    /* environment variable was not found */

#ifdef __WIN32__
    {
	/* If using Win32, look in the registry for a value with
	 * the given name.  The registry value will be under the key
	 * HKEY_CURRENT_USER\Software\AFPL Ghostscript\N.NN
	 * or if that fails under the key
	 * HKEY_LOCAL_MACHINE\Software\AFPL Ghostscript\N.NN
	 * where "AFPL Ghostscript" is actually gs_productfamily
	 * and N.NN is obtained from gs_revision.
	 */
	DWORD version = GetVersion();

	if (!(((HIWORD(version) & 0x8000) != 0)
	      && ((HIWORD(version) & 0x4000) == 0))) {
	    /* not Win32s */
	    int code;
	    char key[256];
	    char dotversion[16];
	    
	    sprintf(dotversion, "%d.%02d", (int)(gs_revision / 100),
		    (int)(gs_revision % 100));
	    sprintf(key, "Software\\%s\\%s", gs_productfamily, dotversion);

	    code = gp_getenv_registry(HKEY_CURRENT_USER, key, name, ptr, plen);
	    if ( code <= 0 )
		return code;	/* found it */

	    code = gp_getenv_registry(HKEY_LOCAL_MACHINE, key, name, ptr, plen);
	    if ( code <= 0 )
		return code;	/* found it */
	}
    }
#endif

    /* nothing found at all */

    if (*plen > 0)
	*ptr = 0;
    *plen = 1;
    return 1;
}


/*
 * Get a named registry value.
 * Key = hkeyroot\\key, named value = name.
 * name, ptr, plen and return values are the same as in gp_getenv();
 */

int 
gp_getenv_registry(HKEY hkeyroot, const char *key, const char *name, 
    char *ptr, int *plen)
{
    HKEY hkey;
    DWORD cbData, keytype;
    BYTE b;
    LONG rc;
    BYTE *bptr = (BYTE *)ptr;

    if (RegOpenKeyEx(hkeyroot, key, 0, KEY_READ, &hkey)
	== ERROR_SUCCESS) {
	keytype = REG_SZ;
	cbData = *plen;
	if (bptr == (char *)NULL)
	    bptr = &b;	/* Registry API won't return ERROR_MORE_DATA */
			/* if ptr is NULL */
	rc = RegQueryValueEx(hkey, (char *)name, 0, &keytype, bptr, &cbData);
	RegCloseKey(hkey);
	if (rc == ERROR_SUCCESS) {
	    *plen = cbData;
	    return 0;	/* found environment variable and copied it */
	} else if (rc == ERROR_MORE_DATA) {
	    /* buffer wasn't large enough */
	    *plen = cbData;
	    return -1;
	}
    }
    return 1;	/* not found */
}
