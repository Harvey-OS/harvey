/* Copyright (C) 1993, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: windows_.h,v 1.7 2004/08/31 19:23:14 alexcher Exp $ */
/* Wrapper for windows.h */

#ifndef windows__INCLUDED
#  define windows__INCLUDED

#define STRICT
#include <windows.h>

#ifdef __WATCOMC__
typedef RGBQUAD FAR * LPRGBQUAD;
	/* Watcom's _beginthread takes an extra stack_bottom argument. */
#  define BEGIN_THREAD(proc, stksize, data)\
     _beginthread(proc, NULL, stksize, data)
#else
#  define BEGIN_THREAD(proc, stksize, data)\
     _beginthread(proc, stksize, data)
	/* Define null equivalents of the Watcom 32-to-16-bit glue. */
#  define AllocAlias16(ptr) ((DWORD)(ptr))
#  define FreeAlias16(dword)	/* */
#  define MK_FP16(fp32) ((DWORD)(fp32))
#  define MK_FP32(fp16) (fp16)
#  define GetProc16(proc, ptype) (proc)
#  define ReleaseProc16(cbp)	/* */
#endif

/* Substitute for special "far" library procedures under Win32. */
#ifdef __WIN32__
#  undef _fstrtok
#  define _fstrtok(str, set) strtok(str, set)
#endif

#if defined(__BORLANDC__)
#  define exception_code() __exception_code  
#endif

#endif /* windows__INCLUDED */
