/* Copyright (C) 1989, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: genarch.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/* Generate a header file (arch.h) with parameters */
/* reflecting the machine architecture and compiler characteristics. */

#include "stdpre.h"
#include <ctype.h>
#include <stdio.h>
/*
 * In theory, not all systems provide <string.h>, or declare memset in
 * that header file, but at this point I don't think we care about any
 * that don't.
 */
#include <string.h>
#include <time.h>

/* We should write the result on stdout, but the original Turbo C 'make' */
/* can't handle output redirection (sigh). */

private void
section(FILE * f, const char *str)
{
    fprintf(f, "\n\t /* ---------------- %s ---------------- */\n\n", str);
}

private clock_t
time_clear(char *buf, int bsize, int nreps)
{
    clock_t t = clock();
    int i;

    for (i = 0; i < nreps; ++i)
	memset(buf, 0, bsize);
    return clock() - t;
}

private void
define(FILE *f, const char *str)
{
    fprintf(f, "#define %s ", str);
}

private void
define_int(FILE *f, const char *str, int value)
{
    fprintf(f, "#define %s %d\n", str, value);
}

const char ff_str[] = "ffffffffffffffff";	/* 8 bytes */

int
main(int argc, char *argv[])
{
    char *fname = argv[1];
    long one = 1;
    int ff_strlen = sizeof(ff_str) - 1;
    struct {
	char c;
	short s;
    } ss;
    struct {
	char c;
	int i;
    } si;
    struct {
	char c;
	long l;
    } sl;
    struct {
	char c;
	char *p;
    } sp;
    struct {
	char c;
	float f;
    } sf;
    struct {
	char c;
	double d;
    } sd;
    static int log2s[17] =
    {0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4};
    long lm1 = -1;
    long lr1 = lm1 >> 1, lr2 = lm1 >> 2;
    unsigned long um1 = ~(unsigned long)0;
    int im1 = -1;
    int ir1 = im1 >> 1, ir2 = im1 >> 2;
    union {
	long l;
	char *p;
    } pl0, pl1;
    int ars;
    int lwidth = size_of(long) * 8;
    union {
	float f;
	int i;
	long l;
    } f0 , f1, fm1;
    int floats_are_IEEE;
    FILE *f = fopen(fname, "w");

    if (f == NULL) {
	fprintf(stderr, "genarch.c: can't open %s for writing\n", fname);
	return exit_FAILED;
    }
    fprintf(f, "/* Parameters derived from machine and compiler architecture */\n");
    /* We have to test the size dynamically here, */
    /* because the preprocessor can't evaluate sizeof. */
    f0.f = 0.0, f1.f = 1.0, fm1.f = -1.0;
    floats_are_IEEE =
	(size_of(float) == size_of(int) ?
	 f0.i == 0 && f1.i == (int)0x3f800000 && fm1.i == (int)0xbf800000 :
	 f0.l == 0 && f1.l == 0x3f800000L && fm1.l == 0xbf800000L);

    section(f, "Scalar alignments");

#define OFFSET_IN(s, e) (int)((char *)&s.e - (char *)&s)
    define_int(f, "ARCH_ALIGN_SHORT_MOD", OFFSET_IN(ss, s));
    define_int(f, "ARCH_ALIGN_INT_MOD", OFFSET_IN(si, i));
    define_int(f, "ARCH_ALIGN_LONG_MOD", OFFSET_IN(sl, l));
    define_int(f, "ARCH_ALIGN_PTR_MOD", OFFSET_IN(sp, p));
    define_int(f, "ARCH_ALIGN_FLOAT_MOD", OFFSET_IN(sf, f));
    define_int(f, "ARCH_ALIGN_DOUBLE_MOD", OFFSET_IN(sd, d));
#undef OFFSET_IN

    section(f, "Scalar sizes");

    define_int(f, "ARCH_LOG2_SIZEOF_SHORT", log2s[size_of(short)]);
    define_int(f, "ARCH_LOG2_SIZEOF_INT", log2s[size_of(int)]);
    define_int(f, "ARCH_LOG2_SIZEOF_LONG", log2s[size_of(long)]);
    define_int(f, "ARCH_SIZEOF_PTR", size_of(char *));
    define_int(f, "ARCH_SIZEOF_FLOAT", size_of(float));
    define_int(f, "ARCH_SIZEOF_DOUBLE", size_of(double));
    if (floats_are_IEEE) {
	define_int(f, "ARCH_FLOAT_MANTISSA_BITS", 24);
	define_int(f, "ARCH_DOUBLE_MANTISSA_BITS", 53);
    } else {
	/*
	 * There isn't any general way to compute the number of mantissa
	 * bits accurately, especially if the machine uses hex rather
	 * than binary exponents.  Use conservative values, assuming
	 * the exponent is stored in a 16-bit word of its own.
	 */
	define_int(f, "ARCH_FLOAT_MANTISSA_BITS", sizeof(float) * 8 - 17);
	define_int(f, "ARCH_DOUBLE_MANTISSA_BITS", sizeof(double) * 8 - 17);
    }

    section(f, "Unsigned max values");

#define PRINT_MAX(str, typ, tstr, l)\
  define(f, str);\
  fprintf(f, "((%s)0x%s%s + (%s)0)\n",\
    tstr, ff_str + ff_strlen - size_of(typ) * 2, l, tstr)
    PRINT_MAX("ARCH_MAX_UCHAR", unsigned char, "unsigned char", "");
    PRINT_MAX("ARCH_MAX_USHORT", unsigned short, "unsigned short", "");
    /*
     * For uint and ulong, a different approach is required to keep gcc
     * with -Wtraditional from spewing out pointless warnings.
     */
    define(f, "ARCH_MAX_UINT");
    fprintf(f, "((unsigned int)~0 + (unsigned int)0)\n");
    define(f, "ARCH_MAX_ULONG");
    fprintf(f, "((unsigned long)~0L + (unsigned long)0)\n");
#undef PRINT_MAX

    section(f, "Cache sizes");

    /*
     * Determine the primary and secondary cache sizes by looking for a
     * non-linearity in the time required to fill blocks with memset.
     */
    {
#define MAX_BLOCK (1 << 20)
	static char buf[MAX_BLOCK];
	int bsize = 1 << 10;
	int nreps = 1;
	clock_t t = 0;
	clock_t t_eps;

	/*
	 * Increase the number of repetitions until the time is
	 * long enough to exceed the likely uncertainty.
	 */

	while ((t = time_clear(buf, bsize, nreps)) == 0)
	    nreps <<= 1;
	t_eps = t;
	while ((t = time_clear(buf, bsize, nreps)) < t_eps * 10)
	    nreps <<= 1;

	/*
	 * Increase the block size until the time jumps non-linearly.
	 */
	for (; bsize <= MAX_BLOCK;) {
	    clock_t dt = time_clear(buf, bsize, nreps);

	    if (dt > t + (t >> 1)) {
		t = dt;
		break;
	    }
	    bsize <<= 1;
	    nreps >>= 1;
	    if (nreps == 0)
		nreps = 1, t <<= 1;
	}
	define_int(f, "ARCH_CACHE1_SIZE", bsize >> 1);
	/*
	 * Do the same thing a second time for the secondary cache.
	 */
	if (nreps > 1)
	    nreps >>= 1, t >>= 1;
	for (; bsize <= MAX_BLOCK;) {
	    clock_t dt = time_clear(buf, bsize, nreps);

	    if (dt > t * 1.25) {
		t = dt;
		break;
	    }
	    bsize <<= 1;
	    nreps >>= 1;
	    if (nreps == 0)
		nreps = 1, t <<= 1;
	}
	define_int(f, "ARCH_CACHE2_SIZE", bsize >> 1);
    }

    section(f, "Miscellaneous");

    define_int(f, "ARCH_IS_BIG_ENDIAN", 1 - *(char *)&one);
    pl0.l = 0;
    pl1.l = -1;
    define_int(f, "ARCH_PTRS_ARE_SIGNED", (pl1.p < pl0.p));
    define_int(f, "ARCH_FLOATS_ARE_IEEE", (floats_are_IEEE ? 1 : 0));

    /*
     * There are three cases for arithmetic right shift:
     * always correct, correct except for right-shifting a long by 1
     * (a bug in some versions of the Turbo C compiler), and
     * never correct.
     */
    ars = (lr2 != -1 || ir1 != -1 || ir2 != -1 ? 0 :
	   lr1 != -1 ? 1 :	/* Turbo C problem */
	   2);
    define_int(f, "ARCH_ARITH_RSHIFT", ars);
    /*
     * Some machines can't handle a variable shift by
     * the full width of a long.
     */
    define_int(f, "ARCH_CAN_SHIFT_FULL_LONG", um1 >> lwidth == 0);
    /*
     * Determine whether dividing a negative integer by a positive one
     * takes the floor or truncates toward zero.
     */
    define_int(f, "ARCH_DIV_NEG_POS_TRUNCATES", im1 / 2 == 0);

/* ---------------- Done. ---------------- */

    fclose(f);
    return exit_OK;
}
