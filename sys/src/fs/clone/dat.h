#define RBUFSIZE	(16*1024)	/* raw buffer size */
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

/*
 *  a parsed plan9.ini line
 */
#define ISAOPTLEN	16
#define NISAOPT		8

typedef struct ISAConf {
	char	type[NAMELEN];
	ulong	port;
	ulong	irq;
	ulong	dma;
	ulong	mem;
	ulong	size;
	ulong	freq;

	int	nopt;
	char	opt[NISAOPT][ISAOPTLEN];
} ISAConf;

typedef struct Pcidev Pcidev;

extern char nvrfile[128];
