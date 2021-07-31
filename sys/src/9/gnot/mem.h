/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */

/*
 * Sizes
 */

#define	BI2BY		8			/* bits per byte */
#define BI2WD		32			/* bits per word */
#define	BY2WD		4			/* bytes per word */
#define	BY2PG		8192			/* bytes per page */
#define	WD2PG		(BY2PG/BY2WD)		/* words per page */
#define	PGSHIFT		13			/* log(BY2PG) */
#define PGROUND(s)	(((s)+(BY2PG-1))&~(BY2PG-1))
#define ICACHESIZE	0
#define MB4		(4*1024*1024)		/* Lots of things are 4Mb in size */

#define	MAXMACH		1			/* max # cpus system can run */

/*
 * Time
 */
#define	HZ		(60)			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	TK2MS(t)	((((ulong)(t))*1000)/HZ)	/* ticks to milliseconds */
#define	MS2TK(t)	((((ulong)(t))*HZ)/1000)	/* milliseconds to ticks */

/*
 * SR bits
 */
#define SUPER		0x2000
#define SPL(n)		(n<<8)

/*
 * CACR
 */
#define	CCLEAR		0x08
#define	CENABLE		0x01

/*
 * Magic registers (unused in current system)
 */

#define	MACH		A5		/* A5 is m-> */
#define	USER		A4		/* A4 is u-> */

/*
 * Fundamental addresses
 */

#define	USERADDR	0x80000000
/* assuming we're in a syscall, this is the address of the Ureg structure */
#define	UREGVARSZ	(23*BY2WD)	/* size of variable part of Ureg */
#define	UREGADDR	(USERADDR+BY2PG-(UREGVARSZ+2+4+2+(8+8+1+1)*BY2WD))

/*
 * Devices poked during bootstrap
 */
#define	TACADDR		0x40600000
#define	MOUSE		0x40200000

/*
 * MMU
 */

#define	VAMASK	0xCFFFFFFF	/* clear balu bits in address */
#define	KUSEG	0x00000000
#define KSEG	0x80000000

/*
 * MMU entries
 */
#define	PTEVALID	(1<<13)
#define PTEWRITE	0
#define	PTERONLY	(1<<14)
#define	PTEKERNEL	(1<<15)
#define PTEUNCACHED	0
#define	INVALIDPTE	0
#define PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	16

#define	PPN(pa)		((pa>>13)&0x1FFF)

#define	KMAP	((unsigned long *)0xD0000000)
#define	UMAP	((unsigned long *)0x50000000)

/*
 * Virtual addresses
 */
#define	VTAG(va)	((va>>22)&0x03F)
#define	VPN(va)		((va>>13)&0x1FF)

#define	PARAM		((char*)0x40500000)
#define	TLBFLUSH_	0x01

/*
 * Address spaces
 */

#define	UZERO	KUSEG			/* base of user address space */
#define	UTZERO	(UZERO+BY2PG)		/* first address in user text */
#define	TSTKTOP	0x10000000		/* end of new stack in sysexec */
#define TSTKSIZ 100
#define	USTKTOP	(TSTKTOP-TSTKSIZ*BY2PG) /* byte just beyond user stack */
#define	KZERO	KSEG			/* base of kernel address space */
#define	KTZERO	(KZERO+BY2PG)		/* first address in kernel text */
#define	USTKSIZE	(4*1024*1024)	/* size of user stack */

#define	MACHSIZE	4096


#define isphys(p) ((((ulong)(p))&0xF0000000) == KSEG)
