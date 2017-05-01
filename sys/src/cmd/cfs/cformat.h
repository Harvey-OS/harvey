/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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
	BtoUL=		8*sizeof(uint32_t),/* bits in a ulong */
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
	uint32_t	magic;
	uint32_t	bsize;		/* logical block size */
	char	name[CACHENAMELEN];
	short	nab;		/* number of allocation blocks */
};
struct Dalloc
{
	Dahdr;
	uint32_t	bits[1];
};

/*
 *  A pointer to disk data
 */
struct Dptr
{
	uint32_t	fbno;		/* file block number */
	uint32_t	bno;		/* disk block number */
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
	uint32_t	magic;
	uint32_t	nino;		/* number of inodes */
};
struct Dinode
{
	Dihdr;
	Inode	inode[1];
};
