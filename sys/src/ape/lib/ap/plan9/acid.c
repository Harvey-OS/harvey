/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* include struct defs to get acid library 
	cpp -I/sys/include/ape -I/$objtype/include/ape -I./include  acid.c > t.c
	vc -a t.c > acidlib
*/
#define _POSIX_SOURCE 1
#define _BSD_EXTENSION 1
#define _LOCK_EXTENSION
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <lock.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <termios.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <sys/utsname.h>
/* #include "lib.h" buf.c below */
/* #include "sys9.h" buf.c below */
#include "_buf.c"
#include "dir.h"
#include "fcall.h"
