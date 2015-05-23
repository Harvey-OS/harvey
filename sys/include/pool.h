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
	uint32_t	maxsize;

	uint32_t	cursize;
	uint32_t	curfree;
	uint32_t	curalloc;

	uint32_t	minarena;	/* smallest size of new arena */
	uint32_t	quantum;	/* allocated blocks should be multiple of */
	uint32_t	minblock;	/* smallest newly allocated block */

	void*	freeroot;	/* actually Free* */
	void*	arenalist;	/* actually Arena* */

	void*	(*alloc)(uint32_t);
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

extern void*	poolalloc(Pool*, uint32_t);
extern void*	poolallocalign(Pool*, uint32_t, uint32_t, int32_t,
				   uint32_t);
extern void	poolfree(Pool*, void*);
extern uint32_t	poolmsize(Pool*, void*);
extern void*	poolrealloc(Pool*, void*, uint32_t);
extern void	poolcheck(Pool*);
extern int	poolcompact(Pool*);
extern void	poolblockcheck(Pool*, void*);

extern Pool*	mainmem;
extern Pool*	imagmem;

enum {	/* flags */
	POOL_ANTAGONISM	= 1<<0,
	POOL_PARANOIA	= 1<<1,
	POOL_VERBOSITY	= 1<<2,
	POOL_DEBUGGING	= 1<<3,
	POOL_LOGGING	= 1<<4,
	POOL_TOLERANCE	= 1<<5,
	POOL_NOREUSE	= 1<<6,
};
