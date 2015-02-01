/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Local acconfig.h for autoheader.
   Descriptive text for the C preprocessor macros that
   the patch configure.in can define.
   autoheader copies the comments into config.hin.  */

/* Define if there is a member named d_ino in the struct describing
   directory headers.  */
#undef D_INO_IN_DIRENT

/* Define if memchr works.  */
#undef HAVE_MEMCHR

/* Define if `struct utimbuf' is declared -- usually in <utime.h>.  */
#undef HAVE_STRUCT_UTIMBUF
