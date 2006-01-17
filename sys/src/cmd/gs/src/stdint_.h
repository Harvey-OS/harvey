/* Copyright (C) 2003-2004 artofcode LLC.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: stdint_.h,v 1.5 2004/06/17 21:42:53 giles Exp $ */
/* Generic substitute for stdint.h */

#ifndef stdint__INCLUDED
#  define stdint__INCLUDED

/*
 * ensure our standard defines have been included
 */
#include "std.h"

/* Define some stdint.h types. The jbig2dec headers and ttf bytecode
 * interpreter require these and they're generally useful to have around
 * now that there's a standard.
 */

/* Some systems are guaranteed to have stdint.h
 * but don't use the autoconf detection
 */
#ifndef HAVE_STDINT_H
# ifdef __MACOS__
#   define HAVE_STDINT_H
# endif
#endif

/* try a generic header first */
#if defined(HAVE_STDINT_H)
# include <stdint.h>
# define STDINT_TYPES_DEFINED
#elif defined(SYS_TYPES_HAS_STDINT_TYPES)
  /* std.h will have included sys/types.h */
# define STDINT_TYPES_DEFINED
#endif

/* try platform-specific defines */
#ifndef STDINT_TYPES_DEFINED
# if defined(__WIN32__) /* MSVC currently doesn't provide C99 headers */
   typedef signed char             int8_t;
   typedef short int               int16_t;
   typedef int                     int32_t;
   typedef __int64                 int64_t;
   typedef unsigned char           uint8_t;
   typedef unsigned short int      uint16_t;
   typedef unsigned int            uint32_t;
   typedef unsigned __int64        uint64_t;
#  define STDINT_TYPES_DEFINED
# elif defined(__VMS) /* OpenVMS provides these types in inttypes.h */
#  include <inttypes.h>
#  define STDINT_TYPES_DEFINED
# elif defined(__CYGWIN__)
   /* Cygwin defines the signed versions in sys/types.h */
   /* but uses a u_ prefix for the unsigned versions */
   typedef u_int8_t                uint8_t;
   typedef u_int16_t               uint16_t;
   typedef u_int32_t               uint32_t;
   typedef u_int64_t               uint64_t;
#  define STDINT_TYPES_DEFINED
# endif
   /* other archs may want to add defines here, 
      or use the fallbacks in std.h */
#endif

/* fall back to tests based on arch.h */
#ifndef STDINT_TYPES_DEFINED
/* 8 bit types */
# if ARCH_SIZEOF_CHAR == 1
typedef signed char int8_t;
typedef unsigned char uint8_t;
# endif
/* 16 bit types */
# if ARCH_SIZEOF_SHORT == 2
typedef signed short int16_t;
typedef unsigned short uint16_t;
# else
#  if ARCH_SIZEOF_INT == 2
typedef signed int int16_t;
typedef unsigned int uint16_t;
#  endif
# endif
/* 32 bit types */
# if ARCH_SIZEOF_INT == 4
typedef signed int int32_t;
typedef unsigned int uint32_t;
# else
#  if ARCH_SIZEOF_LONG == 4
typedef signed long int32_t;
typedef unsigned long uint32_t;
#  else
#   if ARCH_SIZEOF_SHORT == 4
typedef signed short int32_t;
typedef unsigned short uint32_t;
#   endif
#  endif
# endif
/* 64 bit types */
# if ARCH_SIZEOF_INT == 8
typedef signed int int64_t;
typedef unsigned int uint64_t;
# else
#  if ARCH_SIZEOF_LONG == 8
typedef signed long int64_t;
typedef unsigned long uint64_t;
#  else
#   ifdef ARCH_SIZEOF_LONG_LONG
#    if ARCH_SIZEOF_LONG_LONG == 8
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#    endif
#   endif
#  endif
# endif
#  define STDINT_TYPES_DEFINED
#endif /* STDINT_TYPES_DEFINED */

#endif /* stdint__INCLUDED */
