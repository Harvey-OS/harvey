/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef LAME_PORTABLEIO_H
#define LAME_PORTABLEIO_H
/* Copyright (C) 1988-1991 Apple Computer, Inc.
 * All Rights Reserved.
 *
 * Warranty Information
 * Even though Apple has reviewed this software, Apple makes no warranty
 * or representation, either express or implied, with respect to this
 * software, its quality, accuracy, merchantability, or fitness for a
 * particular purpose.  As a result, this software is provided "as is,"
 * and you, its user, are assuming the entire risk as to its quality
 * and accuracy.
 *
 * This code may be used and freely distributed as long as it includes
 * this copyright notice and the warranty information.
 *
 * Machine-independent I/O routines for 8-, 16-, 24-, and 32-bit integers.
 *
 * Motorola processors (Macintosh, Sun, Sparc, MIPS, etc)
 * pack bytes from high to low (they are big-endian).
 * Use the HighLow routines to match the native format
 * of these machines.
 *
 * Intel-like machines (PCs, Sequent)
 * pack bytes from low to high (the are little-endian).
 * Use the LowHigh routines to match the native format
 * of these machines.
 *
 * These routines have been tested on the following machines:
 *	Apple Macintosh, MPW 3.1 C compiler
 *	Apple Macintosh, THINK C compiler
 *	Silicon Graphics IRIS, MIPS compiler
 *	Cray X/MP and Y/MP
 *	Digital Equipment VAX
 *
 *
 * Implemented by Malcolm Slaney and Ken Turkowski.
 *
 * Malcolm Slaney contributions during 1988-1990 include big- and little-
 * endian file I/O, conversion to and from Motorola's extended 80-bit
 * floating-point format, and conversions to and from IEEE single-
 * precision floating-point format.
 *
 * In 1991, Ken Turkowski implemented the conversions to and from
 * IEEE double-precision format, added more precision to the extended
 * conversions, and accommodated conversions involving +/- infinity,
 * NaN's, and denormalized numbers.
 *
 * $Id: portableio.h,v 1.2 2000/11/18 04:24:06 markt Exp $
 *
 * $Log: portableio.h,v $
 * Revision 1.2  2000/11/18 04:24:06  markt
 * Removed ieeefloat.*
 *
 * Revision 1.1  2000/09/28 16:36:53  takehiro
 * moved frontend staffs into frontend/
 * Need to debug vorbis/mpglib/analyzer/bitrate histgram.
 * still long way to go...
 *
 * HAVEGTK is changed ANALYSIS(library side) and HAVEGTK(frontend side)
 *
 * BRHIST is deleted from library. all the bitrate histogram works are
 * now in frontend(but not works properly, yet).
 *
 * timestatus things are also moved to frontend.
 *
 * parse.c is now out of library.
 *
 * Revision 1.2  2000/09/17 04:19:09  cisc
 * conformed all this-is-included-defines to match 'project_file_name' style
 *
 * Revision 1.1.1.1  1999/11/24 08:43:37  markt
 * initial checkin of LAME
 * Starting with LAME 3.57beta with some modifications
 *
 * Revision 2.6  91/04/30  17:06:02  malcolm
 */

#include	<stdio.h>

#ifndef	__cplusplus
# define	CLINK
#else
# define	CLINK "C"
#endif

extern CLINK int ReadByte(FILE *fp);
extern CLINK int Read16BitsLowHigh(FILE *fp);
extern CLINK int Read16BitsHighLow(FILE *fp);
extern CLINK void Write8Bits(FILE *fp, int i);
extern CLINK void Write16BitsLowHigh(FILE *fp, int i);
extern CLINK void Write16BitsHighLow(FILE *fp, int i);
extern CLINK int Read24BitsHighLow(FILE *fp);
extern CLINK int Read32Bits(FILE *fp);
extern CLINK int Read32BitsHighLow(FILE *fp);
extern CLINK void Write32Bits(FILE *fp, int i);
extern CLINK void Write32BitsLowHigh(FILE *fp, int i);
extern CLINK void Write32BitsHighLow(FILE *fp, int i);
extern CLINK void ReadBytes(FILE *fp, char *p, int n);
extern CLINK void ReadBytesSwapped(FILE *fp, char *p, int n);
extern CLINK void WriteBytes(FILE *fp, char *p, int n);
extern CLINK void WriteBytesSwapped(FILE *fp, char *p, int n);
extern CLINK double ReadIeeeFloatHighLow(FILE *fp);
extern CLINK double ReadIeeeFloatLowHigh(FILE *fp);
extern CLINK double ReadIeeeDoubleHighLow(FILE *fp);
extern CLINK double ReadIeeeDoubleLowHigh(FILE *fp);
extern CLINK double ReadIeeeExtendedHighLow(FILE *fp);
extern CLINK double ReadIeeeExtendedLowHigh(FILE *fp);
extern CLINK void WriteIeeeFloatLowHigh(FILE *fp, double num);
extern CLINK void WriteIeeeFloatHighLow(FILE *fp, double num);
extern CLINK void WriteIeeeDoubleLowHigh(FILE *fp, double num);
extern CLINK void WriteIeeeDoubleHighLow(FILE *fp, double num);
extern CLINK void WriteIeeeExtendedLowHigh(FILE *fp, double num);
extern CLINK void WriteIeeeExtendedHighLow(FILE *fp, double num);

#define	Read32BitsLowHigh(f)	Read32Bits(f)
#define WriteString(f,s)	fwrite(s,strlen(s),sizeof(char),f)
#endif
