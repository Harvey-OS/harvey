#define RBUFSIZE	(16*1024)	/* raw buffer size */
/*
 * verify that the kernel prints this size when you
 * first boot this kernel.
 * #define	DSIZE		157933
 */
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
