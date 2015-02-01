/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Bbuf	Bbuf;
typedef struct Bcache	Bcache;

enum
{
	Nbcache=	32,		/* number of blocks kept in pool */
};

/*
 *  block cache descriptor
 */
struct Bbuf
{
	Lru;				/* must be first in struct */
	ulong	bno;
	int	inuse;
	Bbuf	*next;			/* next in dirty list */
	int	dirty;
	char	*data;
};

/*
 *  the buffer cache
 */
struct Bcache
{
	Lru;
	int	bsize;			/* block size in bytes */
	int	f;			/* fd to disk */
	Bbuf	*dfirst;		/* dirty list */
	Bbuf	*dlast;
	Bbuf	bb[Nbcache];
};

int	bcinit(Bcache*, int, int);
Bbuf*	bcalloc(Bcache*, ulong);
Bbuf*	bcread(Bcache*, ulong);
void	bcmark(Bcache*, Bbuf*);
int	bcwrite(Bcache*, Bbuf*);
int	bcsync(Bcache*);
int	bread(Bcache*, ulong, void*);
int	bwrite(Bcache*, ulong, void*);
int	bref(Bcache*, Bbuf*);
void	error(char*, ...);
void	warning(char*);
