enum
{
	Bufsize		= 16*1024,	/* 92 ms each */
	Nbuf		= 16,		/* 1.5 seconds total */
	Dma		= 6,
	IrqAUDIO	= 7,
	SBswab		= 0,
};

#define seteisadma(a, b)	dmainit(a, Bufsize);
#define CACHELINESZ		8
#define UNCACHED(type, v)	(type*)((ulong)(v))

#define Int0vec
#define setvec(v, f, a)		intrenable(v, f, a, BUSUNKNOWN, "audio")
