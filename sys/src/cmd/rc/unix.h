/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#undef _BSD_EXTENSION		/* avoid multiple def'n if predefined */
#undef _PLAN9_SOURCE
#undef _POSIX_SOURCE
#undef _RESEARCH_SOURCE
#undef _SUSV2_SOURCE

#define _BSD_EXTENSION
#define _PLAN9_SOURCE
#define _POSIX_SOURCE
#define _RESEARCH_SOURCE
#define _SUSV2_SOURCE

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <inttypes.h>

#ifndef NSIG
#define NSIG 32
#endif

/* plan 9 compatibility */
#define RFPROC 1
#define RFFDG 1
#define RFNOTEG 1

#define uintptr uintptr_t

char *strdup(const char *);

#define nil ((void*)0)

/* in case uchar, etc. are built-in types */
#define uchar	_fmtuchar
#define ushort	_fmtushort
#define uint	_fmtuint
#define ulong	_fmtulong
#define vlong	_fmtvlong
#define uvlong	_fmtuvlong

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;
typedef unsigned long long	uvlong;

#define OREAD	O_RDONLY
#define OWRITE	O_WRONLY
#define ORDWR	O_RDWR
#define OCEXEC	0
