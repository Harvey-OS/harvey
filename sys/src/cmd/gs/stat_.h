/* Copyright (C) 1991, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* stat_.h */
/* Generic substitute for Unix sys/stat.h */

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"
#include <sys/stat.h>

/* Several environments, including the MS-DOS compilers, don't define */
/* the st_blocks member of a stat structure. */
#if defined(__SVR3) || defined(__EMX__) || defined(__DVX__) || defined(OSK) || defined(__MSDOS__) || defined(VMS) || defined(__WIN32__) || defined(__IBMC__) || defined(Plan9)
#  define stat_blocks(psbuf) (((psbuf)->st_size + 1023) >> 10)
#else
#  define stat_blocks(psbuf) ((psbuf)->st_blocks)
#endif

/* Microsoft C uses _stat instead of stat, */
/* for both the function name and the structure name. */
#ifdef _MSC_VER
#  define stat _stat
#endif

/* Some (System V?) systems test for directories */
/* in a slightly different way. */
#if defined(OSK) || !defined(S_ISDIR)
#  define stat_is_dir(stbuf) ((stbuf).st_mode & S_IFDIR)
#else
#  define stat_is_dir(stbuf) S_ISDIR((stbuf).st_mode)
#endif
