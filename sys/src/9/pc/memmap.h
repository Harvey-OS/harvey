enum {
	MEMDEBUG	= 0,

	MemUPA		= 0,		/* unbacked physical address */
	MemRAM		= 1,		/* physical memory */
	MemUMB		= 2,		/* upper memory block (<16MB) */
	MemReserved	= 3,
	NMemType	= 4,
};

typedef struct Map Map;
struct Map {
	ulong	size;
	ulong	addr;
};

typedef struct RMap RMap;
struct RMap {
	char*	name;
	Map*	map;
	Map*	mapend;

	Lock;
};

uintptr	finde820map(void);
uintptr	freebasemem(void);
void	map(ulong base, ulong len, int type);
ulong	mapalloc(RMap* rmap, ulong addr, int size, int align);
void	mapfree(RMap* rmap, ulong addr, ulong size);
void	mapprint(RMap *rmap);
void	memdebug(void);
int	rdmbootmmap(void);
void	umbscan(void);

/*
 * Memory allocation tracking.
 */
Map mapupa[16];
RMap rmapupa;

Map mapram[32];
RMap rmapram;

Map mapumb[64];
RMap rmapumb;

Map mapumbrw[16];
RMap rmapumbrw;
