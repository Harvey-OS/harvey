/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#undef _FORTIFY_SOURCE	/* stupid ubuntu warnings */
#define __BSD_VISIBLE 1 /* FreeBSD 5.x */
#define _BSD_SOURCE 1
#define _NETBSD_SOURCE 1	/* NetBSD */
#define _SVID_SOURCE 1
#if !defined(__APPLE__) && !defined(__OpenBSD__)
#	define _XOPEN_SOURCE 1000
#	define _XOPEN_SOURCE_EXTENDED 1
#endif
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stddef.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <stdarg.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>
#ifdef PTHREAD
#include <pthread.h>
#endif

typedef long long		p9_vlong;
typedef unsigned long long p9_uvlong;
typedef uintptr_t uintptr;
