/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gp_msdll.c,v 1.1 2001/03/13 07:09:28 ghostgum Exp $ */
/*
 * Microsoft Windows DLL support for Ghostscript.
 *
 */
#include "windows_.h"
#include "iapi.h"
#include "gp_mswin.h"

/* DLL entry point for Borland C++ */
GSDLLEXPORT BOOL WINAPI
DllEntryPoint(HINSTANCE hInst, DWORD fdwReason, LPVOID lpReserved)
{
    /* Win32s: HIWORD bit 15 is 1 and bit 14 is 0 */
    /* Win95:  HIWORD bit 15 is 1 and bit 14 is 1 */
    /* WinNT:  HIWORD bit 15 is 0 and bit 14 is 0 */
    /* WinNT Shell Update Release is WinNT && LOBYTE(LOWORD) >= 4 */
    DWORD version = GetVersion();

    if (((HIWORD(version) & 0x8000) != 0) && ((HIWORD(version) & 0x4000) == 0))
	is_win32s = TRUE;

    phInstance = hInst;
    return TRUE;
}

/* DLL entry point for Microsoft Visual C++ */
GSDLLEXPORT BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpReserved)
{
    return DllEntryPoint(hInst, fdwReason, lpReserved);
}


