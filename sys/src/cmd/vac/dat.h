typedef struct Source Source;
typedef struct VacFile VacFile;
typedef struct VacDirSource VacDirSource;
typedef struct MetaBlock MetaBlock;
typedef struct Lump Lump;
typedef struct Cache Cache;
typedef struct Super Super;

struct VacFS {
	int ref;
	
	/* need a read write lock? */

	uchar score[VtScoreSize];
	VacFile *root;
	
	VtSession *z;
	int readOnly;
	int bsize;		/* maximum block size */
	uvlong qid;		/* next qid */
	Cache *cache;
};

struct VacFile {
	int ref;
	VtLock *lk;
	VacFS *fs;		/* immutable */

	int	used;
	int	removed;

	Source *source;
	Source *msource;	/* meta source: only for directories */
	
	VacDir dir;		/* meta data for this file */

	VacFile *up;	/* parent file */
	VacFile *down;	/* children */
	VacFile *next;	/* sibling */
};

struct Source {
	VtLock *lk;

	Cache *cache;	/* immutable */
	int readOnly;	/* immutable */
	Lump *lump;	/* lump containing dir entry */
	int entry;	/* which entry in the block: immutable */

	/* unpacked VtDirEntry */
	ulong gen;	/* generation: immutable */
	int dir;	/* dir flags: immutable */
	int depth;	/* number of levels of pointer blocks */
	int psize;	/* pointer block size: immutable */
	int dsize;	/* data block size: immutable */
	uvlong size2;	/* size in bytes of file */

	int epb;	/* dir entries per block = dize/VtDirEntrySize: immutable */
};

struct MetaBlock {
	int size;
	int free;
	int maxindex;
	int nindex;
	uchar *buf;
};

/*
 * contains a one block buffer
 * to avoid problems of the block changing underfoot
 * and to enable an interface that supports unget.
 */
struct VacDirEnum {
	VacFile *file;
	
	ulong block;	/* current block */
	MetaBlock mb;	/* parsed version of block */
	int index;	/* index in block */
	uchar *buf;	/* raw data of block */
};

/* Lump states */
enum {
	LumpFree,
	LumpVenti,	/* on venti server: score > 2^32: just a cached copy */
	LumpActive,	/* active */
	LumpActiveRO,	/* active: read only block */
	LumpActiveA,	/* active: achrived */
	LumpSnap,	/* snapshot: */
	LumpSnapRO,	/* snapshot: read only */
	LumpSnapA,	/* snapshot: achived */
	LumpZombie,	/* block with no pointer to it: waiting to be freed */
	
	LumpMax
};

/*
 * Each lump has a state and generation
 * The following invariants are maintained
 * 	Each lump has no more than than one parent per generation
 * 	For Active*, no child has a parent of a greater generation
 *	For Snap*, there is a parent of given generation and there are
 *		no parents of greater gen - implies no children of a less gen
 *	For *RO, the lump is fixed - no change ca be made - all pointers
 *		are valid venti addresses
 *	For *A, the lump is on the venti server
 *	There are no pointers to Zombie lumps
 *
 * Transitions
 *	Archiver at generation g
 *	Mutator at generation h
 *	
 *	Want to modify a lump
 *		Venti: create new Active(h)
 *		Active(x): x == h: do nothing
 *		Acitve(x): x < h: change to Snap(h) + add Active(h)
 *		ActiveRO(x): change to SnapRO(h) + add Active(h)
 *		ActiveA(x): add Active(h)
 *		Snap*(x): should not occur
 *		Zombie(x): should not occur
 *	Want to archive
 *		Active(x): x != g: should never happen
 *		Active(x): x == g fix children and free them: move to ActoveRO(g);
 *		ActiveRO(x): x != g: should never happen
 *		ActiveRO(x): x == g: wait until it hits ActiveA or SnapA
 *		ActiveA(x): done
 *		Active(x): x < g: should never happen
 *		Snap(x): x >= g: fix children, freeing all SnapA(y) x == y;
 *		SnapRO(x): wait until it hits SnapA
 *
 */


struct Lump {
	int ref;

	Cache *c;

	VtLock *lk;
	
	int state;
	ulong gen;

	uchar 	*data;
	uchar	score[VtScoreSize];	/* score of packet */
	uchar	vscore[VtScoreSize];	/* venti score - when archived */
	u8int	type;			/* type of packet */
	int	dir;			/* part of a directory - extension of type */
	u16int	size;			/* amount of data allocated */
	Lump	*next;			/* doubly linked hash chains */
	Lump	*prev;
	u32int	heap;			/* index in heap table */
	u32int	used;			/* last reference times */
	u32int	used2;

	u32int	addr;			/* mutable block address */
};

