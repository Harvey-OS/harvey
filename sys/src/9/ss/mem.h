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
#define	HZ		50			/* clock frequency */
#define	MS2HZ		(1000/HZ)		/* millisec per clock tick */
#define	TK2SEC(t)	((t)/HZ)		/* ticks to seconds */
#define	TK2MS(t)	((((ulong)(t))*1000)/HZ)	/* ticks to milliseconds */
#define	MS2TK(t)	((((ulong)(t))*HZ)/1000)	/* milliseconds to ticks */

/*
 * PSR bits
 */
#define	PSRIMPL		0xF0000000
#define	PSRVER		0x0F000000
#define	PSRRESERVED	0x000FC000
#define	PSREC		0x00002000
#define	PSREF		0x00001000
#define	PSRPIL		0x00000F00
#define	PSRSUPER	0x00000080
#define	PSRPSUPER	0x00000040
#define	PSRET		0x00000020
#define	PSRCWP		0x0000001F
#define	SPL(n)		(n<<8)

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
#define	CONTEXT		0x30000000	/* in ASI 2 */

/*
 * MMU regions
 */
#define	INVALIDSEGM	0xFFFC0000	/* highest seg of VA reserved as invalid */
#define	INVALIDPMEG	(conf.npmeg-1)
#define IOSEGSIZE	(MB + 2*MB)	/* 1 meg for screen plus overhead */	
#define IOSEGM		(INVALIDSEGM - IOSEGSIZE)

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
#define SEGMAPSIZE	128

#define	INVALIDPTE	0
#define	PPN(pa)		(((pa)>>12)&0xFFFF)

/*
 * Weird addresses etc. in System ASI (2)
 */
#define	CACHETAGS	0x80000000
#define	CACHEDATA	0x90000000
#define	SER		0x60000000
#define	SEVAR		0x60000004
#define	ASER		0x60000008
#define	ASEVAR		0x6000000C
#define	ENAB		0x40000000
#define	ENABRESET	0x04
#define	ENABCACHE	0x10
#define ENABDMA		0x20

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
