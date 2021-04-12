/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Arch Arch;
typedef struct BList BList;
typedef struct Block Block;
typedef struct Cache Cache;
typedef struct Disk Disk;
typedef struct Entry Entry;
typedef struct Fsck Fsck;
typedef struct Header Header;
typedef struct Label Label;
typedef struct Periodic Periodic;
typedef struct Snap Snap;
typedef struct Source Source;
typedef struct Super Super;
typedef struct WalkPtr WalkPtr;

//#pragma incomplete Arch
//#pragma incomplete BList
//#pragma incomplete Cache
//#pragma incomplete Disk
//#pragma incomplete Periodic
//#pragma incomplete Snap

/* tunable parameters - probably should not be constants */
enum {
	/*
	 * estimate of bytes per dir entries - determines number
	 * of index entries in the block
	 */
	BytesPerEntry = 100,
	/* don't allocate in block if more than this percentage full */
	FullPercentage = 80,
	FlushSize = 200,	/* number of blocks to flush */
	DirtyPercentage = 50,	/* maximum percentage of dirty blocks */
};

enum {
	Nowaitlock,
	Waitlock,

	NilBlock	= ((u32)(~0UL)),
	MaxBlock	= (1UL<<31),
};

enum {
	HeaderMagic = 0x3776ae89,
	HeaderVersion = 1,
	HeaderOffset = 128*1024,
	HeaderSize = 512,
	SuperMagic = 0x2340a3b1,
	SuperSize = 512,
	SuperVersion = 1,
	LabelSize = 14,
};

/* well known tags */
enum {
	BadTag = 0,		/* this tag should not be used */
	RootTag = 1,		/* root of fs */
	EnumTag,		/* root of a dir listing */
	UserTag = 32,		/* all other tags should be >= UserTag */
};

struct Super {
	u16 version;
	u32 epochLow;
	u32 epochHigh;
	u64 qid;			/* next qid */
	u32 active;			/* root of active file system */
	u32 next;			/* root of next snapshot to archive */
	u32 current;			/* root of snapshot currently archiving */
	unsigned char last[VtScoreSize];	/* last snapshot successfully archived */
	char name[128];			/* label */
};


struct Fs {
	Arch	*arch;		/* immutable */
	Cache	*cache;		/* immutable */
	int	mode;		/* immutable */
	int	noatimeupd;	/* immutable */
	int	blockSize;	/* immutable */
	VtSession *z;		/* immutable */
	Snap	*snap;		/* immutable */
	/* immutable; copy here & Fsys to ease error reporting */
	char	*name;

	Periodic *metaFlush; /* periodically flushes metadata cached in files */

	/*
	 * epoch lock.
	 * Most operations on the fs require a read lock of elk, ensuring that
	 * the current high and low epochs do not change under foot.
	 * This lock is mostly acquired via a call to fileLock or fileRlock.
	 * Deletion and creation of snapshots occurs under a write lock of elk,
	 * ensuring no file operations are occurring concurrently.
	 */
	VtLock	*elk;		/* epoch lock */
	u32	ehi;		/* epoch high */
	u32	elo;		/* epoch low */

	int	halted;	/* epoch lock is held to halt (console initiated) */

	Source	*source;	/* immutable: root of sources */
	File	*file;		/* immutable: root of files */
};

/*
 * variant on VtEntry
 * there are extra fields when stored locally
 */
struct Entry {
	u32	gen;			/* generation number */
	u16	psize;			/* pointer block size */
	u16	dsize;			/* data block size */
	unsigned char	depth;			/* unpacked from flags */
	u8	flags;
	u64	size;
	unsigned char	score[VtScoreSize];
	u32	tag;	/* tag for local blocks: zero if stored on Venti */
	u32	snap;	/* non-zero -> entering snapshot of given epoch */
	unsigned char	archive; /* archive this snapshot: only valid for snap != 0 */
};

/*
 * This is called a `stream' in the fossil paper.  There used to be Sinks too.
 * We believe that Sources and Files are one-to-one.
 */
struct Source {
	Fs	*fs;		/* immutable */
	int	mode;		/* immutable */
	int	issnapshot;	/* immutable */
	u32	gen;		/* immutable */
	int	dsize;		/* immutable */
	int	dir;		/* immutable */

	Source	*parent;	/* immutable */
	File	*file;		/* immutable; point back */

	VtLock	*lk;
	int	ref;
	/*
	 * epoch for the source
	 * for ReadWrite sources, epoch is used to lazily notice
	 * sources that must be split from the snapshots.
	 * for ReadOnly sources, the epoch represents the minimum epoch
	 * along the chain from the root, and is used to lazily notice
	 * sources that have become invalid because they belong to an old
	 * snapshot.
	 */
	u32	epoch;
	Block	*b;		/* block containing this source */
	unsigned char	score[VtScoreSize]; /* score of block containing this source */
	u32	scoreEpoch;	/* epoch of block containing this source */
	int	epb;		/* immutable: entries per block in parent */
	u32	tag;		/* immutable: tag of parent */
	u32	offset; 	/* immutable: entry offset in parent */
};


struct Header {
	u16 version;
	u16 blockSize;
	u32 super;	/* super blocks */
	u32 label;	/* start of labels */
	u32 data;	/* end of labels - start of data blocks */
	u32 end;	/* end of data blocks */
};

/*
 * contains a one block buffer
 * to avoid problems of the block changing underfoot
 * and to enable an interface that supports unget.
 */
struct DirEntryEnum {
	File	*file;

	u32	boff; 		/* block offset */

	int	i, n;
	DirEntry *buf;
};

/* Block states */
enum {
	BsFree = 0,		/* available for allocation */
	BsBad = 0xFF,		/* something is wrong with this block */

	/* bit fields */
	BsAlloc = 1<<0,	/* block is in use */
	BsCopied = 1<<1,/* block has been copied (usually in preparation for unlink) */
	BsVenti = 1<<2,	/* block has been stored on Venti */
	BsClosed = 1<<3,/* block has been unlinked on disk from active file system */
	BsMask = BsAlloc|BsCopied|BsVenti|BsClosed,
};

/*
 * block types
 * more regular than Venti block types
 * bit 3 -> block or data block
 * bits 2-0 -> level of block
 */
enum {
	BtData,
	BtDir = 1<<3,
	BtLevelMask = 7,
	BtMax = 1<<4,
};

/* io states */
enum {
	BioEmpty,	/* label & data are not valid */
	BioLabel,	/* label is good */
	BioClean,	/* data is on the disk */
	BioDirty,	/* data is not yet on the disk */
	BioReading,	/* in process of reading data */
	BioWriting,	/* in process of writing data */
	BioReadError,	/* error reading: assume disk always handles write errors */
	BioVentiError,	/* error reading from venti (probably disconnected) */
	BioMax
};

struct Label {
	u8 type;
	u8 state;
	u32 tag;
	u32 epoch;
	u32 epochClose;
};

struct Block {
	Cache	*c;
	int	ref;
	int	nlock;
	usize	pc;		/* pc that fetched this block from the cache */

	VtLock	*lk;

	int 	part;
	u32	addr;
	unsigned char	score[VtScoreSize];	/* score */
	Label	l;

	unsigned char	*dmap;

	unsigned char 	*data;

	/* the following is private; used by cache */

	Block	*next;			/* doubly linked hash chains */
	Block	**prev;
	u32	heap;			/* index in heap table */
	u32	used;			/* last reference times */

	u32	vers;			/* version of dirty flag */

	BList	*uhead;	/* blocks to unlink when this block is written */
	BList	*utail;

	/* block ordering for cache -> disk */
	BList	*prior;			/* list of blocks before this one */

	Block	*ionext;
	int	iostate;
	VtRendez *ioready;
};

/* tree walker, for gc and archiver */
struct WalkPtr
{
	unsigned char	*data;
	int	isEntry;
	int	n;
	int	m;
	Entry	e;
	unsigned char	type;
	u32	tag;
};

enum
{
	DoClose = 1<<0,
	DoClre = 1<<1,
	DoClri = 1<<2,
	DoClrp = 1<<3,
};

struct Fsck
{
	/* filled in by caller */
	int	printblocks;
	int	useventi;
	int	flags;
	int	printdirs;
	int	printfiles;
	int	walksnapshots;
	int	walkfs;
	Fs	*fs;
	int	(*print)(char*, ...);
	void	(*clre)(Fsck*, Block*, int);
	void	(*clrp)(Fsck*, Block*, int);
	void	(*close)(Fsck*, Block*, u32);
	void	(*clri)(Fsck*, char*, MetaBlock*, int, Block*);

	/* used internally */
	Cache	*cache;
	unsigned char	*amap;	/* all blocks seen so far */
	unsigned char	*emap;	/* all blocks seen in this epoch */
	unsigned char	*xmap;	/* all blocks in this epoch with parents in this epoch */
	unsigned char	*errmap;	/* blocks with errors */
	unsigned char	*smap;		/* walked sources */
	int	nblocks;
	int	bsize;
	int	walkdepth;
	u32	hint;		/* where the next root probably is */
	int	nseen;
	int	quantum;
	int	nclre;
	int	nclrp;
	int	nclose;
	int	nclri;
};

/* disk partitions; keep in sync with partname[] in disk.c */
enum {
	PartError,
	PartSuper,
	PartLabel,
	PartData,
	PartVenti,	/* fake partition */
};

extern int vtType[BtMax];
