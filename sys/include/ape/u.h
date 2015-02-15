/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __U_H
#define __U_H
#ifndef _PLAN9_SOURCE
   This header file is an extension to ANSI/POSIX
#endif

#define nil		((void*)0)
typedef	unsigned short	ushort;
typedef	unsigned char	uchar;
typedef unsigned long	ulong;
typedef unsigned int	uint;
typedef   signed char	schar;
typedef	long long	vlong;
typedef	unsigned long long uvlong;
typedef	unsigned int		Rune;
typedef 	union FPdbleword FPdbleword;
typedef	char*	p9va_list;

#endif
