/* Copyright (C) 1992, 1993, 1994, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: stdio_.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Generic substitute for stdio.h */

#ifndef stdio__INCLUDED
#  define stdio__INCLUDED

/*
 * This is here primarily because we must include std.h before
 * any file that includes sys/types.h.
 */
#include "std.h"
#include <stdio.h>

#ifdef VMS
/* VMS doesn't have the unlink system call.  Use delete instead. */
#  ifdef __DECC
#    include <unixio.h>
#  endif
#  define unlink(fname) delete(fname)
#else
/*
 * Other systems may or may not declare unlink in stdio.h;
 * if they do, the declaration will be compatible with this one.
 */
int unlink(P1(const char *));

#endif

/*
 * Plan 9 has a system function called sclose, which interferes with the
 * procedure defined in stream.h.  The following makes the system sclose
 * inaccessible, but avoids the name clash.
 */
#ifdef Plan9
#  undef sclose
#  define sclose(s) Sclose(s)
#endif

/* Patch a couple of things possibly missing from stdio.h. */
#ifndef SEEK_SET
#  define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#  define SEEK_CUR 1
#endif
#ifndef SEEK_END
#  define SEEK_END 2
#endif

#endif /* stdio__INCLUDED */
