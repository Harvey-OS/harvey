typedef struct Bbuf	Bbuf;
typedef struct Bcache	Bcache;

enum
{
	Nbcache=	32,		/* number of blocks kept in pool */
};

/*
 *  block cache descriptor
 */
struct Bbuf
{
	Lru;				/* must be first in struct */
	ulong	bno;
	int	inuse;
	Bbuf	*next;			/* next in dirty list */
	int	dirty;
	char	*data;
};

/*
 *  the buffer cache
 */
struct Bcache
{
	Lru;
	int	bsize;			/* block size in bytes */
	int	f;			/* fd to disk */
	Bbuf	*dfirst;		/* dirty list */
	Bbuf	*dlast;
	Bbuf	bb[Nbcache];
};

int	bcinit(Bcache*, int, int);
Bbuf*	bcalloc(Bcache*, ulong);
Bbuf*	bcread(Bcache*, ulong);
void	bcmark(Bcache*, Bbuf*);
int	bcwrite(Bcache*, Bbuf*);
int	bcsync(Bcache*);
int	bread(Bcache*, ulong, void*);
int	bwrite(Bcache*, ulong, void*);
int	bref(Bcache*, Bbuf*);
void	error(char*, ...);
void	warning(char*);
