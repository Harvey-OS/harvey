/* Copyright (C) 1989, 1992, 1993, 1994, 1995, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: std.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Standard definitions for Aladdin Enterprises code */

#ifndef std_INCLUDED
#  define std_INCLUDED

#include "stdpre.h"

/* Include the architecture definitions. */
#include "arch.h"

/*
 * Define lower-case versions of the architecture parameters for backward
 * compatibility.
 */
#define arch_align_short_mod ARCH_ALIGN_SHORT_MOD
#define arch_align_int_mod ARCH_ALIGN_INT_MOD
#define arch_align_long_mod ARCH_ALIGN_LONG_MOD
#define arch_align_ptr_mod ARCH_ALIGN_PTR_MOD
#define arch_align_float_mod ARCH_ALIGN_FLOAT_MOD
#define arch_align_double_mod ARCH_ALIGN_DOUBLE_MOD
#define arch_log2_sizeof_short ARCH_LOG2_SIZEOF_SHORT
#define arch_log2_sizeof_int ARCH_LOG2_SIZEOF_INT
#define arch_log2_sizeof_long ARCH_LOG2_SIZEOF_LONG
#define arch_sizeof_ptr ARCH_SIZEOF_PTR
#define arch_sizeof_float ARCH_SIZEOF_FLOAT
#define arch_sizeof_double ARCH_SIZEOF_DOUBLE
#define arch_float_mantissa_bits ARCH_FLOAT_MANTISSA_BITS
#define arch_double_mantissa_bits ARCH_DOUBLE_MANTISSA_BITS
#define arch_max_uchar ARCH_MAX_UCHAR
#define arch_max_ushort ARCH_MAX_USHORT
#define arch_max_uint ARCH_MAX_UINT
#define arch_max_ulong ARCH_MAX_ULONG
#define arch_cache1_size ARCH_CACHE1_SIZE
#define arch_cache2_size ARCH_CACHE2_SIZE
#define arch_is_big_endian ARCH_IS_BIG_ENDIAN
#define arch_ptrs_are_signed ARCH_PTRS_ARE_SIGNED
#define arch_floats_are_IEEE ARCH_FLOATS_ARE_IEEE
#define arch_arith_rshift ARCH_ARITH_RSHIFT
#define arch_can_shift_full_long ARCH_CAN_SHIFT_FULL_LONG

/* Define integer data type sizes in terms of log2s. */
#define ARCH_SIZEOF_SHORT (1 << ARCH_LOG2_SIZEOF_SHORT)
#define ARCH_SIZEOF_INT (1 << ARCH_LOG2_SIZEOF_INT)
#define ARCH_SIZEOF_LONG (1 << ARCH_LOG2_SIZEOF_LONG)
#define ARCH_INTS_ARE_SHORT (ARCH_SIZEOF_INT == ARCH_SIZEOF_SHORT)
/* Backward compatibility */
#define arch_sizeof_short ARCH_SIZEOF_SHORT
#define arch_sizeof_int ARCH_SIZEOF_INT
#define arch_sizeof_long ARCH_SIZEOF_LONG
#define arch_ints_are_short ARCH_INTS_ARE_SHORT

/* Define whether we are on a large- or small-memory machine. */
/* Currently, we assume small memory and 16-bit ints are synonymous. */
#define ARCH_SMALL_MEMORY (ARCH_SIZEOF_INT <= 2)
/* Backward compatibility */
#define arch_small_memory ARCH_SMALL_MEMORY

/* Define unsigned 16- and 32-bit types.  These are needed in */
/* a surprising number of places that do bit manipulation. */
#if arch_sizeof_short == 2	/* no plausible alternative! */
typedef ushort bits16;
#endif
#if arch_sizeof_int == 4
typedef uint bits32;
#else
# if arch_sizeof_long == 4
typedef ulong bits32;
# endif
#endif

/* Minimum and maximum values for the signed types. */
/* Avoid casts, to make them acceptable to strict ANSI compilers. */
#define min_short (-1 << (arch_sizeof_short * 8 - 1))
#define max_short (~min_short)
#define min_int (-1 << (arch_sizeof_int * 8 - 1))
#define max_int (~min_int)
#define min_long (-1L << (arch_sizeof_long * 8 - 1))
#define max_long (~min_long)

/*
 * The maximum values for the unsigned types are defined in arch.h,
 * because so many compilers handle unsigned constants wrong.
 * In particular, most of the DEC VMS compilers incorrectly sign-extend
 * short unsigned constants (but not short unsigned variables) when
 * widening them to longs.  We program around this on a case-by-case basis.
 * Some compilers don't truncate constants when they are cast down.
 * The UTek compiler does special weird things of its own.
 * All the rest (including gcc on all platforms) do the right thing.
 */
#define max_uchar arch_max_uchar
#define max_ushort arch_max_ushort
#define max_uint arch_max_uint
#define max_ulong arch_max_ulong

/* Minimum and maximum values for pointers. */
#if arch_ptrs_are_signed
#  define min_ptr min_long
#  define max_ptr max_long
#else
#  define min_ptr ((ulong)0)
#  define max_ptr max_ulong
#endif

/* Define a reliable arithmetic right shift. */
/* Must use arith_rshift_1 for a shift by a literal 1. */
#define arith_rshift_slow(x,n) ((x) < 0 ? ~(~(x) >> (n)) : (x) >> (n))
#if arch_arith_rshift == 2
#  define arith_rshift(x,n) ((x) >> (n))
#  define arith_rshift_1(x) ((x) >> 1)
#else
#if arch_arith_rshift == 1	/* OK except for n=1 */
#  define arith_rshift(x,n) ((x) >> (n))
#  define arith_rshift_1(x) arith_rshift_slow(x,1)
#else
#  define arith_rshift(x,n) arith_rshift_slow(x,n)
#  define arith_rshift_1(x) arith_rshift_slow(x,1)
#endif
#endif

/*
 * Standard error printing macros.
 * Use dprintf for messages that just go to dstderr;
 * dlprintf for messages to dsterr with optional with file name (and,
 * if available, line number);
 * eprintf for error messages to estderr that include the program name;
 * lprintf for debugging messages that should include line number info.
 * Since we intercept fprintf to redirect output under MS Windows,
 * we have to define dputc and dputs in terms of fprintf also.
 */

/*
 * We would prefer not to include stdio.h here, but we need it for
 * the FILE * argument of the printing procedures.
 */
#include <stdio.h>

/* dstderr and estderr may be redefined. */
#define dstderr stderr
#define estderr stderr

/* Print the program line # for debugging. */
#if __LINE__			/* compiler provides it */
void dprintf_file_and_line(P3(FILE *, const char *, int));
#  define _dpl dprintf_file_and_line(estderr, __FILE__, __LINE__),
#else
void dprintf_file_only(P2(FILE *, const char *));
#  define _dpl dprintf_file_only(estderr, __FILE__),
#endif

#define dputc(chr) dprintf1("%c", chr)
#define dlputc(chr) dlprintf1("%c", chr)
#define dputs(str) dprintf1("%s", str)
#define dlputs(str) dlprintf1("%s", str)
#define dprintf(str)\
  fprintf(dstderr, str)
#define dlprintf(str)\
  (_dpl dprintf(str))
#define dprintf1(str,arg1)\
  fprintf(dstderr, str, arg1)
#define dlprintf1(str,arg1)\
  (_dpl dprintf1(str, arg1))
#define dprintf2(str,arg1,arg2)\
  fprintf(dstderr, str, arg1, arg2)
#define dlprintf2(str,arg1,arg2)\
  (_dpl dprintf2(str, arg1, arg2))
#define dprintf3(str,arg1,arg2,arg3)\
  fprintf(dstderr, str, arg1, arg2, arg3)
#define dlprintf3(str,arg1,arg2,arg3)\
  (_dpl dprintf3(str, arg1, arg2, arg3))
#define dprintf4(str,arg1,arg2,arg3,arg4)\
  fprintf(dstderr, str, arg1, arg2, arg3, arg4)
#define dlprintf4(str,arg1,arg2,arg3,arg4)\
  (_dpl dprintf4(str, arg1, arg2, arg3, arg4))
#define dprintf5(str,arg1,arg2,arg3,arg4,arg5)\
  fprintf(dstderr, str, arg1, arg2, arg3, arg4, arg5)
#define dlprintf5(str,arg1,arg2,arg3,arg4,arg5)\
  (_dpl dprintf5(str, arg1, arg2, arg3, arg4, arg5))
#define dprintf6(str,arg1,arg2,arg3,arg4,arg5,arg6)\
  fprintf(dstderr, str, arg1, arg2, arg3, arg4, arg5, arg6)
#define dlprintf6(str,arg1,arg2,arg3,arg4,arg5,arg6)\
  (_dpl dprintf6(str, arg1, arg2, arg3, arg4, arg5, arg6))
#define dprintf7(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7)\
  fprintf(dstderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define dlprintf7(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7)\
  (_dpl dprintf7(str, arg1, arg2, arg3, arg4, arg5, arg6, arg7))
#define dprintf8(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)\
  fprintf(dstderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#define dlprintf8(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)\
  (_dpl dprintf8(str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8))
#define dprintf9(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)\
  fprintf(dstderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
#define dlprintf9(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)\
  (_dpl dprintf9(str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9))
#define dprintf10(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10)\
  fprintf(dstderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
#define dlprintf10(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10)\
  (_dpl dprintf10(str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10))
#define dprintf11(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11)\
  fprintf(dstderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
#define dlprintf11(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11)\
  (_dpl dprintf11(str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11))
#define dprintf12(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11,arg12)\
  fprintf(dstderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12)
#define dlprintf12(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11,arg12)\
  (_dpl dprintf12(str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12))

void printf_program_ident(P3(FILE *f, const char *program_name,
			     long revision_number));
void eprintf_program_ident(P3(FILE *f, const char *program_name,
			      long revision_number));
const char *gs_program_name(P0());
long gs_revision_number(P0());

#define _epi eprintf_program_ident(estderr, gs_program_name(),\
				   gs_revision_number()),

#define eprintf(str)\
  (_epi fprintf(estderr, str))
#define eprintf1(str,arg1)\
  (_epi fprintf(estderr, str, arg1))
#define eprintf2(str,arg1,arg2)\
  (_epi fprintf(estderr, str, arg1, arg2))
#define eprintf3(str,arg1,arg2,arg3)\
  (_epi fprintf(estderr, str, arg1, arg2, arg3))
#define eprintf4(str,arg1,arg2,arg3,arg4)\
  (_epi fprintf(estderr, str, arg1, arg2, arg3, arg4))
#define eprintf5(str,arg1,arg2,arg3,arg4,arg5)\
  (_epi fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5))
#define eprintf6(str,arg1,arg2,arg3,arg4,arg5,arg6)\
  (_epi fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6))
#define eprintf7(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7)\
  (_epi fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7))
#define eprintf8(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)\
  (_epi fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8))
#define eprintf9(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)\
  (_epi fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9))
#define eprintf10(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10)\
  (_epi fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10))

#if __LINE__			/* compiler provides it */
void lprintf_file_and_line(P3(FILE *, const char *, int));
#  define _epl _epi lprintf_file_and_line(estderr, __FILE__, __LINE__),
#else
void lprintf_file_only(P2(FILE *, const char *));
#  define _epl _epi lprintf_file_only(estderr, __FILE__)
#endif

#define lprintf(str)\
  (_epl fprintf(estderr, str))
#define lprintf1(str,arg1)\
  (_epl fprintf(estderr, str, arg1))
#define lprintf2(str,arg1,arg2)\
  (_epl fprintf(estderr, str, arg1, arg2))
#define lprintf3(str,arg1,arg2,arg3)\
  (_epl fprintf(estderr, str, arg1, arg2, arg3))
#define lprintf4(str,arg1,arg2,arg3,arg4)\
  (_epl fprintf(estderr, str, arg1, arg2, arg3, arg4))
#define lprintf5(str,arg1,arg2,arg3,arg4,arg5)\
  (_epl fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5))
#define lprintf6(str,arg1,arg2,arg3,arg4,arg5,arg6)\
  (_epl fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6))
#define lprintf7(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7)\
  (_epl fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7))
#define lprintf8(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)\
  (_epl fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8))
#define lprintf9(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)\
  (_epl fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9))
#define lprintf10(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10)\
  (_epl fprintf(estderr, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10))

/*
 * Define the prototype for module initialization procedures.  This is not
 * a very good place to define this, but we can't find a better one.
 */
#ifndef gs_memory_DEFINED
#  define gs_memory_DEFINED
typedef struct gs_memory_s gs_memory_t;
#endif
#define init_proc(proc)\
  int proc(P1(gs_memory_t *))

#endif /* std_INCLUDED */
