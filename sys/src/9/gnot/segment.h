/*
 * Attach segment types
 */

typedef struct Physseg Physseg;
struct Physseg
{
	ulong	attr;			/* Segment attributes */
	char	*name;			/* Attach name */
	ulong	pa;			/* Physical address */
	ulong	size;			/* Maximum segment size in pages */
	Page	*(*pgalloc)(ulong);	/* Allocation if we need it */
	void	(*pgfree)(Page*);
}physseg[] = {
	{ SG_SHARED,	"lock",		0,	SEGMAXSIZE,	snewpage, 	putpage },
	{ SG_SHARED,	"shared",	0,	SEGMAXSIZE,	snewpage, 	putpage },
	{ SG_PHYSICAL,	"kmem",		KZERO,	SEGMAXSIZE,	0,		0 	},
	{ SG_BSS,	"memory",	0,	SEGMAXSIZE,	snewpage,	putpage },
	{ 0,		0,		0,	0,		0,		0 	},
};
