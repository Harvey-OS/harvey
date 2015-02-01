/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <io.h>
#include <setjmp.h>
#include <direct.h>
#include <process.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>

/* disable various silly warnings */
#ifdef MSVC
#pragma warning( disable : 4245 4305 4244 4102 4761 4090 4028 4024)
#endif

typedef __int64		p9_vlong;
typedef	unsigned __int64 p9_uvlong;
typedef unsigned uintptr;
