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

#ifdef rom
	{ SG_PHYSICAL,	"physm",	0x80000000, 0x00800000,	0,	0 },
	{ SG_PHYSICAL,	"io",		0xB0000000, 0x08004000,	0,	0 },
#endif /* rom */

	{ SG_SHARED,	"shared",	0,	SEGMAXSIZE,	0, 	0 },
	{ SG_PHYSICAL,	"kmem",		KZERO,	SEGMAXSIZE,	0,	0 },
	{ SG_BSS,	"memory",	0,	SEGMAXSIZE,	0,	0 },
	{ 0,		0,		0,	0,		0,	0 },
};
