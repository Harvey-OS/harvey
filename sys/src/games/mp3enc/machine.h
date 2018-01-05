/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *	Machine dependent defines/includes for LAME.
 *
 *	Copyright (c) 1999 A.L. Faber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LAME_MACHINE_H
#define LAME_MACHINE_H

#include <stdio.h>
#include <memory.h>

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if  defined(__riscos__)  &&  defined(FPA10)
# include "ymath.h"
#else
# include <math.h>
#endif

#include <ctype.h>

#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#if defined(macintosh)
# include <types.h>
# include <stat.h>
#else
# include <sys/types.h>
# include <sys/stat.h>
#endif

/*
 * 3 different types of pow() functions:
 *   - table lookup
 *   - pow()
 *   - exp()   on some machines this is claimed to be faster than pow()
 */

#define POW20(x)  pow20[x]
//#define POW20(x)  pow(2.0,((double)(x)-210)*.25)
//#define POW20(x)  exp( ((double)(x)-210)*(.25*LOG2) )

#define IPOW20(x)  ipow20[x]
//#define IPOW20(x)  exp( -((double)(x)-210)*.1875*LOG2 )
//#define IPOW20(x)  pow(2.0,-((double)(x)-210)*.1875)


/* in case this is used without configure */
#ifndef inline
# define inline
#endif
/* compatibility */
#define INLINE inline

#if defined(_MSC_VER)
# undef inline
# define inline _inline
#elif defined(__SASC) || defined(__GNUC__)
/* if __GNUC__ we always want to inline, not only if the user requests it */
# undef inline
# define inline __inline
#endif

#if    defined(_MSC_VER)
# pragma warning( disable : 4244 )
//# pragma warning( disable : 4305 )
#endif

/*
 * FLOAT    for variables which require at least 32 bits
 * FLOAT8   for variables which require at least 64 bits
 *
 * On some machines, 64 bit will be faster than 32 bit.  Also, some math
 * routines require 64 bit float, so setting FLOAT=float will result in a
 * lot of conversions.
 */

#if ( defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW32__) )
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# ifndef FLOAT
typedef float   FLOAT;
# endif
#endif

#ifndef FLOAT8  /* NOTE: RH: 7/00:  if FLOAT8=float, it breaks resampling and VBR code */
typedef double  FLOAT8;
#endif

/* Various integer types */

#if   defined _WIN32 && !defined __CYGWIN__
typedef unsigned char	u_char;
#elif defined __DECALPHA__
// do nothing
#elif defined OS_AMIGAOS
// do nothing
#elif defined __DJGPP__
typedef unsigned char	u_char;
#elif !defined __GNUC__  ||  defined __STRICT_ANSI__
typedef unsigned char	u_char;
#else
// do nothing
#endif

/* sample_t must be floating point, at least 32 bits */
typedef FLOAT     sample_t;
typedef sample_t  stereo_t [2];

#endif

/* end of machine.h */
