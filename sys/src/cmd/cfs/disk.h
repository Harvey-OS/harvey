typedef struct Disk	Disk;

/*
 *  Reference to the disk
 */
struct Disk
{
	Bcache;
	ulong	nb;	/* number of blocks */
	ulong	nab;	/* number of allocation blocks */
	int	b2b;	/* allocation bits to a block */
	int	p2b;	/* Dptr's per page */
	char	name[CACHENAMELEN];
};

int	dinit(Disk*, int, int, char*);
int	dformat(Disk*, int, char*, ulong, ulong);
ulong	dalloc(Disk*, Dptr*);
ulong	dpalloc(Disk*, Dptr*);
int	dfree(Disk*, Dptr*);

extern int debug;

#define DPRINT if(debug)fprint
