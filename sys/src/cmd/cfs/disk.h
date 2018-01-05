/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Disk	Disk;

/*
 *  Reference to the disk
 */
struct Disk
{
	Bcache	bcache;
	uint32_t	nb;	/* number of blocks */
	uint32_t	nab;	/* number of allocation blocks */
	int	b2b;	/* allocation bits to a block */
	int	p2b;	/* Dptr's per page */
	char	name[CACHENAMELEN];
};

int	dinit(Disk*, int, int, char*);
int	dformat(Disk*, int, char*, uint32_t, uint32_t);
uint32_t	dalloc(Disk*, Dptr*);
uint32_t	dpalloc(Disk*, Dptr*);
int	dfree(Disk*, Dptr*);

extern int debug;

#define DPRINT if(debug)fprint
