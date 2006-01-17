/* pop (%!) .skipeof

   Copyright (C) 1989, 1995, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: bench.c,v 1.7 2002/05/14 14:59:41 stefan Exp $ */
/* Simple hardware benchmarking suite (C and PostScript) */
#include "stdio_.h"
#include <stdlib.h>

/* Patchup for GS externals */
FILE *gs_stdout;
FILE *gs_stderr;
FILE *gs_debug_out;
const char gp_scratch_file_name_prefix[] = "gs_";
static void
capture_stdio(void)
{
    gs_stdout = stdout;
    gs_stderr = stderr;
    gs_debug_out = stderr;
}
#include "gsio.h"
#undef gs_stdout
#undef gs_stderr
#undef stdout
#define stdout gs_stdout
#undef stderr
#define stderr gs_stderr
FILE *
gp_open_scratch_file(const char *prefix, char *fname, const char *mode)
{
    return NULL;
}
void 
gp_set_printer_binary(int prnfno, int binary)
{
}
void 
gs_to_exit(int n)
{
}
#define eprintf_program_ident(f, pn, rn) (void)0
void 
lprintf_file_and_line(FILE * f, const char *file, int line)
{
    fprintf(f, "%s(%d): ", file, line);
}

/*
 * Read the CPU time (in seconds since an implementation-defined epoch)
 * into ptm[0], and fraction (in nanoseconds) into ptm[1].
 */
#include "gp_unix.c"
#undef stdout
#define stdout gs_stdout
#undef stderr
#define stderr gs_stderr

/* Loop unrolling macros */
#define do10(x) x;x;x;x;x; x;x;x;x;x

/* Define the actual benchmarks. */
static int
iadd(int a, int n, char **msg)
{
    int b = 0, i;

    for (i = n / 20; --i >= 0;) {
	do10((b += a, b += i));
    }
    *msg = "integer adds";
    return b;
}
static int
imul(int a, int n, char **msg)
{
    int b = 1, i;

    for (i = n / 20; --i > 0;) {
	do10((b *= a, b *= i));
    }
    *msg = "integer multiplies";
    return b;
}
static int
idiv(int a, int n, char **msg)
{
    int b = 1, i;

    for (i = n / 20; --i > 0;) {
	b += 999999;
	do10((b /= a, b /= i));
    }
    *msg = "integer divides";
    return b;
}
static int
fadd(float a, int n, char **msg)
{
    float b = 0;
    int i;

    for (i = n / 10; --i >= 0;) {
	do10((b += a));
    }
    *msg = "floating adds";
    return b;
}
static int
fmul(float a, int n, char **msg)
{
    float b = 1;
    int i;

    for (i = n / 10; --i >= 0;) {
	do10((b *= a));
    }
    *msg = "floating multiplies";
    return b;
}
static int
fdiv(float a, int n, char **msg)
{
    float b = 1;
    int i;

    for (i = n / 10; --i >= 0;) {
	do10((b /= a));
    }
    *msg = "floating divides";
    return b;
}
static int
fconv(int a, int n, char **msg)
{
    int b[10];
    float f[10];
    int i;

    b[0] = a;
    for (i = n / 20; --i >= 0;)
	f[0] = b[0], f[1] = b[1], f[2] = b[2], f[3] = b[3], f[4] = b[4],
	    f[5] = b[5], f[6] = b[6], f[7] = b[7], f[8] = b[8], f[9] = b[9],
	    b[0] = f[1], b[1] = f[2], b[2] = f[3], b[3] = f[4], b[4] = f[5],
	    b[5] = f[6], b[6] = f[7], b[7] = f[8], b[8] = f[9], b[9] = f[0];
    *msg = "float/int conversions";
    return b[0];
}
static int
mfast(int *m, int n, char **msg)
{
    int i;

    m[0] = n;
    for (i = n / 20; --i >= 0;)
	m[9] = m[8], m[8] = m[7], m[7] = m[6], m[6] = m[5], m[5] = m[4],
	    m[4] = m[3], m[3] = m[2], m[2] = m[1], m[1] = m[0], m[0] = m[9];
    *msg = "fast memory accesses";
    return m[0];
}
static int
mslow(int *m, int n, char **msg)
{
    int *p;
    int i, k = 0;

    m[0] = n;
    for (i = n / 20; --i >= 0; k = (k + 397) & 0x3ffff)
	p = m + k,
	    p[0] = p[100], p[20] = p[120], p[40] = p[140],
	    p[60] = p[160], p[80] = p[180],
	    p[200] = p[300], p[220] = p[320], p[240] = p[340],
	    p[260] = p[360], p[280] = p[380];
    *msg = "slow memory accesses";
    return m[0];
}

int
main(int argc, const char *argv[])
{
    int i;
    int *mem = malloc(1100000);

    capture_stdio();
    for (i = 0;; ++i) {
	long t0[2], t1[2];
	char *msg;
	int n;

	gp_get_usertime(t0);
	switch (i) {
	    case 0:
		iadd(0, n = 10000000, &msg);
		break;
	    case 1:
		imul(1, n = 1000000, &msg);
		break;
	    case 2:
		idiv(1, n = 1000000, &msg);
		break;
	    case 3:
		fadd(3.14, n = 10000000, &msg);
		break;
	    case 4:
		fmul(1.0000001, n = 10000000, &msg);
		break;
	    case 5:
		fdiv(1.0000001, n = 1000000, &msg);
		break;
	    case 6:
		fconv(12345, n = 10000000, &msg);
		break;
	    case 7:
		mfast(mem, n = 10000000, &msg);
		break;
	    case 8:
		mslow(mem, n = 1000000, &msg);
		break;
	    default:
		free(mem);
		exit(0);
	}
	gp_get_usertime(t1);
	fprintf(stdout, "Time for %9d %s = %g ms\n", n, msg,
		(t1[0] - t0[0]) * 1000.0 + (t1[1] - t0[1]) / 1000000.0);
	fflush(stdout);
    }
}

/*
   Output from SPARCstation 10, gcc -O bench.c gp_unix.c:

   Time for  10000000 integer adds = 113.502 ms
   Time for   1000000 integer multiplies = 467.965 ms
   Time for   1000000 integer divides = 594.328 ms
   Time for  10000000 floating adds = 641.21 ms
   Time for  10000000 floating multiplies = 643.357 ms
   Time for   1000000 floating divides = 131.995 ms
   Time for  10000000 float/int conversions = 602.061 ms
   Time for  10000000 fast memory accesses = 201.048 ms
   Time for   1000000 slow memory accesses = 552.606 ms

   Output from 486DX/25, wcl386 -oit bench.c gp_iwatc.c gp_msdos.c:

   Time for  10000000 integer adds = 490 ms
   Time for   1000000 integer multiplies = 770 ms
   Time for   1000000 integer divides = 1860 ms
   Time for  10000000 floating adds = 4070 ms
   Time for  10000000 floating multiplies = 4450 ms
   Time for   1000000 floating divides = 2470 ms
   Time for  10000000 float/int conversions = 25650 ms
   Time for  10000000 fast memory accesses = 990 ms
   Time for   1000000 slow memory accesses = 330 ms

 */

/*
   The rest of this file contains a similar benchmark in PostScript.

   %!
   /timer               % <str> <N> <proc> timer
   { bind 2 copy usertime mark 4 2 roll repeat cleartomark usertime exch sub
   % Stack: str N proc dt
   exch pop
   (Time for ) print exch =only ( ) print exch =only (: ) print
   =only ( ms
   ) print flush
   } def

   (x 20 integer adds) 5000 { 0
   0 add 0 add 0 add 0 add 0 add 0 add 0 add 0 add 0 add 0 add
   0 add 0 add 0 add 0 add 0 add 0 add 0 add 0 add 0 add 0 add
   pop } timer

   (x 20 integer multiplies) 5000 { 1
   3 mul 3 mul 3 mul 3 mul 3 mul 3 mul 3 mul 3 mul 3 mul 3 mul
   3 mul 3 mul 3 mul 3 mul 3 mul 3 mul 3 mul 3 mul 3 mul 3 mul
   pop } timer

   (x 20 integer divides) 5000 { 1000000000
   3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv
   3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv 3 idiv
   pop } timer

   (x 20 floating adds) 5000 { 0.0
   1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add
   1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add 1.0 add
   pop } timer

   (x 20 floating multiplies) 5000 { 1.0
   2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul
   2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul 2.3 mul
   pop } timer

   (x 20 floating divides) 5000 { 1.0
   2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div
   2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div 2.3 div
   pop } timer

   (x 20 float/int conversions) 5000 { 12345.0
   cvi cvr cvi cvr cvi cvr cvi cvr cvi cvr
   cvi cvr cvi cvr cvi cvr cvi cvr cvi cvr
   pop } timer

   /S 2048 string def
   (x 10000(byte) fast memory accesses) 1000 {
   //S 1024 1000 getinterval //S copy pop
   //S 1024 1000 getinterval //S copy pop
   //S 1024 1000 getinterval //S copy pop
   //S 1024 1000 getinterval //S copy pop
   //S 1024 1000 getinterval //S copy pop
   } timer

   /A [ 500 { 2048 string } repeat ] def
   (x 500 x 2000(byte) slower memory accesses) 10 {
   0 1 499 {
   //A exch get dup 1024 1000 getinterval exch copy pop
   } for
   } timer

   /Times-Roman findfont 36 scalefont setfont
   currentcacheparams pop pop 0 1 index setcacheparams
   /D [4 0 0 4 0 0] 1440 1440 <00 ff> makeimagedevice def
   D setdevice
   72 72 translate
   gsave 15 rotate
   0 0 moveto (A) show
   (x 10 (A) show (cache)) 100 {
   0 0 moveto
   (A) show (A) show (A) show (A) show (A) show
   (A) show (A) show (A) show (A) show (A) show
   } timer grestore

   0 setcachelimit
   gsave 10 rotate
   (x 10 (A) show (no cache)) 10 {
   0 0 moveto
   (A) show (A) show (A) show (A) show (A) show
   (A) show (A) show (A) show (A) show (A) show
   } timer grestore

   quit

   Results for SUN Sparc 2 (rated at 25 MIPS according to manual)

   ./gs
   Now in gs_init.ps
   TextAlphaBits defined GraphicsAlphaBits defined
   Aladdin Ghostscript 3.50 (1995-9-24)
   (c) 1995 Aladdin Enterprises, Menlo Park, CA.  All rights reserved. This 
   software comes with NO WARRANTY: see the file PUBLIC for details. Leaving 
   gs_init.ps
   GS>(ben1.c) run
   Time for 5000 x 20 integer adds: 171 ms
   Time for 5000 x 20 integer multiplies: 504 ms
   Time for 5000 x 20 integer divides: 334 ms
   Time for 5000 x 20 floating adds: 148 ms 
   Time for 5000 x 20 floating multiplies: 165 ms
   Time for 5000 x 20 floating divides: 194 ms 
   Time for 5000 x 20 float/int conversions: 121 ms
   Time for 1000 x 10000(byte) fast memory accesses: 112 ms
   Time for 10 x 500 x 2000(byte) slower memory accesses: 236 ms
   Loading NimbusRomanNo9L-Regular font from 
   [...]/n021003l.gsf... 1739080 414724 2564864 1251073 0 
   done. Time for 100 x 10 (A) show (cache): 144 ms
   Time for 10 x 10 (A) show (no cache): 538 ms

   Output from SPARCstation 10, gs 3.60 compiled with gcc -g -O -DDEBUG:

   gsnd bench.c
   Aladdin Ghostscript 3.60 (1995-10-23)
   Copyright (C) 1995 Aladdin Enterprises, Menlo Park, CA.  All rights reserved.
   This software comes with NO WARRANTY: see the file PUBLIC for details.
   Time for 5000 x 20 integer adds: 192 ms
   Time for 5000 x 20 integer multiplies: 561 ms
   Time for 5000 x 20 integer divides: 396 ms
   Time for 5000 x 20 floating adds: 202 ms
   Time for 5000 x 20 floating multiplies: 247 ms
   Time for 5000 x 20 floating divides: 243 ms
   Time for 5000 x 20 float/int conversions: 157 ms
   Time for 1000 x 10000(byte) fast memory accesses: 136 ms
   Time for 10 x 500 x 2000(byte) slower memory accesses: 235 ms
   Loading Temps-RomanSH font from /opt/home/peter/gs/fonts/soho/tersh___.pfb... 1759156 432729 2564864 1251025 0 done.
   Time for 100 x 10 (A) show (cache): 161 ms
   Time for 10 x 10 (A) show (no cache): 449 ms

   Output from 486DX/25, gs 2.6.1 compiled with wcc386 -oi[t]:

   gsndt bench.c
   Initializing... done.
   Ghostscript 2.6.1 (5/28/93)
   Copyright (C) 1990-1993 Aladdin Enterprises, Menlo Park, CA.
   All rights reserved.
   Ghostscript comes with NO WARRANTY: see the file COPYING for details.
   Time for 5000 x 20 integer adds: 550 ms
   Time for 5000 x 20 integer multiplies: 940 ms
   Time for 5000 x 20 integer divides: 880 ms
   Time for 5000 x 20 floating adds: 550 ms
   Time for 5000 x 20 floating multiplies: 660 ms
   Time for 5000 x 20 floating divides: 930 ms
   Time for 5000 x 20 float/int conversions: 830 ms
   Time for 1000 x 10000(byte) fast memory accesses: 660 ms
   Time for 10 x 500 x 2000(byte) slower memory accesses: 540 ms
   Loading Temps-RomanSH font from c:\gs\fonts\softhorz\tersh___.pfb... 1298792 1207949 0 done.
   Time for 100 x 10 (A) show (cache): 13520 ms
   Time for 10 x 10 (A) show (no cache): 1310 ms

   Output from 486DX/25, gs 3.52 compiled with wcc386 -oi[t]:

   Aladdin Ghostscript 3.52 (1995-10-2)
   Copyright (c) 1995 Aladdin Enterprises, Menlo Park, CA.  All rights reserved.
   This software comes with NO WARRANTY: see the file PUBLIC for details.
   Time for 5000 x 20 integer adds: 660 ms
   Time for 5000 x 20 integer multiplies: 1100 ms
   Time for 5000 x 20 integer divides: 940 ms
   Time for 5000 x 20 floating adds: 710 ms
   Time for 5000 x 20 floating multiplies: 830 ms
   Time for 5000 x 20 floating divides: 1040 ms
   Time for 5000 x 20 float/int conversions: 820 ms
   Time for 1000 x 10000(byte) fast memory accesses: 660 ms
   Time for 10 x 500 x 1000(byte) slower memory accesses: 600 ms
   Loading Temps-RomanSH font from c:\gs\fonts\softhorz\tersh___.pfb... 1678548 375231 2564864 1250964 0 done.
   Time for 100 x 10 (A) show (cache): 2520 ms
   Time for 10 x 10 (A) show (no cache): 1600 ms

 */
