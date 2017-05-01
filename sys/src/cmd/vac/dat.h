/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct MetaBlock MetaBlock;
typedef struct MetaEntry MetaEntry;

#define MaxBlock (1UL<<31)

enum {
	BytesPerEntry = 100,	/* estimate of bytes per dir entries - determines number of index entries in the block */
	FullPercentage = 80,	/* don't allocate in block if more than this percentage full */
	FlushSize = 200,	/* number of blocks to flush */
	DirtyPercentage = 50	/* maximum percentage of dirty blocks */
};


struct MetaEntry
{
	uchar *p;
	ushort size;
};

struct MetaBlock
{
	int maxsize;		/* size of block */
	int size;		/* size used */
	int free;		/* free space within used size */
	int maxindex;		/* entries allocated for table */
	int nindex;		/* amount of table used */
	int unbotch;
	uchar *buf;
};

struct VacDirEnum
{
	VacFile *file;
	u32int boff;
	int i, n;
	VacDir *buf;
};
