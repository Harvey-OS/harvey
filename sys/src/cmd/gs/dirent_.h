/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* dirent_.h */
/* Generic substitute for Unix dirent.h */

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"

/* The location (or existence) of certain system headers is */
/* environment-dependent. We detect this in the makefile */
/* and conditionally define switches in gconfig_.h. */
#include "gconfig_.h"

/* Directory entries may be defined in quite a number of different */
/* header files.  The following switches are defined in gconfig_.h. */
#if !defined (SYSNDIR_H) && !defined (NDIR_H) && !defined (SYSDIR_H)
#  include <dirent.h>
typedef struct dirent dir_entry;
#else		/* SYSNDIR or NDIR or SYSDIR, i.e., no dirent */
#  ifdef SYSDIR_H
#    include <sys/dir.h>
#  endif
#  ifdef SYSNDIR_H
#    include <sys/ndir.h>
#  endif
#  ifdef NDIR_H
#    include <ndir.h>
#  endif
typedef struct direct dir_entry;
#endif		/* SYSNDIR or NDIR or SYSDIR */
