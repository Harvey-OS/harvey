/* c-minmax.h: define INT_MIN, etc.

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

#ifndef C_MINMAX_H
#define C_MINMAX_H

#include "c-limits.h"

/* Declared in <limits.h> on ANSI C systems.  */
#ifndef INT_MIN
#define INT_MIN (-2147483647-1)
#endif

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#ifndef UINT_MAX
#define UINT_MAX 4294967295
#endif

/* Declared in <float.h> on ANSI C systems.  */
#ifndef DBL_MIN
#define DBL_MIN 1e-37
#endif

#ifndef DBL_MAX
#define DBL_MAX 1e+37
#endif
#ifndef FLT_MIN
#define FLT_MIN 1e-37
#endif

#ifndef FLT_MAX
#define FLT_MAX 1e-37
#endif

#endif /* not C_MINMAX_H */
