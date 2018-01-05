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
	Lru	lru;				/* must be first in struct */
	uint32_t	bno;
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
	Lru	lru;
	int	bsize;			/* block size in bytes */
	int	f;			/* fd to disk */
	Bbuf	*dfirst;		/* dirty list */
	Bbuf	*dlast;
	Bbuf	bb[Nbcache];
};

int	bcinit(Bcache*, int, int);
Bbuf*	bcalloc(Bcache*, uint32_t);
Bbuf*	bcread(Bcache*, uint32_t);
void	bcmark(Bcache*, Bbuf*);
int	bcwrite(Bcache*, Bbuf*);
int	bcsync(Bcache*);
int	bread(Bcache*, uint32_t, void*);
int	bwrite(Bcache*, uint32_t, void*);
int	bref(Bcache*, Bbuf*);
void	error(char*, ...);
void	warning(char*);
