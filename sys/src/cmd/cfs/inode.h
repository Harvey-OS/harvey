/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Ibuf	Ibuf;
typedef struct Imap	Imap;
typedef struct Icache	Icache;

enum
{
	Nicache=	64,		/* number of inodes kept in pool */
};

/*
 *  a cached inode buffer
 */
struct Ibuf
{
	Lru;			/* must be first in structure */
	int	inuse;		/* non-0 if in use */
	ulong	ino;		/* index into inode table */
	Inode	inode;		/* the inode contents */
};

/*
 *  in-core qid to inode mapping
 */
struct Imap
{
	Lru;			/* must be first in structure */
	Qid	qid;
	Ibuf	*b;		/* cache buffer */
	int	inuse;		/* non-0 if in use */
};

/*
 *  the inode cache
 */
struct Icache
{
	Disk;

	int	nino;		/* number of inodes */
	ulong	ib0;		/* first inode block */
	int	nib;		/* number of inode blocks */
	int	i2b;		/* inodes to a block */

	Ibuf	ib[Nicache];	/* inode buffers */
	Lru	blru;

	Imap	*map;		/* inode to qid mapping */
	Lru	mlru;
};

Ibuf*	ialloc(Icache*, ulong);
Ibuf*	iget(Icache*, Qid);
Ibuf*	iread(Icache*, ulong);
int	iformat(Icache*, int, ulong, char*, int, int);
int	iinit(Icache*, int, int, char*);
int	iremove(Icache*, ulong);
int	iupdate(Icache*, ulong, Qid);
int	iwrite(Icache*, Ibuf*);
void	ifree(Icache*, Ibuf*);
void	iinc(Icache*, Ibuf*);
