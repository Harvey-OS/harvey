/* c-memstr.h: memcpy, strchr, etc.

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

#ifndef C_MEMSTR_H
#define C_MEMSTR_H

#include "c-std.h"

#if defined (USG) || defined (STDC_HEADERS)
#include <string.h>
#define bcopy(from, to, len) memcpy ((to), (from), (len))
#define bzero(s, l) memset ((s), 0, (l))
#else
#include <strings.h>
#define strchr index
#define strrchr rindex
#ifndef NEED_MEMORY_H
#define memcpy(to, from, len) bcopy ((from), (to), (len))
#endif
extern char *strtok ();
extern char *strstr ();
#endif /* not USG or STDC_HEADERS */

/* SunOS 4.1 declares memchr in <memory.h>, not <string.h>.  I don't
   understand why.  */
#ifdef NEED_MEMORY_H
#include <memory.h>
#endif

#endif /* not C_MEMSTR_H */
