#define RBUFSIZE	(16*1024)	/* raw buffer size */
/* this kernel still uses DSIZE because it uses sony.c instead of juke.c */
#define	DSIZE		157933

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
