/* Copyright (C) 1993, 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: dirent_.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
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
