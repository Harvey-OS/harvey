enum
{
	Bufsize		= 16*1024,	/* 92 ms each */
	Nbuf		= 16,		/* 1.5 seconds total */
	Dma		= 6,
	IrqAUDIO	= 7,

	SBswab		= 1,

	DMA2_WRMASK	= 0xd4,
	DMA2_WRMODE	= 0xd6,
	DMA2_CLRBP	= 0xd8,

	DMA6_ADDRESS	= 0xc8,
	DMA6_COUNT	= 0xca,
	DMA6_LPAGE	= 0x89,
	DMA6_HPAGE	= 0x489,

	Dspport		= 0x220,

	PORT_RESET	= Dspport + 0x6,
	PORT_READ	= Dspport + 0xA,
	PORT_WRITE	= Dspport + 0xC,
	PORT_WSTATUS	= Dspport + 0xC,
	PORT_RSTATUS	= Dspport + 0xE,

	PORT_MIXER_ADDR	= Dspport + 0x4,
	PORT_MIXER_DATA	= Dspport + 0x5,

	PORT_CLRI8	= Dspport + 0xE,
	PORT_CLRI16	= Dspport + 0xF,
	PORT_CLRI401	= Dspport + 0x100,
};

#define	setvec(a,b,c)
#define	outb(a,b)	EISAOUTB(a,b)
#define	inb(a)		EISAINB(a)
