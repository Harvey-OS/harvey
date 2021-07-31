enum
{
	Bufsize	= 1024,	/* 5.8 ms each, must be power of two */
	Nbuf		= 128,	/* .74 seconds total */
	Dma		= 6,
	IrqAUDIO	= 7,
	SBswab	= 0,
};

#define seteisadma(a, b)	dmainit(a, Bufsize);
#define CACHELINESZ		8
#define UNCACHED(type, v)	(type*)((ulong)(v))

#define Int0vec
#define setvec(v, f, a)		intrenable(v, f, a, BUSUNKNOWN, "audio")
