/* Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdebug.h,v 1.6 2002/06/16 08:45:42 lpd Exp $ */
/* Debugging machinery definitions */

#ifndef gdebug_INCLUDED
#  define gdebug_INCLUDED

/*
 * The compile-time DEBUG symbol determines whether debugging/tracing
 * code is included in the compiled code.  DEBUG may be set or not set
 * independently for every compilation; however, a small amount of support
 * machinery in gsmisc.c is always included in the executable, just
 * in case *some* file was compiled with DEBUG set.
 *
 * When DEBUG is set, it does not cause debugging/tracing printout to occur.
 * Rather, it includes code that produces such printout *if* (a) given
 * one(s) of 128 debugging flags is set.  In this way, one can selectively
 * turn printout on and off during debugging.  (In fact, we even provide a
 * PostScript operator, .setdebug, that does this.)
 *
 * The debugging flags are normally indexed by character code.  This is more
 * than a convention: gs_debug_c, which tests whether a given flag is set,
 * considers that if a flag named by a given upper-case letter is set, the
 * flag named by the corresponding lower-case letter is also set.
 *
 * If the output selected by a given flag can be printed by a single
 * printf, the conventional way to produce the output is
 *      if_debugN('x', "...format...", v1, ..., vN);
 * Usually the flag appears in the output explicitly:
 *      if_debugN('x', "[x]...format...", v1, ..., vN);
 * If the output is more complex, the conventional way to produce the
 * output is
 *      if ( gs_debug_c('x') ) {
 *        ... start each line with dlprintfN(...)
 *        ... produce additional output within a line with dprintfN(...)
 * } */

/* Define the array of debugging flags, indexed by character code. */
extern char gs_debug[128];
bool gs_debug_c(int /*char */ );

/*
 * Define an alias for a specialized debugging flag
 * that used to be a separate variable.
 */
#define gs_log_errors gs_debug['#']

/* If debugging, direct all error output to gs_debug_out. */
extern FILE *gs_debug_out;

#ifdef DEBUG
#undef dstderr
#define dstderr gs_debug_out
#undef estderr
#define estderr gs_debug_out
#endif

/* Debugging printout macros. */
#ifdef DEBUG
#  define if_debug0(c,s)\
    BEGIN if (gs_debug_c(c)) dlprintf(s); END
#  define if_debug1(c,s,a1)\
    BEGIN if (gs_debug_c(c)) dlprintf1(s,a1); END
#  define if_debug2(c,s,a1,a2)\
    BEGIN if (gs_debug_c(c)) dlprintf2(s,a1,a2); END
#  define if_debug3(c,s,a1,a2,a3)\
    BEGIN if (gs_debug_c(c)) dlprintf3(s,a1,a2,a3); END
#  define if_debug4(c,s,a1,a2,a3,a4)\
    BEGIN if (gs_debug_c(c)) dlprintf4(s,a1,a2,a3,a4); END
#  define if_debug5(c,s,a1,a2,a3,a4,a5)\
    BEGIN if (gs_debug_c(c)) dlprintf5(s,a1,a2,a3,a4,a5); END
#  define if_debug6(c,s,a1,a2,a3,a4,a5,a6)\
    BEGIN if (gs_debug_c(c)) dlprintf6(s,a1,a2,a3,a4,a5,a6); END
#  define if_debug7(c,s,a1,a2,a3,a4,a5,a6,a7)\
    BEGIN if (gs_debug_c(c)) dlprintf7(s,a1,a2,a3,a4,a5,a6,a7); END
#  define if_debug8(c,s,a1,a2,a3,a4,a5,a6,a7,a8)\
    BEGIN if (gs_debug_c(c)) dlprintf8(s,a1,a2,a3,a4,a5,a6,a7,a8); END
#  define if_debug9(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9)\
    BEGIN if (gs_debug_c(c)) dlprintf9(s,a1,a2,a3,a4,a5,a6,a7,a8,a9); END
#  define if_debug10(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)\
    BEGIN if (gs_debug_c(c)) dlprintf10(s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10); END
#  define if_debug11(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)\
    BEGIN if (gs_debug_c(c)) dlprintf11(s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11); END
#  define if_debug12(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)\
    BEGIN if (gs_debug_c(c)) dlprintf12(s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12); END
#else
#  define if_debug0(c,s) DO_NOTHING
#  define if_debug1(c,s,a1) DO_NOTHING
#  define if_debug2(c,s,a1,a2) DO_NOTHING
#  define if_debug3(c,s,a1,a2,a3) DO_NOTHING
#  define if_debug4(c,s,a1,a2,a3,a4) DO_NOTHING
#  define if_debug5(c,s,a1,a2,a3,a4,a5) DO_NOTHING
#  define if_debug6(c,s,a1,a2,a3,a4,a5,a6) DO_NOTHING
#  define if_debug7(c,s,a1,a2,a3,a4,a5,a6,a7) DO_NOTHING
#  define if_debug8(c,s,a1,a2,a3,a4,a5,a6,a7,a8) DO_NOTHING
#  define if_debug9(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9) DO_NOTHING
#  define if_debug10(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) DO_NOTHING
#  define if_debug11(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) DO_NOTHING
#  define if_debug12(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) DO_NOTHING
#endif

/* Debugging support procedures in gsmisc.c */
void debug_dump_bytes(const byte * from, const byte * to,
		      const char *msg);
void debug_dump_bitmap(const byte * from, uint raster, uint height,
		       const char *msg);
void debug_print_string(const byte * str, uint len);
void debug_print_string_hex(const byte * str, uint len);

#endif /* gdebug_INCLUDED */
