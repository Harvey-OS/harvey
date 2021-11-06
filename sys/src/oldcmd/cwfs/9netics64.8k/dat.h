/* 9net64.8k's configuration: 8K blocks, 64-bit sizes */

/*
 * The most fundamental constant.
 * The code will not compile with RBUFSIZE made a variable;
 * for one thing, RBUFSIZE determines FEPERBUF, which determines
 * the number of elements in a free-list-block array.
 */
#ifndef RBUFSIZE
#define RBUFSIZE	(8*1024)	/* raw buffer size */
#endif
#include "64bit.h"
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
