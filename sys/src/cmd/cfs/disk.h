typedef struct Disk	Disk;

/*
 *  Reference to the disk
 */
struct Disk
{
	Bcache;
	int	nb;	/* number of blocks */
	int	nab;	/* number of allocation blocks */
	int	b2b;	/* allocation bits to a block */
	int	p2b;	/* Dptr's per page */
	char	name[KNAMELEN];
};

int	dinit(Disk*, int, int);
int	dformat(Disk*, int, char*, ulong, ulong);
ulong	dalloc(Disk*, Dptr*);
ulong	dpalloc(Disk*, Dptr*);
int	dfree(Disk*, Dptr*);

extern int debug;

#define DPRINT if(debug)fprint
