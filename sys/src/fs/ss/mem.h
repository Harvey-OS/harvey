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
#define	HZ		20			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	TK2MS(t)	((((ulong)(t))*1000)/HZ)	/* ticks to milliseconds */
#define	MS2TK(t)	((((ulong)(t))*HZ)/1000)	/* milliseconds to ticks */

/*
 * PSR bits
 */
#define	PSREC		0x00002000
#define	PSREF		0x00001000
#define PSRSUPER	0x00000080
#define PSRPSUPER	0x00000040
#define	PSRET		0x00000020
#define SPL(n)		(n<<8)

/*
 * Magic registers
 */

#define	MACH		6		/* R6 is m-> */
#define	USER		5		/* R5 is u-> */

/*
 * Fundamental addresses
 */

#define	USERADDR	0xE0000000
#define	UREGADDR	(USERADDR+BY2PG-((32+6)*BY2WD))
#define	BOOTSTACK	(KTZERO-0*BY2PG)
#define	TRAPS		(KTZERO-2*BY2PG)

/*
 * MMU
 */

#define	VAMASK		0x3FFFFFFF
#define	BY2SEGM		(1<<18)
#define	PG2SEGM		(1<<6)
#define	NTLBPID		(1+MAXCONTEXT)	/* TLBPID 0 is unallocated */
#define	MAXCONTEXT	16
#define	CONTEXT		0x30000000	/* in ASI 2 */

/*
 * MMU regions
 */

#define INVALIDSEG	0xFFFC0000			/* */
#define IOSEGSIZE	(2*1024*1024)			/* screen + 1Mb */
#define IOSEG		(INVALIDSEG-IOSEGSIZE)		/* I/O and kmap */

#define SEGSHIFT	18
#define SEGSIZE		(1<<SEGSHIFT)			/* 256Kb */
#define SEGNUM(va)	(((va)>>SEGSHIFT) & 0x03FF)	/* 4K segments */
#define SEGOFFSET	(SEGSIZE-1)

#define PGSIZE		BY2PG				/* 4Kb */
#define PGNUM(va)	(((va)>>PGSHIFT) & 0x03F)	/* 64 pages in segment */
#define PGOFFSET	(PGSIZE-1)

/*
 * MMU entries
 */
#define	PTEVALID	(1<<31)
#define	PTERONLY	(0<<30)
#define	PTEWRITE	(1<<30)
#define	PTEKERNEL	(1<<29)
#define	PTENOCACHE	(1<<28)
#define	PTEMAINMEM	(0<<26)
#define	PTEIO		(1<<26)
#define	PTEACCESS	(1<<25)
#define	PTEMODIFY	(1<<24)
#define PTEPROBEMEM	(PTEVALID|PTEKERNEL|PTENOCACHE|PTEWRITE|PTEMAINMEM)
#define PTEPROBEIO	(PTEVALID|PTEKERNEL|PTENOCACHE|PTEWRITE|PTEIO)
#define PTEUNCACHED	0

#define PTEMAPMEM	(1024*1024)	
#define	PTEPERTAB	(PTEMAPMEM/BY2PG)
#define SEGMAPSIZE	16

#define	INVALIDPTE	0
#define	PPN(pa)		((pa>>12)&0xFFFF)

/*
 * Virtual addresses
 */
#define	VTAG(va)	((va>>22)&0x03F)
#define	VPN(va)		((va>>13)&0x1FF)

/*
 * Address spaces
 */
#define	UZERO	0x00000000		/* base of user address space */
#define	UTZERO	(UZERO+BY2PG)		/* first address in user text */
#define	TSTKTOP	0x10000000		/* end of new stack in sysexec */
#define TSTKSIZ 32
#define	USTKTOP	(TSTKTOP-TSTKSIZ*BY2PG)	/* byte just beyond user stack */
#define	KZERO	0xE0000000		/* base of kernel address space */
#define	KTZERO	(KZERO+4*BY2PG)		/* first address in kernel text */
#define	USTKSIZE	(4*1024*1024)	/* size of user stack */

#define	MACHSIZE	4096

#define MACHADDR	((ulong)&mach0)	/* hack number 1 */
