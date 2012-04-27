typedef struct Ddatablk Ddatablk;
typedef struct Dptrblk Dptrblk;
typedef struct Drefblk Drefblk;
typedef struct Dattrblk Dattrblk;
typedef struct Dfileblk Dfileblk;
typedef struct Dsuperblk Dsuperblk;
typedef struct Dsuperdata Dsuperdata;
typedef union Diskblk Diskblk;
typedef struct Diskblkhdr Diskblkhdr;
typedef struct Memblk Memblk;
typedef struct Fsys Fsys;
typedef struct Dmeta Dmeta;
typedef struct Blksl Blksl;
typedef struct Mfile Mfile;
typedef struct Cmd Cmd;
typedef struct Path Path;
typedef struct Alloc Alloc;
typedef struct Next Next;
typedef struct Lstat Lstat;
typedef struct List List;
typedef struct Link Link;
typedef struct Usr Usr;
typedef struct Member Member;


/*
 * Conventions:
 *
 * References:
 *	- Ref is used for in-memory RCs. This has nothing to do with on-disk refs.
 * 	- Mem refs include the reference from the hash. That one keeps the file
 *	  loaded in memory while unused.
 *	- The hash ref also accounts for refs from the lru/ref/dirty lists.
 *	- Disk refs count only references within the tree on disk.
 *	- There are two copies of disk references, even, and odd.
 *	  Only one of them is active. Every time the system is written,
 *	  the inactive copy becomes active and vice-versa. Upon errors,
 *	  the active copy on disk is always coherent because the super is
 *	  written last.
 *	- Children do not add refs to parents; parents do not add ref to children.
 *	- 9p, fscmd, ix, and other top-level shells for the fs are expected to
 *	  keep Paths for files in use, so that each file in the path
 *	  is referenced once by the path
 *	- example, on debug fsdump()s:
 *		r=2 -> 1 (from hash) + 1 (while dumping the file info).
 *		(block is cached, in the hash, but unused otherwise).
 *		r=3 in /active: 1 (hash) + 1(fs->active) + 1(dump)
 *		r is greater:
 *			- some fid is referencing the block
 *			- it's a melt and the frozen f->mf->melted is a ref.
 *			- some rpc is using it (reading/writing/...)
 *
 * Assumptions:
 *	- /active is *never* found on disk, it's memory-only.
 *	- b->addr is worm.
 *	- parents of files loaded in memory are also in memory.
 *	  (but this does not hold for pointer and data blocks).
 *	- We try not to hold more than one lock, using the
 *	  reference counters when we need to be sure that
 *	  an unlocked resource does not vanish.
 *	- reference blocks are never removed from memory.
 *	- disk refs are frozen while waiting to go to disk during a fs freeze.
 *	  in which case db*ref functions write the block in place and melt it.
 *	- frozen blocks are quiescent.
 *	- mb*() functions do not raise errors.
 *
 * Locking:
 *	- the caller to functions in [mbf]blk.c acquires the locks before
 *	  calling them, and makes sure the file is melted if needed.
 *	  This prevents races and deadlocks.
 *	- blocks are locked by the file responsible for them, when not frozen.
 *	- next fields in blocks are locked by the list they are used for.
 *
 * Lock order:
 *	- fs, super,... : while locked can't acquire fs or blocks.
 *	- parent -> child
 *	  (but a DBfile protects all ptr and data blocks under it).
 *	- block -> ref block
 *
 * All the code assumes outofmemoryexits = 1.
 */

/*
 * these are used by several functions that have flags to indicate
 * mem-only, also on disk; and read-access/write-access. (eg. dfmap).
 */
enum{
	Mem=0,
	Disk,

	Rd=0,
	Wr,

	Dontmk = 0,
	Mkit,

	Tqlock = 0,
	Trwlock,
	Tlock,

	No = 0,
	Yes,

	/* mtime is ns in creepy, but s in 9p */
	NSPERSEC = 1000000000ULL,
};


struct Lstat
{
	int	type;
	uintptr	pc;
	int	ntimes;
	int	ncant;
	vlong	wtime;
};


enum
{
	DMUSERS = 0x01000000ULL,
	DMBITS = DMDIR|DMAPPEND|DMEXCL|DMTMP|0777,
};

#define HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

/*
 * ##### On disk structures. #####
 *
 * All on-disk integer values are little endian.
 *
 * blk 0: unused
 * blk 1: super
 * even ref blk + odd ref blk + Nblkgrpsz-2 blocks
 * ...
 * even ref blk + odd ref blk + Nblkgrpsz-2 blocks
 *
 * The code assumes these structures are packed.
 * Be careful if they are changed to make things easy for the
 * compiler and keep them naturally aligned.
 */

enum
{
	/* block types */
	DBfree = 0,
	DBref,
	DBattr,
	DBfile,
	DBsuper,
	DBdata,			/* direct block */
	DBptr0 = DBdata+1,	/* simple-indirect block */
				/* double */
				/* triple */
				/*...*/
	DBctl = ~0,		/* DBfile, never on disk. arg for dballoc */

	Daddrsz = BIT64SZ,
	Dblkhdrsz = BIT64SZ,
	Nblkgrpsz = (Dblksz - Dblkhdrsz) / Daddrsz,
	Dblk0addr = 2*Dblksz,

};

typedef u64int daddrt;		/* disk addreses and sizes */

struct Ddatablk
{
	uchar	data[1];	/* raw memory */
};

struct Dptrblk
{
	daddrt	ptr[1];		/* array of block addresses */
};

struct Drefblk
{
	daddrt	ref[1];		/* disk RC or next block in free list */
};

struct Dattrblk
{
	daddrt	next;		/* next block used for attribute data */
	uchar	attr[1];	/* raw attribute data */
};

struct Dmeta			/* mandatory metadata */
{
	u64int	id;		/* ctime, actually */
	u64int	mode;
	u64int	atime;
	u64int	mtime;
	u64int	length;
	u64int	uid;
	u64int	gid;
	u64int	muid;
	/* name\0 */
};

/*
 * The trailing part of the file block is used to store attributes
 * and initial file data.
 * At least Dminattrsz is reserved for attributes, at most
 * all the remaining embedded space.
 * Past the attributes, starts the file data.
 * If more attribute space is needed, an attribute block is allocated.
 * For huge attributes, it is suggested that a file is allocated and
 * the attribute value refers to that file.
 * The pointer in iptr[n] is an n-indirect data pointer.
 *
 * Directories are also files, but their data is simply an array of
 * disk addresses for files.
 *
 * To ensure embed is a multiple of dir entries, we declare it here as [8]
 * and not as [1].
 */
struct Dfileblk
{
	u64int	asize;		/* attribute size */
	u64int	ndents;		/* # of directory entries, for dirs */
	daddrt	aptr;		/* attribute block pointer */
	daddrt	dptr[Ndptr];	/* direct data pointers */
	daddrt	iptr[Niptr];	/* indirect data pointers */
	Dmeta;			/* predefined attributes, followed by name */
	uchar	embed[Daddrsz];	/* embedded attrs and data */
};

#define	MAGIC	0x6699BCB06699BCB0ULL
/*
 * Superblock.
 * The stored tree is:
 *		archive/		root of the archived tree
 *			<epoch>
 *			...
 * (/ and /active are only memory and never on disk, parts
 * under /active that are on disk are shared with entries in /archive)
 *
 * It contains two copies of the information, Both should be identical.
 * If there are errors while writing this block, the one with the
 * oldest epoch should be ok.
 */
struct Dsuperdata
{
	u64int	magic;		/* MAGIC */
	u64int	epoch;
	daddrt	free;		/* first free block on list  */
	daddrt	eaddr;		/* end of the assigned disk portion */
	daddrt	root;		/* address of /archive in disk */
	u64int	oddrefs;	/* use odd ref blocks? or even ref blocks? */
	u64int	ndfree;		/* # of blocks in free list */
	u64int	maxuid;		/* 1st available uid */
	u64int	dblksz;		/* only for checking */
	u64int	nblkgrpsz;	/* only for checking */
	u64int	dminattrsz;	/* only for checking */
	u64int	ndptr;		/* only for checking */
	u64int	niptr;		/* only for checking */
	u64int	dblkdatasz;	/* only for checking */
	u64int	embedsz;	/* only for checking */
	u64int	dptrperblk;	/* only for checking */
};

struct Dsuperblk
{
	union{
		Dsuperdata;
		uchar align[Dblksz/2];
	};
	Dsuperdata dup;
};

enum
{
	/* addresses for ctl files and / have this bit set, and are never
	 * found on disk.
	 */
	Fakeaddr = 0x8000000000000000ULL,
	Noaddr = ~0ULL,
};

#define	TAG(type,addr)		((addr)<<8|((type)&0x7F))
#define	TAGTYPE(t)		((t)&0x7F)
#define	TAGADDROK(t,addr)	(((t)&~0xFF) == ((addr)<<8))

/*
 * disk blocks
 */

/*
 * header for all disk blocks.
 */
struct Diskblkhdr
{
	u64int	tag;		/* block tag */
};

union Diskblk
{
	struct{
		Diskblkhdr;
		union{
			Ddatablk;	/* data block */
			Dptrblk;	/* pointer block */
			Drefblk;	/* reference counters block */
			Dattrblk;	/* attribute block */
			Dfileblk;	/* file block */
			Dsuperblk;
		};
	};
	uchar	ddata[Dblksz];
};

/*
 * These are derived.
 * Embedsz must compensate that embed[] was declared as embed[Daddrsz],
 * to make it easy for the compiler to keep things aligned on 64 bits.
 */
enum
{
	Dblkdatasz = sizeof(Diskblk) - sizeof(Diskblkhdr),
	Embedsz	= Dblkdatasz - sizeof(Dfileblk) + Daddrsz,
	Dptrperblk = Dblkdatasz / Daddrsz,
	Drefperblk = Dblkdatasz / Daddrsz,
};


/*
 * File attributes are name/value pairs.
 * By now, only mandatory attributes are implemented, and
 * have names implied by their position in the Dmeta structure.
 */

/*
 * ##### On memory structures. #####
 */

/*
 * On memory file information.
 */
struct Mfile
{
	Mfile*	next;		/* in free list */
	RWLock;

	char	*uid;		/* reference to the user table */
	char	*gid;		/* reference to the user table */
	char	*muid;		/* reference to the user table */
	char	*name;		/* reference to the disk block */

	Memblk*	melted;		/* next version for this one, if frozen */
	ulong	lastbno;	/* last accessed block nb within this file */
	ulong	sequential;	/* access has been sequential */

	int	open;		/* for DMEXCL */
	int	users;
	uvlong	raoffset;	/* we did read ahead up to this offset */
	int	wadone;		/* we did walk ahead here */
};

struct List
{
	QLock;
	Memblk	*hd;
	Memblk	*tl;
	long	n;
};

struct Link
{
	Memblk	*lprev;
	Memblk	*lnext;
};

/*
 * memory block
 */
struct Memblk
{
	Ref;
	daddrt	addr;			/* block address */
	Memblk	*next;			/* in hash or free list */

	Link;				/* lru / dirty / ref lists */

	Mfile	*mf;			/* DBfile on-memory info. */

	int	type;
	Lock	dirtylk;
	int	dirty;			/* must be written */
	int	frozen;			/* is frozen */
	int	loading;			/* block is being read */
	int	changed;		/* for freerefs/writerefs */
	QLock	newlk;			/* only to wait on DBnew blocks */

	Diskblk	d;
};

/*
 * Slice into a block, used to read/write file blocks.
 */
struct Blksl
{
	Memblk *b;
	void	*data;
	long	len;
};

struct Fsys
{
	QLock;

	struct{
		QLock;
		Memblk	*b;
	} fhash[Fhashsz];	/* hash of blocks by address */

	Memblk	*blk;		/* static global array of memory blocks */
	usize	nblk;		/* # of entries used */
	usize	nablk;		/* # of entries allocated */
	usize	nmused;		/* blocks in use */
	usize	nmfree;		/* free blocks */
	Memblk	*free;		/* free list of unused blocks in blk */

	List	lru;		/* hd: mru; tl: lru */
	List	mdirty;		/* dirty blocks, not on lru */
	List	refs;		/* DBref blocks, neither in lru nor dirty lists */

	QLock	mlk;
	Mfile	*mfree;		/* unused list */


	Memblk	*super;		/* locked by blklk */
	Memblk	*root;		/* only in memory */
	Memblk	*active;		/* /active */
	Memblk	*archive;	/* /archive */
	Memblk	*cons;		/* /cons */
	Memblk	*stats;		/* /stats */
	Channel	*consc;		/* of char*; output for /cons */

	Memblk	*fzsuper;	/* frozen super */

	char	*dev;		/* name for disk */
	int	fd;		/* of disk */
	daddrt	limit;		/* address for end of disk */
	usize	ndblk;		/* # of disk blocks in dev */

	int	nindirs[Niptr];	/* stats */
	int	nmelts;

	QLock	fzlk;		/* freeze, melt, check, write */
	RWLock	quiescence;	/* any activity rlocks() this */
	QLock	policy;		/* fspolicy */

	uchar	*mchk, *dchk;	/* for fscheck() */

	uvlong	wtime;		/* time for last fswrite */
};

/*
 * Misc tools.
 */

struct Cmd
{
	char *name;
	void (*f)(int, char**);
	int nargs;
	char *usage;
};

struct Next
{
	Next *next;
};

struct Alloc
{
	QLock;
	Next *free;
	ulong nfree;
	ulong nalloc;
	usize elsz;
	int zeroing;
};

/*
 * Used to keep references to parents crossed to
 * reach files, to be able to build a melted version of the
 * children. Also to know the parent of a file for things like
 * removals.
 */
struct Path
{
	Path* next;	/* in free list */
	Ref;
	Memblk** f;
	int nf;
	int naf;
};

struct Member
{
	Member *next;
	Usr *u;
};

struct Usr
{
	Usr *nnext;	/* next by name */
	Usr *inext;	/* next by id */

	int id;
	int enabled;
	int allow;
	Usr *lead;
	char name[Unamesz];
	Member *members;
};


#pragma	varargck	type	"H"	Memblk*
#pragma	varargck	type	"A"	Usr*
#pragma	varargck	type	"P"	Path*

/* used in debug prints to print just part of huge values */
#define EP(e)	((e)&0xFFFFFFFFUL)

typedef int(*Blkf)(Memblk*);


extern Fsys*fs;
extern uvlong maxfsz;
extern Alloc mfalloc, pathalloc;
extern int swreaderr, swwriteerr;
extern int fatalaborts;
