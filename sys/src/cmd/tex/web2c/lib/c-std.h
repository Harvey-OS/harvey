/* c-std.h: the first header files.

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

#ifndef C_STD_H
#define C_STD_H

/* Header files that essentially all of our sources need, and
   that all implementations have.  We include these first, to help with
   NULL being defined multiple times.  */
#include <math.h>
#include <stdio.h>

/* POSIX.1 says that <unistd.h> may require <sys/types.h>.  */
#include <sys/types.h>

/* This is the symbol that X uses to determine if <sys/types.h> has been
   read, so we define it.  */
#define __TYPES__

/* X uses this symbol to say whether we have <stddef.h> etc.  */
#ifndef STDC_HEADERS
#define X_NOT_STDC_ENV
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Include <stdlib.h> first to help avoid NULL redefinitions.  */
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#else
extern char *getenv ();
#ifndef ALLOC_RETURN_TYPE
#ifdef DOS
#define ALLOC_RETURN_TYPE void
#else
#define ALLOC_RETURN_TYPE char
#endif /* not DOS */
#endif /* not ALLOC_RETURN_TYPE */
extern ALLOC_RETURN_TYPE *calloc (), *malloc (), *realloc ();
#endif /* not STDC_HEADERS */

/* popen is part of POSIX.2, not POSIX.1.  So STDC_HEADERS isn't enough.  */
extern FILE *popen ();
extern double hypot ();

#endif /* not C_STD_H */
