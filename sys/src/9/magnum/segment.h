/*
 * Attach segment types
 */

Physseg physseg[] = {
	{ SG_SHARED,	"shared",	0,	SEGMAXSIZE,	0, 	0 },
	{ SG_BSS,	"memory",	0,	SEGMAXSIZE,	0,	0 },
	{ SG_PHYSICAL,	"fb",		0,	0,		0,	0 },
	{ 0,		0,		0,	0,		0,	0 },
};
