/* dirio.h: checked directory operations.

Copyright (C) 1992 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef DIRIO_H
#define DIRIO_H

#if defined (DIRENT) || defined (_POSIX_VERSION)
#include <dirent.h>
#define NLENGTH(dirent) (strlen ((dirent)->d_name))
#else /* not (DIRENT or _POSIX_VERSION) */
#define dirent direct
#define NLENGTH(dirent) ((dirent)->d_namlen)
#ifdef USG
#ifdef SYSNDIR
#include <sys/ndir.h>
#else /* not SYSNDIR */
#include <ndir.h>
#endif /* SYSNDIR */
#else /* not USG */
#include <sys/dir.h>
#endif /* USG */
#endif /* DIRENT or _POSIX_VERSION */

/* Like opendir, closedir, and chdir, but abort if DIRNAME can't be opened.  */
extern DIR *xopendir P1H(string dirname);
extern void xclosedir P1H(DIR *);

/* Returns true if FN is a directory (or a symlink to a directory).  */
extern boolean dir_p P1H(string fn);

/* Returns true if FN is directory with no subdirectories.  */
extern boolean leaf_dir_p P1H(string fn);

#endif /* not DIRIO_H */
