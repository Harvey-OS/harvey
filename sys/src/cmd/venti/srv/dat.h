/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Config		Config;
typedef struct AMap		AMap;
typedef struct AMapN		AMapN;
typedef struct Arena		Arena;
typedef struct AState	AState;
typedef struct ArenaCIG	ArenaCIG;
typedef struct ArenaHead	ArenaHead;
typedef struct ArenaPart	ArenaPart;
typedef struct ArenaTail	ArenaTail;
typedef struct ATailStats	ATailStats;
typedef struct CIBlock		CIBlock;
typedef struct Clump		Clump;
typedef struct ClumpInfo	ClumpInfo;
typedef struct Graph Graph;
typedef struct IAddr		IAddr;
typedef struct IBucket		IBucket;
typedef struct IEStream		IEStream;
typedef struct IEntry		IEntry;
typedef struct IFile		IFile;
typedef struct ISect		ISect;
typedef struct Index		Index;
typedef struct Lump		Lump;
typedef struct DBlock		DBlock;
typedef struct Part		Part;
typedef struct Statbin Statbin;
typedef struct Statdesc	Statdesc;
typedef struct Stats		Stats;
typedef struct ZBlock		ZBlock;
typedef struct Round	Round;
typedef struct Bloom	Bloom;


#define	TWID32	((uint32_t)~(uint32_t)0)
#define	TWID64	((uint64_t)~(uint64_t)0)
#define	TWID8	((uint8_t)~(uint8_t)0)

enum
{
	ABlockLog		= 9,		/* log2(512), the quantum for reading arenas */
	ANameSize		= 64,
	MaxDiskBlock		= 64*1024,	/* max. allowed size for a disk block */
	MaxIoSize		= 64*1024,	/* max. allowed size for a disk io operation */
	PartBlank		= 256*1024,	/* untouched section at beginning of partition */
	HeadSize		= 512,		/* size of a header after PartBlank */
	MinArenaSize		= 1*1024*1024,	/* smallest reasonable arena size */
	IndexBase		= 1024*1024,	/* initial address to use in an index */
	MaxIo			= 64*1024,	/* max size of a single read or write operation */
	ICacheBits		= 16,		/* default bits for indexing icache */
	MaxAMap			= 31*1024,	/* max. allowed arenas in an address mapping; must be < 32*1024 */
	Unspecified		= TWID32,

	/*
	 * return codes from syncarena
	 */
	SyncDataErr	= 1 << 0,		/* problem reading the clump data */
	SyncCIErr	= 1 << 1,		/* found erroneous clump directory entries */
	SyncCIZero	= 1 << 2,		/* found unwritten clump directory entries */
	SyncFixErr	= 1 << 3,		/* error writing fixed data */
	SyncHeader	= 1 << 4,		/* altered header fields */

	/*
	 * error severity
	 */
	EOk			= 0,		/* error expected in normal operation */
	EStrange,				/* strange error that should be logged */
	ECorrupt,				/* corrupted data found in arenas */
	EICorrupt,				/* corrupted data found in index */
	EAdmin,					/* should be brought to administrators' attention */
	ECrash,					/* really bad internal error */
	EBug,					/* a limitation which should be fixed */
	EInconsist,				/* inconsistencies between index and arena */
	EMax,

	/*
	 * internal disk formats for the venti archival storage system
	 */
	/*
	 * magic numbers on disk
	 */
	_ClumpMagic		= 0xd15cb10cU,	/* clump header, deprecated */
	ClumpFreeMagic		= 0,		/* free clump; terminates active clump log */

	ArenaPartMagic		= 0xa9e4a5e7U,	/* arena partition header */
	ArenaMagic		= 0xf2a14eadU,	/* arena trailer */
	ArenaHeadMagic		= 0xd15c4eadU,	/* arena header */

	BloomMagic		= 0xb1004eadU,	/* bloom filter header */
	BloomMaxHash	= 32,

	ISectMagic		= 0xd15c5ec7U,	/* index header */

	ArenaPartVersion	= 3,
	ArenaVersion4		= 4,
	ArenaVersion5		= 5,
	BloomVersion		= 1,
	IndexVersion		= 1,
	ISectVersion1		= 1,
	ISectVersion2		= 2,

	/*
	 * encodings of clumps on disk
	 */
	ClumpEErr		= 0,		/* can't happen */
	ClumpENone,				/* plain */
	ClumpECompress,				/* compressed */
	ClumpEMax,

	/*
	 * sizes in bytes on disk
	 */
	U8Size			= 1,
	U16Size			= 2,
	U32Size			= 4,
	U64Size			= 8,

	ArenaPartSize		= 4 * U32Size,
	ArenaSize4		= 2 * U64Size + 6 * U32Size + ANameSize + U8Size,
	ArenaSize5			= ArenaSize4 + U32Size,
	ArenaSize5a		= ArenaSize5 + 2 * U8Size + 2 * U32Size + 2 * U64Size,
	ArenaHeadSize4		= U64Size + 3 * U32Size + ANameSize,
	ArenaHeadSize5		= ArenaHeadSize4 + U32Size,
	BloomHeadSize	= 4 * U32Size,
	ISectSize1		= 7 * U32Size + 2 * ANameSize,
	ISectSize2		= ISectSize1 + U32Size,
	ClumpInfoSize		= U8Size + 2 * U16Size + VtScoreSize,
	ClumpSize		= ClumpInfoSize + U8Size + 3 * U32Size,
	MaxBloomSize		= 1<<(32-3),	/* 2^32 bits */
	MaxBloomHash	= 32,		/* bits per score */
	/*
	 * BUG - The various block copies that manipulate entry buckets
	 * would be faster if we bumped IBucketSize up to 8 and IEntrySize up to 40,
	 * so that everything is word-aligned.  Buildindex is actually cpu-bound
	 * by the (byte at a time) copying in qsort.
	 */
	IBucketSize		= U32Size + U16Size,
	IEntrySize		= U64Size + U32Size + 2*U16Size + 2*U8Size + VtScoreSize,
	IEntryTypeOff		= VtScoreSize + U32Size + U16Size + U64Size + U16Size,
	IEntryAddrOff		= VtScoreSize + U32Size + U16Size,

	MaxClumpBlocks		=  (VtMaxLumpSize + ClumpSize + (1 << ABlockLog) - 1) >> ABlockLog,

	IcacheFrac		= 1000000,	/* denominator */

	SleepForever		= 1000000000,	/* magic value for sleep time */
	/*
	 * dirty flags - order controls disk write order
	 */
	DirtyArena		= 1,
	DirtyArenaCib,
	DirtyArenaTrailer,
	DirtyMax,

	ArenaCIGSize = 10*1024,	// about 0.5 MB worth of IEntry.

	VentiZZZZZZZZ
};

extern char TraceDisk[];
extern char TraceLump[];
extern char TraceBlock[];
extern char TraceProc[];
extern char TraceWork[];
extern char TraceQuiet[];
extern char TraceRpc[];

/*
 * results of parsing and initializing a config file
 */
struct Config
{
	char		*index;			/* name of the index to initialize */
	int		naparts;		/* arena partitions initialized */
	ArenaPart	**aparts;
	int		nsects;			/* index sections initialized */
	ISect		**sects;
	Bloom	*bloom;		/* bloom filter */
	uint32_t	bcmem;
	uint32_t	mem;
	uint32_t	icmem;
	int		queuewrites;
	char*	haddr;
	char*	vaddr;
	char*	webroot;
};

/*
 * a Part is the low level interface to files or disks.
 * there are two main types of partitions
 *	arena paritions, which some number of arenas, each in a sub-partition.
 *	index partition, which only have one subpartition.
 */
struct Part
{
	int		fd;			/* rock for accessing the disk */
	int		mode;
	uint64_t		offset;
	uint64_t		size;			/* size of the partiton */
	uint32_t		blocksize;		/* block size for reads and writes */
	uint32_t		fsblocksize;	/* minimum file system block size */
	char		*name;
	char		*filename;
	Channel		*writechan;		/* chan[dcache.nblock](DBlock*) */
};

/*
 * a cached block from the partition
 * yuck -- most of this is internal structure for the cache
 * all other routines should only use data
 */
struct DBlock
{
	uint8_t	*data;

	Part	*part;			/* partition in which cached */
	uint64_t	addr;			/* base address on the partition */
	uint32_t	size;			/* amount of data available, not amount allocated; should go away */
	uint32_t	mode;
	uint32_t	dirty;
	uint32_t	dirtying;
	DBlock	*next;			/* doubly linked hash chains */
	DBlock	*prev;
	uint32_t	heap;			/* index in heap table */
	uint32_t	used;			/* last reference times */
	uint32_t	used2;
	uint32_t	ref;			/* reference count */
	RWLock	lock;			/* for access to data only */
	Channel	*writedonechan;
	void*	chanbuf[1];		/* buffer for the chan! */
};

/*
 * a cached block from the partition
 * yuck -- most of this is internal structure for the cache
 * all other routines should only use data
 * double yuck -- this is mostly the same as a DBlock
 */
struct Lump
{
	Packet	*data;

	Part	*part;			/* partition in which cached */
	uint8_t	score[VtScoreSize];	/* score of packet */
	uint8_t	type;			/* type of packet */
	uint32_t	size;			/* amount of data allocated to hold packet */
	Lump	*next;			/* doubly linked hash chains */
	Lump	*prev;
	uint32_t	heap;			/* index in heap table */
	uint32_t	used;			/* last reference times */
	uint32_t	used2;
	uint32_t	ref;			/* reference count */
	QLock	lock;			/* for access to data only */
};

/*
 * mapping between names and address ranges
 */
struct AMap
{
	uint64_t		start;
	uint64_t		stop;
	char		name[ANameSize];
};

/*
 * an AMap along with a length
 */
struct AMapN
{
	int		n;
	AMap		*map;
};

/*
 * an ArenaPart is a partition made up of Arenas
 * it exists because most os's don't support many partitions,
 * and we want to have many different Arenas
 */
struct ArenaPart
{
	Part		*part;
	uint64_t		size;			/* size of underlying partition, rounded down to blocks */
	Arena		**arenas;
	uint32_t		tabbase;		/* base address of arena table on disk */
	uint32_t		tabsize;		/* max. bytes in arena table */

	/*
	 * fields stored on disk
	 */
	uint32_t		version;
	uint32_t		blocksize;		/* "optimal" block size for reads and writes */
	uint32_t		arenabase;		/* base address of first arena */

	/*
	 * stored in the arena mapping table on disk
	 */
	AMap		*map;
	int		narenas;
};

/*
 * info about one block in the clump info cache
 */
struct CIBlock
{
	uint32_t		block;			/* blocks in the directory */
	int		offset;			/* offsets of one clump in the data */
	DBlock		*data;
};

/*
 * Statistics kept in the tail.
 */
struct ATailStats
{
	uint32_t		clumps;		/* number of clumps */
	uint32_t		cclumps;		/* number of compressed clumps */
	uint64_t		used;
	uint64_t		uncsize;
	uint8_t		sealed;
};

/*
 * Arena state - represents a point in the data log
 */
struct AState
{
	Arena		*arena;
	uint64_t		aa;			/* index address */
	ATailStats		stats;
};

/*
 * an Arena is a log of Clumps, preceeded by an ArenaHeader,
 * and followed by a Arena, each in one disk block.
 * struct on disk is not always up to date, but should be self-consistent.
 * to sync after reboot, follow clumps starting at used until ClumpFreeMagic if found.
 * <struct name="Arena" type="Arena *">
 *	<field name="name" val="s->name" type="AName"/>
 *	<field name="version" val="s->version" type="U32int"/>
 *	<field name="partition" val="s->part->name" type="AName"/>
 *	<field name="blocksize" val="s->blocksize" type="U32int"/>
 *	<field name="start" val="s->base" type="U64int"/>
 *	<field name="stop" val="s->base+2*s->blocksize" type="U64int"/>
 *	<field name="created" val="s->ctime" type="U32int"/>
 *	<field name="modified" val="s->wtime" type="U32int"/>
 *	<field name="sealed" val="s->sealed" type="Sealed"/>
 *	<field name="score" val="s->score" type="Score"/>
 *	<field name="clumps" val="s->clumps" type="U32int"/>
 *	<field name="compressedclumps" val="s->cclumps" type="U32int"/>
 *	<field name="data" val="s->uncsize" type="U64int"/>
 *	<field name="compresseddata" val="s->used - s->clumps * ClumpSize" type="U64int"/>
 *	<field name="storage" val="s->used + s->clumps * ClumpInfoSize" type="U64int"/>
 * </struct>
 */
struct Arena
{
	QLock		lock;			/* lock for arena fields, writing to disk */
	Part		*part;			/* partition in which arena lives */
	int		blocksize;		/* size of block to read or write */
	uint64_t		base;			/* base address on disk */
	uint64_t		size;			/* total space in the arena */
	uint8_t		score[VtScoreSize];	/* score of the entire sealed & summed arena */

	int		clumpmax;		/* ClumpInfos per block */
	AState		mem;
	int		inqueue;

	/*
	 * fields stored on disk
	 */
	uint32_t		version;
	char		name[ANameSize];	/* text label */
	ATailStats		memstats;
	ATailStats		diskstats;
	uint32_t		ctime;			/* first time a block was written */
	uint32_t		wtime;			/* last time a block was written */
	uint32_t		clumpmagic;

	ArenaCIG	*cig;
	int	ncig;
};

struct ArenaCIG
{
	uint64_t	offset;  // from arena base
};

/*
 * redundant storage of some fields at the beginning of each arena
 */
struct ArenaHead
{
	uint32_t		version;
	char		name[ANameSize];
	uint32_t		blocksize;
	uint64_t		size;
	uint32_t		clumpmagic;
};

/*
 * most interesting meta information for a clump.
 * stored in each clump's header and in the Arena's directory,
 * stored in reverse order just prior to the arena trailer
 */
struct ClumpInfo
{
	uint8_t		type;
	uint16_t		size;			/* size of disk data, not including header */
	uint16_t		uncsize;		/* size of uncompressed data */
	uint8_t		score[VtScoreSize];	/* score of the uncompressed data only */
};

/*
 * header for an immutable clump of data
 */
struct Clump
{
	ClumpInfo	info;
	uint8_t		encoding;
	uint32_t		creator;		/* initial client which wrote the block */
	uint32_t		time;			/* creation at gmt seconds since 1/1/1970 */
};

/*
 * index of all clumps according to their score
 * this is just a wrapper to tie together the index sections
 * <struct name="Index" type="Index *">
 *	<field name="name" val="s->name" type="AName"/>
 *	<field name="version" val="s->version" type="U32int"/>
 *	<field name="blocksize" val="s->blocksize" type="U32int"/>
 *	<field name="tabsize" val="s->tabsize" type="U32int"/>
 *	<field name="buckets" val="s->buckets" type="U32int"/>
 *	<field name="buckdiv" val="s->div" type="U32int"/>
 *	<field name="bitblocks" val="s->div" type="U32int"/>
 *	<field name="maxdepth" val="s->div" type="U32int"/>
 *	<field name="bitkeylog" val="s->div" type="U32int"/>
 *	<field name="bitkeymask" val="s->div" type="U32int"/>
 *	<array name="sect" val="&s->smap[i]" elems="s->nsects" type="Amap"/>
 *	<array name="amap" val="&s->amap[i]" elems="s->narenas" type="Amap"/>
 *	<array name="arena" val="s->arenas[i]" elems="s->narenas" type="Arena"/>
 * </struct>
 * <struct name="Amap" type="AMap *">
 *	<field name="name" val="s->name" type="AName"/>
 *	<field name="start" val="s->start" type="U64int"/>
 *	<field name="stop" val="s->stop" type="U64int"/>
 * </struct>
 */
struct Index
{
	uint32_t		div;			/* divisor for mapping score to bucket */
	uint32_t		buckets;		/* last bucket used in disk hash table */
	uint32_t		blocksize;
	uint32_t		tabsize;		/* max. bytes in index config */

	int		mapalloc;		/* first arena to check when adding a lump */
	Arena		**arenas;		/* arenas in the mapping */
	ISect		**sects;		/* sections which hold the buckets */
	Bloom		*bloom;	/* bloom filter */

	/*
	 * fields stored in config file
	 */
	uint32_t		version;
	char		name[ANameSize];	/* text label */
	int		nsects;
	AMap		*smap;			/* mapping of buckets to index sections */
	int		narenas;
	AMap		*amap;			/* mapping from index addesses to arenas */

	QLock	writing;
};

/*
 * one part of the bucket storage for an index.
 * the index blocks are sequentially allocated
 * across all of the sections.
 */
struct ISect
{
	Part		*part;
	int		blocklog;		/* log2(blocksize) */
	int		buckmax;		/* max. entries in a index bucket */
	uint32_t		tabbase;		/* base address of index config table on disk */
	uint32_t		tabsize;		/* max. bytes in index config */
	Channel	*writechan;
	Channel	*writedonechan;
	void		*ig;		/* used by buildindex only */
	int		ng;

	/*
	 * fields stored on disk
	 */
	uint32_t		version;
	uint32_t		bucketmagic;
	char		name[ANameSize];	/* text label */
	char		index[ANameSize];	/* index owning the section */
	uint32_t		blocksize;		/* size of hash buckets in index */
	uint32_t		blockbase;		/* address of start of on disk index table */
	uint32_t		blocks;			/* total blocks on disk; some may be unused */
	uint32_t		start;			/* first bucket in this section */
	uint32_t		stop;			/* limit of buckets in this section */
};

/*
 * externally interesting part of an IEntry
 */
struct IAddr
{
	uint64_t		addr;
	uint16_t		size;			/* uncompressed size */
	uint8_t		type;			/* type of block */
	uint8_t		blocks;			/* arena io quanta for Clump + data */
};

/*
 * entries in the index
 * kept in IBuckets in the disk index table,
 * cached in the memory ICache.
 */
struct IEntry
{
	/* on disk data - 32 bytes*/
	uint8_t	score[VtScoreSize];
	IAddr	ia;

	IEntry	*nexthash;
	IEntry	*nextdirty;
	IEntry	*next;
	IEntry	*prev;
	uint8_t	state;
};
enum {
	IEClean = 0,
	IEDirty = 1,
	IESummary = 2,
};

/*
 * buckets in the on disk index table
 */
struct IBucket
{
	uint16_t		n;			/* number of active indices */
	uint32_t		buck;		/* used by buildindex/checkindex only */
	uint8_t		*data;
};

/*
 * temporary buffers used by individual threads
 */
struct ZBlock
{
	uint32_t		len;
	uint32_t		_size;
	uint8_t		*data;
	uint8_t		*free;
};

/*
 * simple input buffer for a '\0' terminated text file
 */
struct IFile
{
	char		*name;				/* name of the file */
	ZBlock		*b;				/* entire contents of file */
	uint32_t		pos;				/* current position in the file */
};

struct Statdesc
{
	char *name;
	uint32_t max;
};

/* keep in sync with stats.c:/statdesc and httpd.c:/graphname*/
enum
{
	StatRpcTotal,
	StatRpcRead,
	StatRpcReadOk,
	StatRpcReadFail,
	StatRpcReadBytes,
	StatRpcReadTime,
	StatRpcReadCached,
	StatRpcReadCachedTime,
	StatRpcReadUncached,
	StatRpcReadUncachedTime,
	StatRpcWrite,
	StatRpcWriteNew,
	StatRpcWriteOld,
	StatRpcWriteFail,
	StatRpcWriteBytes,
	StatRpcWriteTime,
	StatRpcWriteNewTime,
	StatRpcWriteOldTime,

	StatLcacheHit,
	StatLcacheMiss,
	StatLcacheRead,
	StatLcacheWrite,
	StatLcacheSize,
	StatLcacheStall,
	StatLcacheReadTime,

	StatDcacheHit,
	StatDcacheMiss,
	StatDcacheLookup,
	StatDcacheRead,
	StatDcacheWrite,
	StatDcacheDirty,
	StatDcacheSize,
	StatDcacheFlush,
	StatDcacheStall,
	StatDcacheLookupTime,

	StatDblockStall,
	StatLumpStall,

	StatIcacheHit,
	StatIcacheMiss,
	StatIcacheRead,
	StatIcacheWrite,
	StatIcacheFill,
	StatIcachePrefetch,
	StatIcacheDirty,
	StatIcacheSize,
	StatIcacheFlush,
	StatIcacheStall,
	StatIcacheReadTime,
	StatIcacheLookup,
	StatScacheHit,
	StatScachePrefetch,

	StatBloomHit,
	StatBloomMiss,
	StatBloomFalseMiss,
	StatBloomLookup,
	StatBloomOnes,
	StatBloomBits,

	StatApartRead,
	StatApartReadBytes,
	StatApartWrite,
	StatApartWriteBytes,

	StatIsectRead,
	StatIsectReadBytes,
	StatIsectWrite,
	StatIsectWriteBytes,

	StatSumRead,
	StatSumReadBytes,

	StatCigLoad,
	StatCigLoadTime,

	NStat
};

extern Statdesc statdesc[NStat];

/*
 * statistics about the operation of the server
 * mainly for performance monitoring and profiling.
 */
struct Stats
{
	uint32_t		now;
	uint32_t		n[NStat];
};

struct Statbin
{
	uint nsamp;
	uint min;
	uint max;
	uint avg;
};

struct Graph
{
	int32_t (*fn)(Stats*, Stats*, void*);
	void *arg;
	int32_t t0;
	int32_t t1;
	int32_t min;
	int32_t max;
	int32_t wid;
	int32_t ht;
	int fill;
};

/*
 * for kicking background processes that run one round after another after another
 */
struct Round
{
	QLock	lock;
	Rendez	start;
	Rendez	finish;
	Rendez	delaywait;
	int		delaytime;
	int		delaykick;
	char*	name;
	int		last;
	int		current;
	int		next;
	int		doanother;
};

/*
 * Bloom filter of stored block hashes
 */
struct Bloom
{
	RWLock lk;		/* protects nhash, nbits, tab, mb */
	QLock mod;		/* one marker at a time, protects nb */
	int nhash;
	uint32_t size;		/* bytes in tab */
	uint32_t bitmask;		/* to produce bit index */
	uint8_t *data;
	Part *part;
	Channel *writechan;
	Channel *writedonechan;
};

extern	Index		*mainindex;
extern	uint32_t		maxblocksize;		/* max. block size used by any partition */
extern	int		paranoid;		/* should verify hashes on disk read */
extern	int		queuewrites;		/* put all lump writes on a queue and finish later */
extern	int		readonly;		/* only allowed to read the disk data */
extern	Stats		stats;
extern	uint8_t		zeroscore[VtScoreSize];
extern	int		compressblocks;
extern	int		writestodevnull;	/* dangerous - for performance debugging */
extern	int		collectstats;
extern	QLock	memdrawlock;
extern	int		icachesleeptime;
extern	int		minicachesleeptime;
extern	int		arenasumsleeptime;
extern	int		manualscheduling;
extern	int		l0quantum;
extern	int		l1quantum;
extern	int		ignorebloom;
extern	int		icacheprefetch;
extern	int		syncwrites;
extern	int		debugarena; /* print in arena error msgs; -1==unknown */

extern	Stats	*stathist;
extern	int	nstathist;
extern	uint32_t	stattime;

#ifndef PLAN9PORT
#define ODIRECT 0
#endif
