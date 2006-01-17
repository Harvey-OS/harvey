/* Copyright (C) 1993, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: dirent_.h,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/* Generic substitute for Unix dirent.h */

#ifndef dirent__INCLUDED
#  define dirent__INCLUDED

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"

/*
 * The location (or existence) of certain system headers is
 * environment-dependent. We detect this in the makefile
 * and conditionally define switches in gconfig_.h.
 */
#include "gconfig_.h"

/*
 * Directory entries may be defined in quite a number of different
 * header files.  The following switches are defined in gconfig_.h.
 */
#ifdef HAVE_DIRENT_H
#  include <dirent.h>
typedef struct dirent dir_entry;

#else /* sys/ndir or ndir or sys/dir, i.e., no dirent */
#  ifdef HAVE_SYS_DIR_H
#    include <sys/dir.h>
#  endif
#  ifdef HAVE_SYS_NDIR_H
#    include <sys/ndir.h>
#  endif
#  ifdef HAVE_NDIR_H
#    include <ndir.h>
#  endif
typedef struct direct dir_entry;

#endif /* sys/ndir or ndir or sys/dir */

#endif /* dirent__INCLUDED */
