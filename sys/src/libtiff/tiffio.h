/* $Header: /usr/people/sam/tiff/libtiff/RCS/tiffio.h,v 1.53 92/02/19 14:24:20 sam Exp $ */

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992 Sam Leffler
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

#ifndef _TIFFIO_
#define	_TIFFIO_

/*
 * TIFF I/O Library Definitions.
 */
#include "tiff.h"

/*
 * TIFF is defined as an incomplete type to hide the
 * library's internal data structures from clients.
 */
typedef	struct tiff TIFF;

#ifndef NULL
#define	NULL	0
#endif

/*
 * Flags to pass to TIFFPrintDirectory to control
 * printing of data structures that are potentially
 * very large.   Bit-or these flags to enable printing
 * multiple items.
 */
#define	TIFFPRINT_NONE		0x0		/* no extra info */
#define	TIFFPRINT_STRIPS	0x1		/* strips/tiles info */
#define	TIFFPRINT_CURVES	0x2		/* color/gray response curves */
#define	TIFFPRINT_COLORMAP	0x4		/* colormap */
#define	TIFFPRINT_JPEGQTABLES	0x100		/* JPEG Q matrices */
#define	TIFFPRINT_JPEGACTABLES	0x200		/* JPEG AC tables */
#define	TIFFPRINT_JPEGDCTABLES	0x200		/* JPEG DC tables */

#if defined(__STDC__) || defined(__EXTENDED__) || USE_CONST
extern const char TIFFVersion[];
extern const unsigned char TIFFBitRevTable[256];
extern const unsigned char TIFFNoBitRevTable[256];
#else
extern char TIFFVersion[];
extern unsigned char TIFFBitRevTable[256];
extern unsigned char TIFFNoBitRevTable[256];
#endif

/*
 * Macros for extracting components from the
 * packed ABGR form returned by TIFFReadRGBAImage.
 */
#define	TIFFGetR(abgr)	((abgr) & 0xff)
#define	TIFFGetG(abgr)	(((abgr) >> 8) & 0xff)
#define	TIFFGetB(abgr)	(((abgr) >> 16) & 0xff)
#define	TIFFGetA(abgr)	(((abgr) >> 24) & 0xff)

#if defined(c_plusplus) || defined(__cplusplus) || defined(__STDC__) || defined(__EXTENDED__) || USE_PROTOTYPES
#include <stdio.h>
#include <stdarg.h>

typedef	void (*TIFFErrorHandler)(char* module, char* fmt, va_list ap);

#if defined(__cplusplus)
extern "C" {
#endif
extern	void TIFFClose(TIFF*);
extern	int TIFFFlush(TIFF*);
extern	int TIFFFlushData(TIFF*);
extern	int TIFFGetField(TIFF*, int, ...);
extern	int TIFFVGetField(TIFF*, int, va_list);
extern	int TIFFGetFieldDefaulted(TIFF*, int, ...);
extern	int TIFFVGetFieldDefaulted(TIFF*, int, va_list);
extern	int TIFFReadDirectory(TIFF*);
extern	int TIFFScanlineSize(TIFF*);
extern	unsigned long TIFFStripSize(TIFF*);
extern	unsigned long TIFFVStripSize(TIFF*, unsigned long);
extern	unsigned long TIFFTileRowSize(TIFF*);
extern	unsigned long TIFFTileSize(TIFF*);
extern	unsigned long TIFFVTileSize(TIFF*, unsigned long);
extern	int TIFFFileno(TIFF*);
extern	int TIFFGetMode(TIFF*);
extern	int TIFFIsTiled(TIFF*);
extern	long TIFFCurrentRow(TIFF*);
extern	int TIFFCurrentDirectory(TIFF*);
extern	int TIFFCurrentStrip(TIFF*);
extern	int TIFFCurrentTile(TIFF*);
extern	int TIFFReadBufferSetup(TIFF*, char*, unsigned);
extern	int TIFFSetDirectory(TIFF*, int);
extern	int TIFFSetField(TIFF*, int, ...);
extern	int TIFFVSetField(TIFF*, int, va_list);
extern	int TIFFWriteDirectory(TIFF *);
#if defined(c_plusplus) || defined(__cplusplus)
extern	TIFF* TIFFOpen(const char*, const char*);
extern	TIFF* TIFFFdOpen(const int, const char*, const char*);
extern	const char* TIFFFileName(TIFF*);
extern	void TIFFError(const char*, const char*, ...);
extern	void TIFFWarning(const char*, const char*, ...);
extern	void TIFFPrintDirectory(TIFF*, FILE*, long = 0);
extern	int TIFFReadScanline(TIFF*, unsigned char*, unsigned, unsigned = 0);
extern	int TIFFWriteScanline(TIFF*, unsigned char*, unsigned, unsigned = 0);
extern	int TIFFReadRGBAImage(TIFF*, unsigned long, unsigned long, unsigned long*, int stop = 0);
#else
extern	TIFF* TIFFOpen(char*, char*);
extern	TIFF* TIFFFdOpen(int, char*, char*);
extern	char* TIFFFileName(TIFF*);
extern	void TIFFError(char*, char*, ...);
extern	TIFFErrorHandler TIFFSetErrorHandler(TIFFErrorHandler handler);
extern	void TIFFWarning(char*, char*, ...);
extern	TIFFErrorHandler TIFFSetWarningHandler(TIFFErrorHandler handler);
extern	void TIFFPrintDirectory(TIFF*, FILE*, long);
extern	int TIFFReadScanline(TIFF*, unsigned char*, unsigned, unsigned);
extern	int TIFFWriteScanline(TIFF*, unsigned char*, unsigned, unsigned);
extern	int TIFFReadRGBAImage(TIFF*, unsigned long, unsigned long, unsigned long*, int stop);
#endif
extern	unsigned int TIFFComputeTile(TIFF*, unsigned long, unsigned long, unsigned int, unsigned long);
extern	int TIFFCheckTile(TIFF*, unsigned long, unsigned long, unsigned long, unsigned);
extern	unsigned int TIFFNumberOfTiles(TIFF*);
extern	int TIFFReadTile(TIFF*, unsigned char*, unsigned long, unsigned long, unsigned long, unsigned);
extern	unsigned int TIFFComputeStrip(TIFF*, unsigned long, unsigned int);
extern	unsigned int TIFFNumberOfStrips(TIFF*);
extern	int TIFFReadEncodedStrip(TIFF*, unsigned, unsigned char*, unsigned);
extern	int TIFFReadRawStrip(TIFF*, unsigned, unsigned char*, unsigned);
extern	int TIFFReadEncodedTile(TIFF*, unsigned, unsigned char*, unsigned);
extern	int TIFFReadRawTile(TIFF*, unsigned, unsigned char*, unsigned);
extern	int TIFFWriteEncodedStrip(TIFF*, unsigned, unsigned char*, unsigned);
extern	int TIFFWriteRawStrip(TIFF*, unsigned, unsigned char*, unsigned);
extern	int TIFFWriteEncodedTile(TIFF*, unsigned, unsigned char*, unsigned);
extern	int TIFFWriteRawTile(TIFF*, unsigned, unsigned char*, unsigned);
extern	int TIFFSwabShort(unsigned short *);
extern	int TIFFSwabLong(unsigned long *);
extern	int TIFFSwabArrayOfShort(unsigned short *, int);
extern	int TIFFSwabArrayOfLong(unsigned long *, int);
extern	int TIFFReverseBits(unsigned char *, int);
#if defined(__cplusplus)
}
#endif
#else
typedef	void (*TIFFErrorHandler)();

extern	void TIFFClose();
extern	TIFF *TIFFOpen();
extern	TIFF *TIFFFdOpen();
extern	char* TIFFFileName();
extern	int TIFFFileno();
extern	int TIFFGetMode();
extern	int TIFFIsTiled();
extern	unsigned int TIFFComputeTile();
extern	long TIFFCurrentRow();
extern	int TIFFCurrentDirectory();
extern	int TIFFCurrentStrip();
extern	int TIFFCurrentTile();
extern	void TIFFError();
extern	TIFFErrorHandler TIFFSetErrorHandler();
extern	int TIFFFlush();
extern	int TIFFFlushData();
extern	int TIFFGetField();
extern	int TIFFVGetField();
extern	int TIFFGetFieldDefaulted();
extern	int TIFFVGetFieldDefaulted();
extern	unsigned int TIFFNumberOfTiles();
extern	void TIFFPrintDirectory();
extern	int TIFFReadDirectory();
extern	int TIFFReadBufferSetup();
extern	int TIFFReadScanline();
extern	int TIFFReadTile();
extern	unsigned int TIFFComputeStrip();
extern	unsigned int TIFFNumberOfStrips();
extern	int TIFFReadEncodedStrip();
extern	int TIFFReadRawStrip();
extern	int TIFFReadEncodedTile();
extern	int TIFFReadRGBAImage();
extern	int TIFFReadRawTile();
extern	int TIFFScanlineSize();
extern	unsigned long TIFFStripSize();
extern	unsigned long TIFFVStripSize();
extern	unsigned long TIFFTileRowSize();
extern	unsigned long TIFFTileSize();
extern	unsigned long TIFFVTileSize();
extern	int TIFFSetDirectory();
extern	int TIFFSetField();
extern	int TIFFVSetField();
extern	void TIFFWarning();
extern	TIFFErrorHandler TIFFSetWarningHandler();
extern	int TIFFWriteDirectory();
extern	int TIFFWriteScanline();
extern	int TIFFWriteEncodedStrip();
extern	int TIFFWriteRawStrip();
extern	int TIFFWriteEncodedTile();
extern	int TIFFWriteRawTile();
extern	int TIFFSwabShort();
extern	int TIFFSwabLong();
extern	int TIFFSwabArrayOfShort();
extern	int TIFFSwabArrayOfLong();
extern	int TIFFReverseBits();
extern	int TIFFCheckTile();
#endif
#endif /* _TIFFIO_ */
