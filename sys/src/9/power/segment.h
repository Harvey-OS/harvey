/*
 * Attach segment types
 */

#define VME16IO3	0x17C00000	
#define VME32TDFB	(0x30000000+128*MB)

Physseg physseg[] =
{
	{ SG_PHYSICAL,	"lock",		0,	1024*BY2PG,	lkpage,		lkpgfree },
	{ SG_SHARED,	"shared",	0,	SEGMAXSIZE,	0, 0 },
	{ SG_BSS,	"memory",	0,	SEGMAXSIZE,	0, 0 },
	{ SG_PHYSICAL,	"vme16",	VME16IO3, 	64*1024,	0, 0 },
	{ SG_PHYSICAL,	"tdsfb",	VME32TDFB, 	4*MB,		0, 0 },
	{ 0,		0,		0,	0,		0, 0 },
};
