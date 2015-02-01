/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* 9net32.16k's configuration: 16K blocks, 32-bit sizes */

/*
 * The most fundamental constant.
 * The code will not compile with RBUFSIZE made a variable;
 * for one thing, RBUFSIZE determines FEPERBUF, which determines
 * the number of elements in a free-list-block array.
 */
#ifndef RBUFSIZE
#define RBUFSIZE	(16*1024)	/* raw buffer size */
#endif
#include "32bit.h"
/*
 * setting this to zero permits the use of discs of different sizes, but
 * can make jukeinit() quite slow while the robotics work through each disc
 * twice (once per side).
 */
enum { FIXEDSIZE = 1 };


#include "portdat.h"

enum { MAXBANK = 2 };

typedef struct Mbank {
	ulong	base;
	ulong	limit;
} Mbank;

typedef struct Mconf {
	Lock;
	Mbank	bank[MAXBANK];
	int	nbank;
	ulong	memsize;
} Mconf;
extern Mconf mconf;
