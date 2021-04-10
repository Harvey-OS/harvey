/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Pool Pool;
struct Pool {
	char*	name;
	u32	maxsize;

	u32	cursize;
	u32	curfree;
	u32	curalloc;

	u32	minarena;	/* smallest size of new arena */
	u32	quantum;	/* allocated blocks should be multiple of */
	u32	minblock;	/* smallest newly allocated block */

	void*	freeroot;	/* actually Free* */
	void*	arenalist;	/* actually Arena* */

	void*	(*alloc)(u32);
	int	(*merge)(void*, void*);
	void	(*move)(void* from, void* to);

	int	flags;
	int	nfree;
	int	lastcompact;

	void	(*lock)(Pool*);
	void	(*unlock)(Pool*);
	void	(*print)(Pool*, char*, ...);
	void	(*panic)(Pool*, char*, ...);
	void	(*logstack)(Pool*);

	void*	private;
};

extern void*	poolalloc(Pool*, u32);
extern void*	poolallocalign(Pool*, u32, u32, i32,
				   u32);
extern void	poolfree(Pool*, void*);
extern u32	poolmsize(Pool*, void*);
extern void*	poolrealloc(Pool*, void*, u32);
extern void	poolcheck(Pool*);
extern int	poolcompact(Pool*);
extern void	poolblockcheck(Pool*, void*);

extern Pool*	mainmem;

enum {	/* flags */
	POOL_ANTAGONISM	= 1<<0,
	POOL_PARANOIA	= 1<<1,
	POOL_VERBOSITY	= 1<<2,
	POOL_DEBUGGING	= 1<<3,
	POOL_LOGGING	= 1<<4,
	POOL_TOLERANCE	= 1<<5,
	POOL_NOREUSE	= 1<<6,
};
