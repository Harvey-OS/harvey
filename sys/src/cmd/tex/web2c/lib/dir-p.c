/* dir-p.c: directory predicates.

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

#include "config.h"

#include "dirio.h"
#include "xstat.h"


/* Return true if FN is a directory or a symlink to a directory,
   false if not. */

boolean
dir_p P1C(string, fn)
{
  struct stat stats;
  return stat (fn, &stats) == 0 && S_ISDIR (stats.st_mode);
}


/* Return true if FN is a directory (or a symlink to a directory) with
   no subdirectories, i.e., has exactly two links (. and ..).  This
   means that symbolic links to directories do not affect the leaf-ness
   of FN.  This is arguably wrong, but the only alternative I know of is
   to stat every entry in the directory, and that is unacceptably slow.  */

boolean
leaf_dir_p P1C(string, fn)
{
  struct stat stats; /* Old compilers can't initialize structures.  */
  
  return
    stat (fn, &stats) == 0 && stats.st_nlink == 2 && S_ISDIR (stats.st_mode);
}
