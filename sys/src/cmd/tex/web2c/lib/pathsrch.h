/* pathsrch.h: environment-variable path searching for files, possibly
   in subdirectories.

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

#ifndef PATHSEARCH_H
#define PATHSEARCH_H

#include "types.h"


/* Look for FILENAME in each of the directories given in the DIR_LIST
   array.  (Unless DIR_LIST is null, or FILENAME is absolute or
   explicitly relative, in which case we just look at FILENAME.)  Return
   the complete pathname of the first readable file found, or NULL.  */
extern string find_path_filename P2H(string filename, string *dir_list);

/* Return a NULL-terminated array of all the directory names in the
   value of the environment variable ENV_NAME (or DEFAULT_PATH if it is
   not set).  Each directory in the list ends with the directory separator,
   e.g., `/'.
   
   A leading or trailing path separator (e.g., `:') in the value of
   ENV_NAME is replaced by DEFAULT_PATH.
   
   If any element of the path ends with a double directory separator
   (e.g., `foo//'), it is replaced by all its subdirectories.  */
extern string *initialize_path_list P2H(string env_name, string default_path);

/* Replace a leading or trailing `:' in ENV_PATH with DEFAULT_PATH.  If
   neither is present, return ENV_PATH if that is non-null, else
   DEFAULT_PATH.  */
extern string expand_default P2H(string env_path, string default_path);

/* Replace a leading ~ or ~name in FILENAME with getenv ("HOME") or
   name's home directory.  */
extern string expand_tilde P1H(string filename);

#endif /* not PATHSEARCH_H */
