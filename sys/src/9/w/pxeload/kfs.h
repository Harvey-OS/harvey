/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Qid9p1 Qid9p1;
typedef struct Dentry Dentry;
typedef struct Kfsfile Kfsfile;
typedef struct Kfs Kfs;

/* DONT TOUCH, this is the disk structure */
struct	Qid9p1
{
	long	path;
	long	version;
};

//#define	NAMELEN		28		/* size of names */
#define	NDBLOCK		6		/* number of direct blocks in Dentry */

/* DONT TOUCH, this is the disk structure */
struct	Dentry
{
	char	name[NAMELEN];
	short	uid;
	short	gid;
	ushort	mode;
/*
		#define	DALLOC	0x8000
		#define	DDIR	0x4000
		#define	DAPND	0x2000
		#define	DLOCK	0x1000
		#define	DREAD	0x4
		#define	DWRITE	0x2
		#define	DEXEC	0x1
*/
	Qid9p1	qid;
	long	size;
	long	dblock[NDBLOCK];
	long	iblock;
	long	diblock;
	long	atime;
	long	mtime;
};

struct Kfsfile
{
	Dentry;
	long off;
};

struct Kfs
{
	int	RBUFSIZE;
	int	BUFSIZE;
	int	DIRPERBUF;
	int	INDPERBUF;
	int	INDPERBUF2;
};

extern int kfsinit(Fs*);

