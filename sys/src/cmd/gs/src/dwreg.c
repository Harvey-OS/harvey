/* Copyright (C) 2001, Ghostgum Software Pty Ltd.  All rights reserved.
  
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

/* $Id: dwreg.c,v 1.3 2002/02/21 22:24:51 giles Exp $ */
/* MS Windows registry values */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>		/* for getenv */
#include <string.h>
#include "gscdefs.h"		/* for gs_productfamily and gs_revision */

/* We store registry named values under the key 
 * "Software\\AFPL Ghostscript"
 * where "AFPL Ghostscript" is actually gs_productfamily.
 * Either HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER will be used.
 */
int
win_registry_key(char *buf, int len)
{
    const char *software = "Software";
    if (strlen(software) + 1 + strlen(gs_productfamily) >= len)
	return -1;

   strcpy(buf, software);
   strcat(buf, "\\");
   strcat(buf, gs_productfamily);
   return 0;
}

/*
 * Get a named registry value from HKCU.
 * name, ptr, plen and return values are the same as in gp_getenv();
 */
int 
win_get_reg_value(const char *name, char *ptr, int *plen)
{
    HKEY hkey;
    DWORD cbData, keytype;
    BYTE b;
    LONG rc;
    BYTE *bptr = (BYTE *)ptr;
    char key[256];

    win_registry_key(key, sizeof(key));
    if (RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, KEY_READ, &hkey)
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

/*
 * Set a named registry value under HKCU.
 * name = name of named value
 * str = value of named value
 * Returns 0 on success.
 */
int 
win_set_reg_value(const char *name, const char *value)
{
    HKEY hkey;
    LONG rc;
    char key[256];
    DWORD dwDisposition;

    win_registry_key(key, sizeof(key));
    rc = RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, KEY_WRITE, &hkey);
    if (rc != ERROR_SUCCESS)
	rc = RegCreateKeyEx(HKEY_CURRENT_USER, key, 0, "", 0,
	    KEY_ALL_ACCESS, NULL, &hkey, &dwDisposition);
    if (rc == ERROR_SUCCESS) {
	rc = RegSetValueEx(hkey, name, 0, REG_SZ, 
		(CONST BYTE *)value, strlen(value)+1);
	RegCloseKey(hkey);
    }

    return rc == ERROR_SUCCESS ? 0 : -1;
}

