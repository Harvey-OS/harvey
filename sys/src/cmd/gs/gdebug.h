/* Copyright (C) 1989, 1992, 1993, 1994, 1995 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gdebug.h */
/* Debugging machinery definitions */

/* Define the array of debugging flags, indexed by character code. */
extern char gs_debug[128];
#define gs_debug_c(c)\
  ((c)>='a' && (c)<='z' ? gs_debug[c] | gs_debug[(c)^32] : gs_debug[c])
/* Define aliases for two specialized debugging flags */
/* that used to be separate variables. */
#define gs_alloc_debug gs_debug['@']
#define gs_log_errors gs_debug['#']

/* If debugging, direct all error output to gs_debug_out. */
#ifdef DEBUG
extern FILE *gs_debug_out;
#undef dstderr
#define dstderr gs_debug_out
#undef estderr
#define estderr gs_debug_out
#endif

/* Redefine eprintf_program_name and lprintf_file_and_line as procedures */
/* so one can set breakpoints on them. */
#undef eprintf_program_name
extern void eprintf_program_name(P2(FILE *, const char *));
#undef lprintf_file_and_line
extern void lprintf_file_and_line(P3(FILE *, const char *, int));

/* Insert code conditionally if debugging. */
#ifdef DEBUG
#  define do_debug(x) x
#else
#  define do_debug(x)
#endif

/* Debugging printout macros. */
#ifdef DEBUG
#  define if_debug0(c,s)\
    if (gs_debug_c(c)) dprintf(s)
#  define if_debug1(c,s,a1)\
    if (gs_debug_c(c)) dprintf1(s,a1)
#  define if_debug2(c,s,a1,a2)\
    if (gs_debug_c(c)) dprintf2(s,a1,a2)
#  define if_debug3(c,s,a1,a2,a3)\
    if (gs_debug_c(c)) dprintf3(s,a1,a2,a3)
#  define if_debug4(c,s,a1,a2,a3,a4)\
    if (gs_debug_c(c)) dprintf4(s,a1,a2,a3,a4)
#  define if_debug5(c,s,a1,a2,a3,a4,a5)\
    if (gs_debug_c(c)) dprintf5(s,a1,a2,a3,a4,a5)
#  define if_debug6(c,s,a1,a2,a3,a4,a5,a6)\
    if (gs_debug_c(c)) dprintf6(s,a1,a2,a3,a4,a5,a6)
#  define if_debug7(c,s,a1,a2,a3,a4,a5,a6,a7)\
    if (gs_debug_c(c)) dprintf7(s,a1,a2,a3,a4,a5,a6,a7)
#  define if_debug8(c,s,a1,a2,a3,a4,a5,a6,a7,a8)\
    if (gs_debug_c(c)) dprintf8(s,a1,a2,a3,a4,a5,a6,a7,a8)
#  define if_debug9(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9)\
    if (gs_debug_c(c)) dprintf9(s,a1,a2,a3,a4,a5,a6,a7,a8,a9)
#  define if_debug10(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)\
    if (gs_debug_c(c)) dprintf10(s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)
#  define if_debug11(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)\
    if (gs_debug_c(c)) dprintf11(s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)
#  define if_debug12(c,s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)\
    if (gs_debug_c(c)) dprintf12(s,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)
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
void debug_dump_bytes(P3(const byte *from, const byte *to, 
			 const char *msg));
void debug_dump_bitmap(P4(const byte *from, uint raster, uint height,
			  const char *msg));
void debug_print_string(P2(const byte *str, uint len));
