/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gsmisc.c */
/* Miscellaneous utilities for Ghostscript library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gconfigv.h"		/* for USE_ASM */
#include "gxfixed.h"

/* Define private replacements for stdin, stdout, and stderr. */
FILE *gs_stdin, *gs_stdout, *gs_stderr;
/* Ghostscript writes debugging output to gs_debug_out. */
/* We define gs_debug and gs_debug_out even if DEBUG isn't defined, */
/* so that we can compile individual modules with DEBUG set. */
char gs_debug[128];
FILE *gs_debug_out;

/* Define eprintf_program_name and lprintf_file_and_line as procedures */
/* so one can set breakpoints on them. */
void
eprintf_program_name(FILE *f, const char *program_name)
{	fprintf(f, "%s: ", program_name);
}
void
lprintf_file_and_line(FILE *f, const char *file, int line)
{	fprintf(f, "%s(%d): ", file, line);
}

/* ------ Substitutes for missing C library functions ------ */

#ifdef memory__need_memmove		/* see memory_.h */
/* Copy bytes like memcpy, guaranteed to handle overlap correctly. */
/* ANSI C defines the returned value as being the src argument, */
/* but with the const restriction removed! */
void *
gs_memmove(void *dest, const void *src, size_t len)
{	if ( !len )
		return (void *)src;
#define bdest ((byte *)dest)
#define bsrc ((const byte *)src)
	/* We use len-1 for comparisons because adding len */
	/* might produce an offset overflow on segmented systems. */
	if ( ptr_le(bdest, bsrc) )
	  {	register byte *end = bdest + (len - 1);
		if ( ptr_le(bsrc, end) )
		  {	/* Source overlaps destination from above. */
			register const byte *from = bsrc;
			register byte *to = bdest;
			for ( ; ; )
			  {	*to = *from;
				if ( to >= end )	/* faster than = */
				  return (void *)src;
				to++; from++;
			  }
		  }
	  }
	else
	  {	register const byte *from = bsrc + (len - 1);
		if ( ptr_le(bdest, from) )
		  {	/* Source overlaps destination from below. */
			register const byte *end = bsrc;
			register byte *to = bdest + (len - 1);
			for ( ; ; )
			  {	*to = *from;
				if ( from <= end )	/* faster than = */
				  return (void *)src;
				to--; from--;
			  }
		  }
	  }
#undef bdest
#undef bsrc
	/* No overlap, it's safe to use memcpy. */
	memcpy(dest, src, len);
	return (void *)src;
}
#endif

#ifdef memory__need_memchr		/* see memory_.h */
/* ch should obviously be char rather than int, */
/* but the ANSI standard declaration uses int. */
const char *
gs_memchr(const char *ptr, int ch, size_t len)
{	if ( len > 0 )
	{	register const char *p = ptr;
		register uint count = len;
		do { if ( *p == (char)ch ) return p; p++; } while ( --count );
	}
	return 0;
}
#endif

#ifdef memory__need_memset		/* see memory_.h */
/* ch should obviously be char rather than int, */
/* but the ANSI standard declaration uses int. */
void *
gs_memset(void *dest, register int ch, size_t len)
{	if ( ch == 0 )
		bzero(dest, len);
	else if ( len > 0 )
	{	register char *p = dest;
		register uint count = len;
		do { *p++ = (char)ch; } while ( --count );
	}
	return dest;
}
#endif

/* ------ Debugging support ------ */

/* Dump a region of memory. */
void
debug_dump_bytes(const byte *from, const byte *to, const char *msg)
{	const byte *p = from;
	if ( from < to && msg )
		dprintf1("%s:\n", msg);
	while ( p != to )
	   {	const byte *q = min(p + 16, to);
		dprintf1("0x%lx:", (ulong)p);
		while ( p != q ) dprintf1(" %02x", *p++);
		dputc('\n');
	   }
}

/* Dump a bitmap. */
void
debug_dump_bitmap(const byte *bits, uint raster, uint height, const char *msg)
{	uint y;
	const byte *data = bits;
	for ( y = 0; y < height; ++y, data += raster )
		debug_dump_bytes(data, data + raster, (y == 0 ? msg : NULL));
}

/* Print a string. */
void
debug_print_string(const byte *chrs, uint len)
{	uint i;
	for ( i = 0; i < len; i++ )
	  dputc(chrs[i]);
	fflush(dstderr);
}

/* ------ Arithmetic ------ */

/* Compute the GCD of two integers. */
int
igcd(int x, int y)
{	int c = x, d = y;
	if ( c < 0 ) c = -c;
	if ( d < 0 ) d = -d;
	while ( c != 0 && d != 0 )
	  if ( c > d ) c %= d;
	  else d %= c;
	return d + c;		/* at most one is non-zero */
}

#if defined(set_fmul2fixed_vars) && !USE_ASM

/*
 * Floating multiply with fixed result, for avoiding floating point in
 * common coordinate transformations.  Assumes IEEE representation,
 * 16-bit short, 32-bit long.  Optimized for the case where the first
 * operand has no more than 16 mantissa bits, e.g., where it is a user space
 * coordinate (which are often integers).
 *
 * The assembly language version of this code is actually faster than
 * the FPU, if the code is compiled with FPU_TYPE=0 (which requires taking
 * a trap on every FPU operation).  If there is no FPU, the assembly
 * language version of this code is over 10 times as fast as the emulated FPU.
 */
/* Some of the following code has been tweaked for the Borland 16-bit */
/* compiler.  The tweaks do not change the algorithms. */
#if arch_ints_are_short && !defined(FOR80386)
#  define SHORT_ARITH
#endif
int
set_fmul2fixed_(fixed *pr, long /*float*/ a, long /*float*/ b)
{
#ifdef SHORT_ARITH
#  define long_rsh8_ushort(x)\
    (((ushort)(x) >> 8) | ((ushort)((ulong)(x) >> 16) << 8))
#  define utemp ushort
#else
#  define long_rsh8_ushort(x) ((ushort)((x) >> 8))
#  define utemp ulong
#endif
	/* utemp may be either ushort or ulong.  This is OK because */
	/* we only use ma and mb in multiplications involving */
	/* a long or ulong operand. */
	utemp ma = long_rsh8_ushort(a) | 0x8000;
	utemp mb = long_rsh8_ushort(b) | 0x8000;
	int e = 260 + _fixed_shift - ((
		(((uint)((ulong)a >> 16)) & 0x7f80) +
		(((uint)((ulong)b >> 16)) & 0x7f80)
	  ) >> 7);
	ulong p1 = ma * (b & 0xff);
	ulong p = (ulong)ma * mb;
#define p_bits (sizeof(p) * 8)

	if ( (byte)a )		/* >16 mantissa bits */
	{	ulong p2 = (a & 0xff) * mb;
		p += ((((uint)(byte)a * (uint)(byte)b) >> 8) + p1 + p2) >> 8;
	}
	else
		p += p1 >> 8;
	if ( (uint)e < p_bits )		/* e = -1 is possible */
		p >>= e;
	else if ( e >= p_bits )		/* also detects a=0 or b=0 */
	  {	*pr = fixed_0;
		return 0;
	  }
	else if ( e >= -(p_bits - 1) || p >= 1L << (p_bits - 1 + e) )
		return_error(gs_error_limitcheck);
	else
		p <<= -e;
	*pr = ((a ^ b) < 0 ? -p : p);
	return 0;
}
int
set_dfmul2fixed_(fixed *pr, ulong /*double lo*/ xalo, long /*float*/ b, long /*double hi*/ xahi)
{
#ifdef SHORT_ARITH
#  define long_lsh3(x) ((((x) << 1) << 1) << 1)
#  define long_rsh(x,ng16) ((uint)((x) >> 16) >> (ng16 - 16))
#else
#  define long_lsh3(x) ((x) << 3)
#  define long_rsh(x,ng16) ((x) >> ng16)
#endif
	return set_fmul2fixed_(pr,
			       (xahi & 0xc0000000) +
			        (long_lsh3(xahi) & 0x3ffffff8) +
			        long_rsh(xalo, 29),
			       b);
}

#endif
