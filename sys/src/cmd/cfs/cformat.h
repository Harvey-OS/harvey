/*
 *  format of cache on disk
 */
typedef struct Dptr	Dptr;
typedef struct Dahdr	Dahdr;
typedef struct Dalloc	Dalloc;
typedef struct Fphdr	Fphdr;
typedef struct Fptr	Fptr;
typedef struct Inode	Inode;
typedef struct Dihdr	Dihdr;
typedef struct Dinode	Dinode;

enum
{
	Amagic= 	0xbebeefed,	/* allocation block magic */
	Imagic=		0xbadc00ce,	/* inode block magic */
	BtoUL=		8*sizeof(ulong),/* bits in a ulong */
	CACHENAMELEN=	128
};
#define	Indbno		0x80000000	/* indirect block */
#define	Notabno		0xFFFFFFFF	/* not a block number */

/*
 *  Allocation blocks at the begining of the disk.  There are
 *  enough of these blocks to supply 1 bit for each block on the
 *  disk;
 */
struct Dahdr
{
	ulong	magic;
	ulong	bsize;		/* logical block size */
	char	name[CACHENAMELEN];
	short	nab;		/* number of allocation blocks */
};
struct Dalloc
{
	Dahdr;
	ulong	bits[1];
};

/*
 *  A pointer to disk data
 */
struct Dptr
{
	ulong	fbno;		/* file block number */
	ulong	bno;		/* disk block number */
	ushort	start;		/* offset into block of valid data */
	ushort	end;		/* offset into block after valid data */
};

/*
 *  A file descriptor.
 */
struct Inode
{
	Qid	qid;
	vlong	length;
	Dptr	ptr;		/* pointer page */	
	char	inuse;
};

/*
 *  inode blocks (after allocation blocks)
 */
struct Dihdr
{
	ulong	magic;
	ulong	nino;		/* number of inodes */
};
struct Dinode
{
	Dihdr;
	Inode	inode[1];
};
