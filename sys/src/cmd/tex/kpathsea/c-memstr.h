/* c-memstr.h: memcpy, strchr, etc.

Copyright (C) 1992, 93, 94, 95, 97 Free Software Foundation, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef KPATHSEA_C_MEMSTR_H
#define KPATHSEA_C_MEMSTR_H

/* <X11/Xfuncs.h> tries to declare bcopy etc., which can only conflict.  */
#define _XFUNCS_H_

/* Just to be complete, we make both the system V/ANSI and the BSD
   versions of the string functions available.  */
#if defined (STDC_HEADERS) || defined (HAVE_STRING_H)
#if 0 /* OK, we'll try without; seems to be unnecessary now.  */
#define SYSV /* so <X11/Xos.h> knows not to include <strings.h> */
#endif /* 0 */
#include <string.h>

/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
#if !defined (STDC_HEADERS) && defined (HAVE_MEMORY_H)
#endif /* not STDC_HEADERS and HAVE_MEMORY_H */

/* Do not define these if we are not STDC_HEADERS, because in that
   case X11/Xos.h defines `strchr' to be `index'. */
#ifdef STDC_HEADERS
/* Let's hope that if index/rindex are defined, they're defined to the
   right thing.  */
#ifndef index
#define index strchr
#endif
#ifndef rindex
#define rindex strrchr
#endif
#endif /* STDC_HEADERS */

#ifndef HAVE_BCOPY
#ifndef bcmp
#define bcmp(s1, s2, len) memcmp ((s1), (s2), (len))
#endif
#ifndef bcopy
#define bcopy(from, to, len) memcpy ((to), (from), (len))
#endif
#ifndef bzero
#define bzero(s, len) memset ((s), 0, (len))
#endif
#endif /* not HAVE_BCOPY */

#else /* not (STDC_HEADERS or HAVE_STRING_H) */


#ifndef strchr
#define strchr index
#endif
#ifndef strrchr
#define strrchr rindex
#endif

#define memcmp(s1, s2, n) bcmp ((s1), (s2), (n))
#define memcpy(to, from, len) bcopy ((from), (to), (len))

extern char *strtok ();
extern char *strstr ();

#endif /* not (STDC_HEADERS or HAVE_STRING_H) */

#endif /* not KPATHSEA_C_MEMSTR_H */
