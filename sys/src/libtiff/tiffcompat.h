/* $Header: /usr/people/sam/tiff/libtiff/RCS/tiffcompat.h,v 1.21 92/03/30 18:31:03 sam Exp $ */

/*
 * Copyright (c) 1990, 1991, 1992 Sam Leffler
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

#ifndef _COMPAT_
#define	_COMPAT_
/*
 * This file contains a hodgepodge of definitions and
 * declarations that are needed to provide compatibility
 * between the native system and the base UNIX implementation
 * that the library assumes (~4BSD).  In particular, you
 * can override the standard i/o interface (read/write/lseek)
 * by redefining the ReadOK/WriteOK/SeekOK macros to your
 * liking.
 *
 * NB: This file is a mess.
 */
#if (defined(__STDC__) || defined(__EXTENDED__)) && !defined(USE_PROTOTYPES)
#define	USE_PROTOTYPES	1
#define	USE_CONST	1
#endif

#if !USE_CONST && !defined(const)
#define	const
#endif

#ifdef THINK_C
#include <unix.h>
#include <math.h>
#endif
#if USE_PROTOTYPES
#include <stdio.h>
#endif
#ifndef THINK_C
#include <sys/types.h>
#endif
#ifdef VMS
#include <file.h>
#include <unixio.h>
#else
#include <fcntl.h>
#endif
#if defined(THINK_C) || defined(applec)
#include <stdlib.h>
#endif

/*
 * Workarounds for BSD lseek definitions.
 */
#if defined(SYSV) || defined(VMS)
#if defined(SYSV)
#include <unistd.h>
#endif
#define	L_SET	SEEK_SET
#define	L_INCR	SEEK_CUR
#define	L_XTND	SEEK_END
#endif
#ifndef L_SET
#define L_SET	0
#define L_INCR	1
#define L_XTND	2
#endif

/*
 * SVID workarounds for BSD bit
 * string manipulation routines.
 */
#if defined(SYSV) || defined(THINK_C) || defined(applec) || defined(VMS) || defined(plan9)
#define	bzero(dst,len)		memset((char *)dst, 0, len)
#define	bcopy(src,dst,len)	memcpy((char *)dst, (char *)src, len)
#define	bcmp(src, dst, len)	memcmp((char *)dst, (char *)src, len)
#endif

/*
 * The BSD typedefs are used throughout the library.
 * If your system doesn't have them in <sys/types.h>,
 * then define BSDTYPES in your Makefile.
 */
#ifdef BSDTYPES
typedef	unsigned char u_char;
typedef	unsigned short u_short;
typedef	unsigned int u_int;
typedef	unsigned long u_long;
#endif

/*
 * Return an open file descriptor or -1.
 */
#if defined(applec) || defined(THINK_C)
#define	TIFFOpenFile(name, mode, prot)	open(name, mode)
#else
#if defined(MSDOS)
#define	TIFFOpenFile(name, mode, prot)	open(name, mode|O_BINARY, prot)
#else
#define	TIFFOpenFile(name, mode, prot)	open(name, mode, prot)
#endif
#endif

/*
 * Return the size in bytes of the file
 * associated with the supplied file descriptor.
 */
#if USE_PROTOTYPES
extern	long TIFFGetFileSize(int fd);
#else
extern	long TIFFGetFileSize();
#endif

#ifdef MMAP_SUPPORT
/*
 * Mapped file support.
 *
 * TIFFMapFileContents must map the entire file into
 *     memory and return the address of the mapped
 *     region and the size of the mapped region.
 * TIFFUnmapFileContents does the inverse operation.
 */
#if USE_PROTOTYPES
extern	int TIFFMapFileContents(int fd, char **paddr, long *psize);
extern	void TIFFUnmapFileContents(char *addr, long size);
#else
extern	int TIFFMapFileContents();
extern	void TIFFUnmapFileContents();
#endif
#endif

/*
 * Mac workaround to handle the file
 * extension semantics of lseek.
 */
#ifdef applec
#define	lseek	mpw_lseek
extern long mpw_lseek(int, long, int);
#else
extern	long lseek();
#endif

/*
 * Default Read/Seek/Write definitions.
 */
#ifndef ReadOK
#define	ReadOK(fd, buf, size)	(read(fd, (char *)buf, size) == size)
#endif
#ifndef SeekOK
#define	SeekOK(fd, off)	(lseek(fd, (long)off, L_SET) == (long)off)
#endif
#ifndef WriteOK
#define	WriteOK(fd, buf, size)	(write(fd, (char *)buf, size) == size)
#endif

#if defined(__MACH__) || defined(THINK_C)
extern	void *malloc(size_t size);
extern	void *realloc(void *ptr, size_t size);
#else /* !__MACH__ && !THINK_C */
#if defined(MSDOS)
#include <malloc.h>
#else /* !MSDOS */
#if defined(_IBMR2)
#include <stdlib.h>
#else /* !_IBMR2 */
extern	char *malloc();
extern	char *realloc();
#endif /* _IBMR2 */
#endif /* !MSDOS */
#endif /* !__MACH__ */

/*
 * dblparam_t is the type that a double precision
 * floating point value will have on the parameter
 * stack (when coerced by the compiler).
 */
#ifdef applec
typedef extended dblparam_t;
#else
typedef double dblparam_t;
#endif

/*
 * Varargs parameter list handling...YECH!!!!
 */
#if defined(__STDC__) && !defined(USE_VARARGS)
#define	USE_VARARGS	0
#endif

#if defined(USE_VARARGS)
#if USE_VARARGS
#include <varargs.h>
#define	VA_START(ap, parmN)	va_start(ap)
#else
#include <stdarg.h>
#define	VA_START(ap, parmN)	va_start(ap, parmN)
#endif
#endif /* defined(USE_VARARGS) */
#endif /* _COMPAT_ */
