/* c-pathchar.h: define the characters which separate components of
   pathnames and environment variable paths.

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

#ifndef C_PATHCHAR_H
#define C_PATHCHAR_H

/* What separates pathname components?  */
#ifndef PATH_SEP
#ifdef VMS
#define PATH_SEP ':'
#define PATH_SEP_STRING ":"
#else
#ifdef DOS
#define PATH_SEP '\\'
#define PATH_SEP_STRING "\\"
#else
#ifdef VMCMS
#define PATH_SEP ' '
#define PATH_SEP_STRING " "
#else
#define PATH_SEP '/'
#define PATH_SEP_STRING "/"
#endif /* not VM/CMS */
#endif /* not DOS */
#endif /* not VMS */
#endif /* not PATH_SEP */

/* What separates elements in environment variable path lists?  */
#ifndef PATH_DELIMITER
#ifdef VMS
#define PATH_DELIMITER ','
#define PATH_DELIMITER_STRING ","
#else
#ifdef DOS
#define PATH_DELIMITER ';'
#define PATH_DELIMITER_STRING ";"
#else
#ifdef VMCMS
#define PATH_DELIMITER ' '
#define PATH_DELIMITER_STRING " "
#else
#define PATH_DELIMITER ':'
#define PATH_DELIMITER_STRING ":"
#endif /* not VM/CMS */
#endif /* not DOS */
#endif /* not VMS */
#endif /* not PATH_DELIMITER */

#endif /* not C_PATHCHAR_H */
