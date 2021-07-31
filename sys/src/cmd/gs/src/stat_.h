/* Copyright (C) 1991, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: stat_.h,v 1.6 2001/08/07 22:13:29 dancoby Exp $ */
/* Generic substitute for Unix sys/stat.h */

#ifndef stat__INCLUDED
#  define stat__INCLUDED

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"
#include <sys/stat.h>

/*
 * Many environments, including the MS-DOS compilers, don't define
 * the st_blocks member of a stat structure.
 */
#if defined(__SVR3) || defined(__EMX__) || defined(__DVX__) || defined(OSK) || defined(__MSDOS__) || defined(__QNX__) || defined(VMS) || defined(__WIN32__) || defined(__IBMC__) || defined(__BEOS__) || defined(Plan9) || defined(__WATCOMC__)
#  define stat_blocks(psbuf) (((psbuf)->st_size + 1023) >> 10)
#else
#  define stat_blocks(psbuf) ((psbuf)->st_blocks)
#endif

/*
 * Microsoft C uses _stat instead of stat,
 * for both the function name and the structure name.
 */
#ifdef _MSC_VER
#  define stat _stat
#endif

/*
 * Some (System V?) systems test for directories in a slightly different way.
 */
#if defined(OSK) || !defined(S_ISDIR)
#  ifdef S_IFDIR
#    define stat_is_dir(stbuf) ((stbuf).st_mode & S_IFDIR)
#  else
#    ifdef _S_IFDIR
#      define stat_is_dir(stbuf) ((stbuf).st_mode & _S_IFDIR)
#    endif
#  endif
#else
#  define stat_is_dir(stbuf) S_ISDIR((stbuf).st_mode)
#endif

/*
 * Some systems have S_IFMT and S_IFCHR but not S_ISCHR.
 */
#ifndef S_ISCHR
#  ifndef S_IFMT
#    ifdef _S_IFMT
#      define S_IFMT _S_IFMT
#      define S_IFCHR _S_IFCHR
#    else
#    ifdef __S_IFMT
#      define S_IFMT __S_IFMT
#      define S_IFCHR __S_IFCHR
#    endif
#    endif
#  endif
#  define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#endif

/*
 * Microsoft C doesn't define S_IRUSR or S_IWUSR.
 */
#ifndef S_IRUSR
#  ifndef S_IREAD
#    define S_IRUSR _S_IREAD
#  else
#    define S_IRUSR S_IREAD
#  endif
#endif
#ifndef S_IWUSR
#  ifndef S_IWRITE
#    define S_IWUSR _S_IWRITE
#  else
#    define S_IWUSR S_IWRITE
#  endif
#endif

#endif /* stat__INCLUDED */
