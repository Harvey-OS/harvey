/* Copyright (C) 2003 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ttconf.h,v 1.2 2003/11/13 22:41:41 igor Exp $ */
/* Configuration of the True Type interpreter. */
/* This file is copied from the FreeType project and modified to satisfy Ghostscript needs. */

#ifndef TTCONF_H
#define TTCONF_H

/* Define to empty if the keyword does not work.  */
#undef const

/* Define if you have a working `mmap' system call.  */
#undef HAVE_MMAP

/* Define if you have the ANSI C header files.  */
#undef STDC_HEADERS

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
#define WORDS_BIGENDIAN ARCH_IS_BIG_ENDIAN

/* Define if you have the getpagesize function.  */
#undef HAVE_GETPAGESIZE

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY

/* Define if you have the valloc function.  */
#undef HAVE_VALLOC

/* Define if you have the <fcntl.h> header file.  */
#undef HAVE_FCNTL_H

/* Define if you have the <unistd.h> header file.  */
#undef HAVE_UNISTD_H

/* Define if you have the <getopt.h> header file.  */
#undef HAVE_GETOPT_H

/* Define if you need <conio.h> for console I/O functions.  */
#undef HAVE_CONIO_H

/* command.com can't pipe stderr into a file; any message would be */
/* written into the graphics screen.                               */
#undef HAVE_PRINT_FUNCTION

/* The number of bytes in a int. */
#define SIZEOF_INT  (1 << ARCH_LOG2_SIZEOF_INT)

/* The number of bytes in a long.  */
#define SIZEOF_LONG (1 << ARCH_LOG2_SIZEOF_LONG)

/* Define if you have the basename function.  */
#undef HAVE_BASENAME


/* End of ft_conf.h */

#endif /* TTCONF_H */
