/*
 * The most fundamental constant.
 * The code will not compile with RBUFSIZE made a variable;
 * for one thing, RBUFSIZE determines FEPERBUF, which determines
 * the number of elements in a free-list-block array.
 */
#define RBUFSIZE	(4*1024)	/* raw buffer size */

#include "../port/portdat.h"

extern	Mach	mach0;

typedef struct Segdesc	Segdesc;
struct Segdesc
{
	ulong	d0;
	ulong	d1;
};

typedef struct Mbank {
	ulong	base;
	ulong	limit;
} Mbank;

#define MAXBANK		8

typedef struct Mconf {
	Lock;
	Mbank	bank[MAXBANK];
	int	nbank;
	ulong	topofmem;
} Mconf;
extern Mconf mconf;

extern char nvrfile[128];
