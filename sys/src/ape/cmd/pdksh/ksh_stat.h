/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Wrapper around the ugly sys/stat includes/ifdefs */
/* $Id$ */

/* assumes <sys/types.h> already included */
#include <sys/stat.h>

#ifndef HAVE_LSTAT
#define lstat(path, buf) stat(path, buf)
#endif /* HAVE_LSTAT */

#ifdef STAT_MACROS_BROKEN
#undef S_ISREG
#undef S_ISDIR
#undef S_ISCHR
#undef S_ISBLK
#undef S_ISFIFO
#undef S_ISSOCK
#undef S_ISLNK
#endif /* STAT_MACROS_BROKEN */

#if !defined(S_ISREG) && defined(S_IFREG)
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#endif /* S_ISREG */
#if !defined(S_ISDIR) && defined(S_IFDIR)
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif /* S_ISDIR */
#if !defined(S_ISCHR) && defined(S_IFCHR)
#define S_ISCHR(m) (((m)&S_IFMT) == S_IFCHR)
#endif /* S_ISCHR */
#if !defined(S_ISBLK) && defined(S_IFBLK)
#define S_ISBLK(m) (((m)&S_IFMT) == S_IFBLK)
#endif /* S_ISBLK */
#if !defined(S_ISFIFO) && defined(S_IFIFO)
#define S_ISFIFO(m) (((m)&S_IFMT) == S_IFIFO)
#endif /* S_ISFIFO */
#if !defined(S_ISLNK) && defined(S_IFLNK)
#define S_ISLNK(m) (((m)&S_IFMT) == S_IFLNK)
#endif /* S_ISLNK */
#if !defined(S_ISSOCK) && defined(S_IFSOCK)
#define S_ISSOCK(m) (((m)&S_IFMT) == S_IFSOCK)
#endif /* S_ISSOCK */
#if !defined(S_ISCDF) && defined(S_CDF)
#define S_ISCDF(m) (S_ISDIR(m) && ((m)&S_CDF))
#endif /* S_ISSOCK */

#ifndef S_ISVTX
#define S_ISVTX 01000 /* sticky bit */
#endif		      /* S_ISVTX */

#ifndef S_IXUSR
#define S_IXUSR 00100 /* user execute bit */
#endif		      /* S_IXUSR */
#ifndef S_IXGRP
#define S_IXGRP 00010 /* user execute bit */
#endif		      /* S_IXGRP */
#ifndef S_IXOTH
#define S_IXOTH 00001 /* user execute bit */
#endif		      /* S_IXOTH */
