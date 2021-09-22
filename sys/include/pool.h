typedef struct Pool Pool;
struct Pool {
	char*	name;
	uintptr	maxsize;

	uintptr	cursize;
	uintptr	curfree;
	uintptr	curalloc;

	uintptr	minarena;	/* smallest size of new arena */
	uintptr	quantum;	/* allocated blocks should be multiple of (2^n) */
	uintptr	minblock;	/* smallest newly allocated block */

	void*	freeroot;	/* actually Free* */
	void*	arenalist;	/* actually Arena* */

	void*	(*alloc)(uintptr);
	int	(*merge)(void*, void*);
	void	(*move)(void* from, void* to);

	int	flags;
	int	nfree;
	int	lastcompact;

	void	(*lock)(Pool*);
	void	(*unlock)(Pool*);
	void	(*print)(Pool*, char*, ...);
	void	(*panic)(Pool*, char*, ...);
	void	(*logstack)(Pool*);

	void*	private;
};

extern void*	poolalloc(Pool*, uintptr);
extern void*	poolallocalign(Pool*, uintptr, uintptr, vlong, uintptr);
extern void	poolfree(Pool*, void*);
extern uintptr	poolmsize(Pool*, void*);
extern void*	poolrealloc(Pool*, void*, uintptr);
extern void	poolcheck(Pool*);
extern int	poolcompact(Pool*);
extern void	poolblockcheck(Pool*, void*);

extern Pool*	mainmem;
extern Pool*	imagmem;

enum {	/* flags */
	POOL_ANTAGONISM	= 1<<0,
	POOL_PARANOIA	= 1<<1,
	POOL_VERBOSITY	= 1<<2,
	POOL_DEBUGGING	= 1<<3,
	POOL_LOGGING	= 1<<4,
	POOL_TOLERANCE	= 1<<5,
	POOL_NOREUSE	= 1<<6,
};
