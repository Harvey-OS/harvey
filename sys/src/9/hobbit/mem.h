/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */
#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2PG		4096			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		12			/* log(BY2PG) */
#define PGROUND(s)	(((s)+(BY2PG-1))&~(BY2PG-1))

#define	MAXMACH		1			/* max # cpus system can run */

/*
 * Time
 */
#define HZ		25			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	TK2MS(t)	((t)*MS2HZ)		/* ticks to milliseconds */
#define	MS2TK(t)	((t)/MS2HZ)		/* milliseconds to ticks */

/*
 * MMU
 */
#define	PTEVALID	0x00000001		/* valid */
#define	PTEUNCACHED	0
#define PTEWRITE	0x00000002		/* writeable */
#define	PTERONLY	0
#define PPN(x)		((x)&~(BY2PG-1))
#define PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	16

/*
 * Address spaces
 */
#define RAMBASE		0x80000000
#define IOBASE		0xB0000000
#define PROMBASE	0x00000000
#define VROMBASE	0xC0000000

#ifdef rom

#define KTEXTBASE	VROMBASE
#define KDATABASE	(VROMBASE+0x00600000)
#define KTZERO		(KTEXTBASE)		/* first address in kernel text */

#define KADDR(a)	((void*)(((ulong)(a)-RAMBASE)+KZERO+0x400000))
#define PADDR(a)	(((ulong)(a)-KZERO-0x400000)+RAMBASE)

#else

#define KTEXTBASE	RAMBASE
#define KDATABASE	RAMBASE
#define KTZERO		(KTEXTBASE+0x4000)	/* first address in kernel text */

#define KADDR(a)	((void*)(a))
#define PADDR(a)	((ulong)(a))

#endif /* rom */

#define KZERO		KTEXTBASE	/* base of kernel address space */
#define	TSTKTOP		0xFFC00000	/* end of new stack in sysexec */
#define TSTKSIZ 	10

#define UZERO		0x00000000	/* base of user address space */
#define	UTZERO		(UZERO+0x1000)	/* first address in user text */
#define	USTKTOP		KZERO		/* byte just beyond user stack */
#define	USTKSIZE	(4*1024*1024)	/* size of user stack */

/*
 * Fundamental addresses
 */
#define VBADDR		(KDATABASE+0x0000)
#define KSTBADDR	(KDATABASE+0x1000)
#define MACHADDR	(KDATABASE+0x2000)
#define ISPOFFSET	(0x1000-0x0020)
#define SPOFFSET	(0x1000-0x0100)

#define	USERADDR	0xFFFFE000
#define UREGADDR	(USERADDR+ISPOFFSET-0x0020)
