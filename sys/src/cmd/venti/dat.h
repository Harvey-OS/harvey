typedef struct Config		Config;
typedef struct AMap		AMap;
typedef struct AMapN		AMapN;
typedef struct Arena		Arena;
typedef struct ArenaHead	ArenaHead;
typedef struct ArenaPart	ArenaPart;
typedef struct CIBlock		CIBlock;
typedef struct Clump		Clump;
typedef struct ClumpInfo	ClumpInfo;
typedef struct IAddr		IAddr;
typedef struct IBucket		IBucket;
typedef struct ICache		ICache;
typedef struct IEStream		IEStream;
typedef struct IEntry		IEntry;
typedef struct IFile		IFile;
typedef struct ISect		ISect;
typedef struct Index		Index;
typedef struct Lump		Lump;
typedef struct DBlock		DBlock;
typedef struct Part		Part;
typedef struct Stats		Stats;
typedef struct ZBlock		ZBlock;

#define TWID32	((u32int)~(u32int)0)
#define TWID64	((u64int)~(u64int)0)
#define	TWID8	((u8int)~(u8int)0)

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
	ICacheDepth		= 4,		/* default depth of an icache hash chain */
	MaxAMap			= 2*1024,	/* max. allowed arenas in an address mapping; must be < 32*1024 */

	/*
	 * return codes from syncArena
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
	ClumpMagic		= 0xd15cb10c,	/* clump header */
	ClumpFreeMagic		= 0,		/* free clump; terminates active clump log */

	ArenaPartMagic		= 0xa9e4a5e7,	/* arena partition header */
	ArenaMagic		= 0xf2a14ead,	/* arena trailer */
	ArenaHeadMagic		= 0xd15c4ead,	/* arena header */

	ISectMagic		= 0xd15c5ec7,	/* index header */

	ArenaPartVersion	= 3,
	ArenaVersion		= 4,
	IndexVersion		= 1,
	ISectVersion		= 1,

	/*
	 * encodings of clumps on disk
	 */
	ClumpEErr		= 0,		/* can't happen */
	ClumpENone,				/* plain */
	ClumpECompress,				/* compressed */
	ClumpEMax,

	/*
	 * marker for corrupted data on disk
	 */
	VtTypeCorrupt		= VtPrivateTypes,

	/*
	 * sizes in bytes on disk
	 */
	U8Size			= 1,
	U16Size			= 2,
	U32Size			= 4,
	U64Size			= 8,

	ArenaPartSize		= 4 * U32Size,
	ArenaSize		= 2 * U64Size + 6 * U32Size + ANameSize + U8Size,
	ArenaHeadSize		= U64Size + 3 * U32Size + ANameSize,
	ISectSize		= 7 * U32Size + 2 * ANameSize,
	ClumpInfoSize		= U8Size + 2 * U16Size + VtScoreSize,
	ClumpSize		= ClumpInfoSize + U8Size + 3 * U32Size,
	IBucketSize		= U32Size + U16Size,
	IEntrySize		= U64Size + U32Size + 2*U16Size + 2*U8Size + VtScoreSize,
	IEntryTypeOff		= VtScoreSize + U64Size + U32Size + 2 * U16Size,

	MaxClumpBlocks		=  (VtMaxLumpSize + ClumpSize + (1 << ABlockLog) - 1) >> ABlockLog,

	VentiZZZZZZZZ
};

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
	u64int		size;			/* size of the partiton */
	u32int		blockSize;		/* block size for reads and writes */
	char		*name;
};

/*
 * a cached block from the partition
 * yuck -- most of this is internal structure for the cache
 * all other routines should only use data
 */
struct DBlock
{
	u8int	*data;

	Part	*part;			/* partition in which cached */
	u64int	addr;			/* base address on the partition */
	u16int	size;			/* amount of data available, not amount allocated; should go away */
	DBlock	*next;			/* doubly linked hash chains */
	DBlock	*prev;
	u32int	heap;			/* index in heap table */
	u32int	used;			/* last reference times */
	u32int	used2;
	u32int	ref;			/* reference count */
	VtLock	*lock;			/* for access to data only */
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
	u8int	score[VtScoreSize];	/* score of packet */
	u8int	type;			/* type of packet */
	u16int	size;			/* amount of data allocated to hold packet */
	Lump	*next;			/* doubly linked hash chains */
	Lump	*prev;
	u32int	heap;			/* index in heap table */
	u32int	used;			/* last reference times */
	u32int	used2;
	u32int	ref;			/* reference count */
	VtLock	*lock;			/* for access to data only */
};

/*
 * mapping between names and address ranges
 */
struct AMap
{
	u64int		start;
	u64int		stop;
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
	u64int		size;			/* size of underlying partition, rounded down to blocks */
	Arena		**arenas;
	u32int		tabBase;		/* base address of arena table on disk */
	u32int		tabSize;		/* max. bytes in arena table */

	/*
	 * fields stored on disk
	 */
	u32int		version;
	u32int		blockSize;		/* "optimal" block size for reads and writes */
	u32int		arenaBase;		/* base address of first arena */

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
	u32int		block;			/* blocks in the directory */
	int		offset;			/* offsets of one clump in the data */
	DBlock		*data;
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
 *	<field name="blockSize" val="s->blockSize" type="U32int"/>
 *	<field name="start" val="s->base" type="U64int"/>
 *	<field name="stop" val="s->base+2*s->blockSize" type="U64int"/>
 *	<field name="created" val="s->ctime" type="U32int"/>
 *	<field name="modified" val="s->wtime" type="U32int"/>
 *	<field name="sealed" val="s->sealed" type="Sealed"/>
 *	<field name="score" val="s->score" type="Score"/>
 *	<field name="clumps" val="s->clumps" type="U32int"/>
 *	<field name="compressedClumps" val="s->cclumps" type="U32int"/>
 *	<field name="data" val="s->uncsize" type="U64int"/>
 *	<field name="compressedData" val="s->used - s->clumps * ClumpSize" type="U64int"/>
 *	<field name="storage" val="s->used + s->clumps * ClumpInfoSize" type="U64int"/>
 * </struct>
 */
struct Arena
{
	VtLock		*lock;			/* lock for arena fields, writing to disk */
	Part		*part;			/* partition in which arena lives */
	int		blockSize;		/* size of block to read or write */
	u64int		base;			/* base address on disk */
	u64int		size;			/* total space in the arena */
	u64int		limit;			/* storage limit for clumps */
	u8int		score[VtScoreSize];	/* score of the entire sealed & summed arena */

	int		clumpMax;		/* ClumpInfos per block */
	CIBlock		cib;			/* dirty clump directory block */

	/*
	 * fields stored on disk
	 */
	u32int		version;
	char		name[ANameSize];	/* text label */
	u32int		clumps;			/* number of allocated clumps */
	u32int		cclumps;		/* clumps which are compressed; informational only */
	u32int		ctime;			/* first time a block was written */
	u32int		wtime;			/* last time a block was written */
	u64int		used;			/* number of bytes currently used */
	u64int		uncsize;		/* total of all clumps's uncsize; informational only */
	u8int		sealed;			/* arena all filled up? */
};

/*
 * redundant storage of some fields at the beginning of each arena
 */
struct ArenaHead
{
	u32int		version;
	char		name[ANameSize];
	u32int		blockSize;
	u64int		size;
};

/*
 * most interesting meta information for a clump.
 * stored in each clump's header and in the Arena's directory,
 * stored in reverse order just prior to the arena trailer
 */
struct ClumpInfo
{
	u8int		type;
	u16int		size;			/* size of disk data, not including header */
	u16int		uncsize;		/* size of uncompressed data */
	u8int		score[VtScoreSize];	/* score of the uncompressed data only */
};

/*
 * header for an immutable clump of data
 */
struct Clump
{
	ClumpInfo	info;
	u8int		encoding;
	u32int		creator;		/* initial client which wrote the block */
	u32int		time;			/* creation at gmt seconds since 1/1/1970 */
};

/*
 * index of all clumps according to their score
 * this is just a wrapper to tie together the index sections
 * <struct name="Index" type="Index *">
 *	<field name="name" val="s->name" type="AName"/>
 *	<field name="version" val="s->version" type="U32int"/>
 *	<field name="blockSize" val="s->blockSize" type="U32int"/>
 *	<field name="tabSize" val="s->tabSize" type="U32int"/>
 *	<field name="buckets" val="s->buckets" type="U32int"/>
 *	<field name="buckDiv" val="s->div" type="U32int"/>
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
	u32int		div;			/* divisor for mapping score to bucket */
	u32int		buckets;		/* last bucket used in disk hash table */
	u32int		blockSize;
	u32int		tabSize;		/* max. bytes in index config */
	int		mapAlloc;		/* first arena to check when adding a lump */
	Arena		**arenas;		/* arenas in the mapping */
	ISect		**sects;		/* sections which hold the buckets */

	/*
	 * fields stored in config file 
	 */
	u32int		version;
	char		name[ANameSize];	/* text label */
	int		nsects;
	AMap		*smap;			/* mapping of buckets to index sections */
	int		narenas;
	AMap		*amap;			/* mapping from index addesses to arenas */
};

/*
 * one part of the bucket storage for an index.
 * the index blocks are sequentially allocated
 * across all of the sections.
 */
struct ISect
{
	Part		*part;
	int		blockLog;		/* log2(blockSize) */
	int		buckMax;		/* max. entries in a index bucket */
	u32int		tabBase;		/* base address of index config table on disk */
	u32int		tabSize;		/* max. bytes in index config */

	/*
	 * fields stored on disk
	 */
	u32int		version;
	char		name[ANameSize];	/* text label */
	char		index[ANameSize];	/* index owning the section */
	u32int		blockSize;		/* size of hash buckets in index */
	u32int		blockBase;		/* address of start of on disk index table */
	u32int		blocks;			/* total blocks on disk; some may be unused */
	u32int		start;			/* first bucket in this section */
	u32int		stop;			/* limit of buckets in this section */
};

/*
 * externally interesting part of an IEntry
 */
struct IAddr
{
	u64int		addr;
	u16int		size;			/* uncompressed size */
	u8int		type;			/* type of block */
	u8int		blocks;			/* arena io quanta for Clump + data */
};

/*
 * entries in the index
 * kept in IBuckets in the disk index table,
 * cached in the memory ICache.
 */
struct IEntry
{
	u8int		score[VtScoreSize];
	IEntry		*next;			/* next in hash chain */
	u32int		wtime;			/* last write time */
	u16int		train;			/* relative train containing the most recent ref; 0 if no ref, 1 if in same car */
	IAddr		ia;
};

/*
 * buckets in the on disk index table
 */
struct IBucket
{
	u16int		n;			/* number of active indices */
	u32int		next;			/* overflow bucket */
	u8int		*data;
};

/*
 * temporary buffers used by individual threads
 */
struct ZBlock
{
	u32int		len;
	u8int		*data;
};

/*
 * simple input buffer for a '\0' terminated text file
 */
struct IFile
{
	char		*name;				/* name of the file */
	ZBlock		*b;				/* entire contents of file */
	u32int		pos;				/* current position in the file */
};

/*
 * statistics about the operation of the server
 * mainly for performance monitoring and profiling.
 */
struct Stats
{
	VtLock		*lock;
	long		lumpWrites;		/* protocol block writes */
	long		lumpReads;		/* protocol block reads */
	long		lumpHit;		/* lump cache hit */
	long		lumpMiss;		/* lump cache miss */
	long		clumpWrites;		/* clumps to disk */
	vlong		clumpBWrites;		/* clump data bytes to disk */
	vlong		clumpBComp;		/* clump bytes compressed */
	long		clumpReads;		/* clumps from disk */
	vlong		clumpBReads;		/* clump data bytes from disk */
	vlong		clumpBUncomp;		/* clump bytes uncompressed */
	long		ciWrites;		/* clump directory to disk */
	long		ciReads;		/* clump directory from disk */
	long		indexWrites;		/* index to disk */
	long		indexReads;		/* index from disk */
	long		indexWReads;		/* for writing a new entry */
	long		indexAReads;		/* for allocating an overflow block */
	long		diskWrites;		/* total disk writes */
	long		diskReads;		/* total disk reads */
	vlong		diskBWrites;		/* total disk bytes written */
	vlong		diskBReads;		/* total disk bytes read */
	long		pcHit;			/* partition cache hit */
	long		pcMiss;			/* partition cache miss */
	long		pcReads;		/* partition cache reads from disk */
	vlong		pcBReads;		/* partition cache bytes read */
	long		icInserts;		/* stores into index cache */
	long		icLookups;		/* index cache lookups */
	long		icHits;			/* hits in the cache */
	long		icFills;		/* successful fills from index */
};

extern	Index		*mainIndex;
extern	u32int		maxBlockSize;		/* max. block size used by any partition */
extern	int		paranoid;		/* should verify hashes on disk read */
extern	int		queueWrites;		/* put all lump writes on a queue and finish later */
extern	int		readonly;		/* only allowed to read the disk data */
extern	Stats		stats;
extern	u8int		zeroScore[VtScoreSize];
