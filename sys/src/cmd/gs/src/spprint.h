/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: spprint.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Print values in ASCII form on a stream */

#ifndef spprint_INCLUDED
#  define spprint_INCLUDED

/* Define an opaque type for streams. */
#ifndef stream_DEFINED
#  define stream_DEFINED
typedef struct stream_s stream;
#endif

/* Put a character on a stream. */
#define pputc(s, c) spputc(s, c)

/* Put a byte array on a stream. */
int pwrite(P3(stream * s, const void *ptr, uint count));

/* Put a string on a stream. */
int pputs(P2(stream * s, const char *str));

/*
 * Print (a) floating point number(s) using a format.  This is needed
 * because %f format always prints a fixed number of digits after the
 * decimal point, and %g format may use %e format, which PDF disallows.
 * These functions return a pointer to the next %-element of the format, or
 * to the terminating 0.
 */
const char *pprintg1(P3(stream * s, const char *format, floatp v));
const char *pprintg2(P4(stream * s, const char *format, floatp v1, floatp v2));
const char *pprintg3(P5(stream * s, const char *format,
			floatp v1, floatp v2, floatp v3));
const char *pprintg4(P6(stream * s, const char *format,
			floatp v1, floatp v2, floatp v3, floatp v4));
const char *pprintg6(P8(stream * s, const char *format,
			floatp v1, floatp v2, floatp v3, floatp v4,
			floatp v5, floatp v6));

/*
 * The rest of these printing functions exist solely because the ANSI C
 * "standard" for functions with a variable number of arguments is not
 * implemented properly or consistently across compilers.
 */
/* Print (an) int value(s) using a format. */
const char *pprintd1(P3(stream * s, const char *format, int v));
const char *pprintd2(P4(stream * s, const char *format, int v1, int v2));
const char *pprintd3(P5(stream * s, const char *format,
			int v1, int v2, int v3));
const char *pprintd4(P6(stream * s, const char *format,
			int v1, int v2, int v3, int v4));

/* Print a long value using a format. */
const char *pprintld1(P3(stream * s, const char *format, long v));
const char *pprintld2(P4(stream * s, const char *format, long v1, long v2));
const char *pprintld3(P5(stream * s, const char *format,
			 long v1, long v2, long v3));

/* Print (a) string(s) using a format. */
const char *pprints1(P3(stream * s, const char *format, const char *str));
const char *pprints2(P4(stream * s, const char *format,
			const char *str1, const char *str2));
const char *pprints3(P5(stream * s, const char *format,
			const char *str1, const char *str2, const char *str3));

#endif /* spprint_INCLUDED */
