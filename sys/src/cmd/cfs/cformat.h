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
	BtoUL=		8*sizeof(u32),/* bits in a ulong */
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
	u32	magic;
	u32	bsize;		/* logical block size */
	char	name[CACHENAMELEN];
	short	nab;		/* number of allocation blocks */
};
struct Dalloc
{
	Dahdr	dahdr;
	u32	bits[1];
};

/*
 *  A pointer to disk data
 */
struct Dptr
{
	u32	fbno;		/* file block number */
	u32	bno;		/* disk block number */
	u16	start;		/* offset into block of valid data */
	u16	end;		/* offset into block after valid data */
};

/*
 *  A file descriptor.
 */
struct Inode
{
	Qid	qid;
	i64	length;
	Dptr	ptr;		/* pointer page */
	char	inuse;
};

/*
 *  inode blocks (after allocation blocks)
 */
struct Dihdr
{
	u32	magic;
	u32	nino;		/* number of inodes */
};
struct Dinode
{
	Dihdr	dihdr;
	Inode	inode[1];
};
