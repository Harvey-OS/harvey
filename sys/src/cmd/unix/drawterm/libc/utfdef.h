/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define uchar _utfuchar
#define ushort _utfushort
#define uint _utfuint
#define ulong _utfulong
#define vlong _utfvlong
#define uvlong _utfuvlong

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

#define nelem(x) (sizeof(x)/sizeof((x)[0]))
#define nil ((void*)0)
