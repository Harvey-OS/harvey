/* $Header: /d/sam/tiff/libtiff/RCS/prototypes.h,v 1.8 92/02/18 18:20:08 sam Exp $ */

/*
 * Copyright (c) 1991, 1992 Sam Leffler
 * Copyright (c) 1991, 1992 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#if USE_PROTOTYPES
#define	DECLARE1(f,t1,a1)		f(t1 a1)
#define	DECLARE2(f,t1,a1,t2,a2)		f(t1 a1, t2 a2)
#define	DECLARE3(f,t1,a1,t2,a2,t3,a3)	f(t1 a1, t2 a2, t3 a3)
#define	DECLARE4(f,t1,a1,t2,a2,t3,a3,t4,a4)\
	f(t1 a1, t2 a2, t3 a3, t4 a4)
#define	DECLARE5(f,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5)\
	f(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)
#define	DECLARE6(f,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6)\
	f(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6)
#define	DECLARE1V(f,t1,a1)		f(t1 a1 ...)
#define	DECLARE2V(f,t1,a1,t2,a2)	f(t1 a1, t2 a2, ...)
#define	DECLARE3V(f,t1,a1,t2,a2,t3,a3)	f(t1 a1, t2 a2, t3 a3, ...)
#else
#define	DECLARE1(f,t1,a1)		f(a1) t1 a1;
#define	DECLARE2(f,t1,a1,t2,a2)		f(a1,a2) t1 a1; t2 a2;
#define	DECLARE3(f,t1,a1,t2,a2,t3,a3)	f(a1, a2, a3) t1 a1; t2 a2; t3 a3;
#define	DECLARE4(f,t1,a1,t2,a2,t3,a3,t4,a4) \
	f(a1, a2, a3, a4) t1 a1; t2 a2; t3 a3; t4 a4;
#define	DECLARE5(f,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5)\
	f(a1, a2, a3, a4, a5) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5;
#define	DECLARE6(f,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6)\
	f(a1, a2, a3, a4, a5, a6) t1 a1; t2 a2; t3 a3; t4 a4; t5 a5; t6 a6;
#if USE_VARARGS
#define	DECLARE1V(f,t1,a1) \
	f(a1, va_alist) t1 a1; va_dcl
#define	DECLARE2V(f,t1,a1,t2,a2) \
	f(a1, a2, va_alist) t1 a1; t2 a2; va_dcl
#define	DECLARE3V(f,t1,a1,t2,a2,t3,a3) \
	f(a1, a2, a3, va_alist) t1 a1; t2 a2; t3 a3; va_dcl
#else
"Help, I don't know how to handle this case: !USE_PROTOTYPES and !USE_VARARGS?"
#endif
#endif
